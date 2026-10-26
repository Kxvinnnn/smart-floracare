/**
 * ============================================================================
 * SMART FLORACARE - FIRMWARE ESP32 CON MONITOREO SEGURO Y CONTROL CENTRALIZADO
 * ============================================================================
 * Placa: ESP32 DevKit V1 (Xtensa Dual-Core 3.3V)
 * ============================================================================
 */

#include <stdint.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "SensorValidator.h"
#include "IrrigationController.h"

using namespace FloraCare;

// --- GESTIÓN ESTRICTA DE CREDENCIALES (CERO HARDCODED DEFAULTS) ---
#if __has_include("Credentials.h")
  #include "Credentials.h"
  #define HAS_LOCAL_CREDENTIALS 1
#else
  #define HAS_LOCAL_CREDENTIALS 0
#endif

#if HAS_LOCAL_CREDENTIALS && defined(SECRET_WIFI_SSID) && defined(SECRET_WIFI_PASS)
  static const char* WIFI_SSID       = SECRET_WIFI_SSID;
  static const char* WIFI_PASS       = SECRET_WIFI_PASS;
  static constexpr bool WIFI_ENABLED = true;
#else
  static const char* WIFI_SSID       = "";
  static const char* WIFI_PASS       = "";
  static constexpr bool WIFI_ENABLED = false;
#endif

#if HAS_LOCAL_CREDENTIALS && defined(SECRET_API_TOKEN)
  static const char* API_AUTH_TOKEN = SECRET_API_TOKEN;
#else
  static const char* API_AUTH_TOKEN = "";
#endif

static WebServer server(80);

// --- PANTALLA OLED SSD1306 (128x64 I2C) ---
static constexpr uint8_t SCREEN_WIDTH   = 128;
static constexpr uint8_t SCREEN_HEIGHT  = 64;
static constexpr int8_t  OLED_RESET     = -1;
static constexpr uint8_t SCREEN_ADDRESS = 0x3C;
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- ASIGNACIÓN DE PINES (ESP32 HAL) ---
static constexpr uint8_t PIN_SENSOR_HUMEDAD = 34; // ADC1_CH6 (Solo entrada)
static constexpr uint8_t PIN_DRIVER_BOMBA   = 18; // Salida digital a Entrada Pre-Driver NPN+PNP
static constexpr uint8_t PIN_FLOTADOR_AGUA  = 19; // Entrada digital (INPUT_PULLUP)
static constexpr uint8_t PIN_BUZZER         = 23; // Salida digital al zumbador
static constexpr uint8_t PIN_BOTON_MANUAL   = 4;  // Entrada digital (INPUT_PULLUP)

// --- PERFIL DE CALIBRACIÓN ADC ESP32 (12 BITS: 0 - 4095) ---
static constexpr CalibrationProfile ESP32_CALIBRATION = {
    3100, // airRaw (Seco total)
    1350, // waterRaw (Sumergido en agua)
    100,  // minPhysLimit (Bajo 100 implica cortocircuito o masa)
    4000, // maxPhysLimit (Sobre 4000 implica desconexión / flotante)
    200   // minDelta requerido
};

// --- CONFIGURACIÓN DEL CONTROLADOR DE RIEGO ---
static constexpr ControllerConfig IRRIGATION_CFG = {
    20000, // maxAutoDurationMs: 20 segundos
    3000,  // manualBurstDurationMs: 3 segundos
    25000, // maxSafetyWatchdogMs: 25 segundos absoluto
    5000,  // cooldownPeriodMs: 5 segundos de enfriamiento entre ciclos
    25,    // autoStartMoisturePct: Iniciar si humedad <= 25%
    70     // autoStopMoisturePct: Detener si humedad >= 70%
};

static IrrigationController controller(IRRIGATION_CFG);

// --- VARIABLES DE TELEMETRÍA Y TEMPORIZACIÓN NO BLOQUEANTE ---
static uint32_t lastSampleTimeMs  = 0;
static uint32_t lastDisplayTimeMs = 0;
static uint32_t lastBeepTimeMs    = 0;
static uint8_t  currentMoisturePct= 50;
static SensorHealth sensorStatus  = SensorHealth::OK;
static bool     waterLevelOk      = true;
static bool     lastButtonState   = HIGH;
static uint32_t lastButtonDebounceMs = 0;

// --- ESTADO DEL DISPLAY OLED ---
static bool oledAvailable = false;

// ============================================================================
// LECTURA FILTRADA DEL SENSOR ANALÓGICO (SOBREMUESTREO RÁPIDO NO BLOQUEANTE)
// ============================================================================
static uint16_t sampleAnalogFast() {
    uint32_t accum = 0;
    static constexpr uint8_t SAMPLES = 16;
    for (uint8_t i = 0; i < SAMPLES; ++i) {
        accum += analogRead(PIN_SENSOR_HUMEDAD);
        delayMicroseconds(200); // 16 * 200us = 3.2ms total (no bloquea el servidor)
    }
    return static_cast<uint16_t>(accum / SAMPLES);
}

// ============================================================================
// CAPA DE PRESENTACIÓN: RENDERIZADO OLED
// ============================================================================
static void updateDisplay(const char* statusMsg) {
    if (!oledAvailable) {
        return;
    }
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(F("FLORACARE PRO (ESP32)"));
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    // Humedad
    display.setTextSize(3);
    display.setCursor(6, 16);
    if (sensorStatus == SensorHealth::OK) {
        display.print(currentMoisturePct);
        display.print(F("%"));
    } else {
        display.print(F("ERR"));
    }

    // Nivel de agua
    display.setTextSize(1);
    display.setCursor(80, 20);
    display.print(waterLevelOk ? F("[AGUA OK]") : F("[VACIO!]"));

    // Barra de progreso
    display.drawRect(0, 44, 128, 6, SSD1306_WHITE);
    if (sensorStatus == SensorHealth::OK) {
        uint8_t bar = static_cast<uint8_t>(map(currentMoisturePct, 0, 100, 0, 128));
        display.fillRect(0, 44, bar, 6, SSD1306_WHITE);
    }

    display.setCursor(0, 54);
    display.print(F("ESTADO: "));
    display.print(statusMsg);

    display.display();
}

// ============================================================================
// SERVIDOR WEB Y API REST PROTEGIDA CON AUTENTICACIÓN SEGURA Y DEFENSIVA
// ============================================================================

// --- LÍMITES Y POLÍTICA DE SEGURIDAD PARA AUTENTICACIÓN ---
static constexpr size_t MIN_TOKEN_LEN = 16;
static constexpr size_t MAX_TOKEN_LEN = 128;

/**
 * Valida si el token configurado en firmware es nulo, menor a 16 caracteres
 * o corresponde a un marcador de posición de ejemplo conocido.
 */
static bool isDefaultOrInsecureToken(const char* token) {
    if (!token) {
        return true;
    }
    const size_t len = strlen(token);
    if (len < MIN_TOKEN_LEN) {
        return true;
    }
    if (strcmp(token, "GENERA_UN_TOKEN_SECRETO_DE_16_O_MAS_CARACTERES") == 0) {
        return true;
    }
    if (strcmp(token, "CAMBIA_ESTE_TOKEN_SECRETO_12345") == 0) {
        return true;
    }

    if (strcmp(token, "1234567890123456") == 0) {
        return true;
    }
    return false;
}

/**
 * Comparación segura en tiempo constante para mitigar ataques de temporización (timing attacks).
 * Recorre siempre las posiciones requeridas sin salidas tempranas basadas en valor de caracteres.
 */
static bool safeConstantTimeEquals(const char* a, const char* b, size_t maxLen = MAX_TOKEN_LEN) {
    if (!a || !b) {
        return false;
    }
    const size_t lenA = strlen(a);
    const size_t lenB = strlen(b);
    if (lenA > maxLen || lenB > maxLen) {
        return false;
    }

    volatile uint8_t diff = static_cast<uint8_t>(lenA ^ lenB);
    const size_t maxIter = (lenA > lenB) ? lenA : lenB;
    for (size_t i = 0; i < maxIter; ++i) {
        const uint8_t ca = (i < lenA) ? static_cast<uint8_t>(a[i]) : 0;
        const uint8_t cb = (i < lenB) ? static_cast<uint8_t>(b[i]) : 0;
        diff |= (ca ^ cb);
    }
    return (diff == 0);
}

/**
 * Valida el origen de la solicitud para mitigar ataques CSRF desde navegadores (RFC 6454).
 * Un origen web se compone de la tupla (esquema, host, puerto).
 * Política de seguridad estricta para Smart FloraCare en entorno local IPv4:
 *  - Si no existe cabecera "Origin" ni "Referer", se permite el acceso exclusivamente a
 *    clientes programáticos (cURL, scripts Python, Home Assistant) debido a que la API
 *    exige un token secreto en cada petición y el firmware NO utiliza cookies ni sesiones.
 *  - Si la cabecera está presente pero vacía o contiene solo espacios, SE RECHAZA.
 *  - Esquema obligatorio: estrictamente "http://" (se rechaza "https://", "null", etc.).
 *  - No se permiten credenciales en la autoridad (ej. usuario@host) ni caracteres inválidos.
 *  - IPv6 se rechaza explícitamente (el sistema opera exclusivamente sobre red local IPv4).
 *  - Puerto efectivo: debe ser estrictamente 80 o implícito. Se rechazan puertos dispares,
 *    no canónicos (ej. ":080", ":+80"), vacíos (ej. ":") o no numéricos (ej. ":80a").
 *  - Host permitido: debe coincidir exactamente con la dirección IPv4 local asignada
 *    al dispositivo (WiFi.localIP()). No se admiten nombres mDNS no configurados ni
 *    se refleja el encabezado Host del cliente.
 *  - Si se reciben simultáneamente Origin y Referer, AMBOS deben ser válidos y coincidir
 *    con la autoridad local autorizada (sin contradicciones).
 */
static bool validateSingleAuthority(String target, bool isOriginHeader) {
    target.trim();
    if (target.length() < 8) {
        return false;
    }

    // 1. Esquema obligatorio http:// (H-01.5: se rechaza estrictamente https://, ftp://, etc.)
    String scheme = target.substring(0, 7);
    scheme.toLowerCase();
    if (scheme != "http://") {
        return false;
    }
    String authority = target.substring(7);

    // 2. Delimitadores según tipo de cabecera
    if (isOriginHeader) {
        // H-01.1: Un Origin válido NO puede contener '/', '?' ni '#' (RFC 6454)
        if (authority.indexOf('/') != -1 || authority.indexOf('?') != -1 || authority.indexOf('#') != -1) {
            return false;
        }
    } else {
        // Para Referer, extraer la autoridad previa a la ruta, query o fragmento
        int slashIdx = authority.indexOf('/');
        if (slashIdx != -1) authority = authority.substring(0, slashIdx);
        int questionIdx = authority.indexOf('?');
        if (questionIdx != -1) authority = authority.substring(0, questionIdx);
        int hashIdx = authority.indexOf('#');
        if (hashIdx != -1) authority = authority.substring(0, hashIdx);
    }

    // 3. Rechazar caracteres de control (< 33), no-ASCII (> 126), espacios, userinfo (@) o corchetes IPv6 (H-01.3)
    if (authority.length() == 0) {
        return false;
    }
    for (unsigned int i = 0; i < authority.length(); ++i) {
        const char c = authority.charAt(i);
        if (c < 33 || c > 126 || c == '@' || c == '[' || c == ']') {
            return false;
        }
    }

    // 4. Separación de Host y Puerto con validación estricta (H-01.6)
    String originHost = "";
    String originPort = "";
    int colonIdx = authority.indexOf(':');

    if (colonIdx != -1) {
        originHost = authority.substring(0, colonIdx);
        originPort = authority.substring(colonIdx + 1);

        // Puerto presente debe tener dígitos válidos y ser estrictamente 80
        if (originPort.length() == 0) {
            return false; // Delimitador ":" presente sin dígitos (ej. "http://192.168.1.50:")
        }
        for (unsigned int i = 0; i < originPort.length(); ++i) {
            if (!isDigit(originPort.charAt(i))) {
                return false; // Caracteres no numéricos en puerto (ej. "http://192.168.1.50:80a", ":+80")
            }
        }
        // Exigir representación canónica exacta "80" (rechaza 8080, 080 con ceros a la izquierda, etc.)
        if (originPort != "80") {
            return false;
        }
    } else {
        originHost = authority; // Puerto implícito 80
    }

    if (originHost.length() == 0) {
        return false;
    }

    // H-01.2 & H-01.7: Normalización a minúsculas y validación contra la IP local real asignada
    originHost.toLowerCase();
    String localIp = WiFi.localIP().toString();
    if (localIp.length() > 0 && localIp != "0.0.0.0" && originHost.equals(localIp)) {
        return true;
    }

    return false;
}

static bool isOriginAllowed() {
    const bool hasOrigin = server.hasHeader("Origin");
    const bool hasReferer = server.hasHeader("Referer");

    if (!hasOrigin && !hasReferer) {
        // Clientes REST legítimos sin navegador (cURL, scripts Python, Home Assistant)
        // La protección contra CSRF se garantiza mediante la ausencia total de credenciales ambientales
        // (sin cookies ni sesiones) y la obligatoriedad de presentar el token secreto Bearer en cada petición.
        return true;
    }

    // Si Origin está presente, DEBE ser rigurosamente válido
    if (hasOrigin) {
        if (!validateSingleAuthority(server.header("Origin"), true)) {
            return false;
        }
    }

    // Si Referer está presente, DEBE ser rigurosamente válido
    if (hasReferer) {
        if (!validateSingleAuthority(server.header("Referer"), false)) {
            return false;
        }
    }

    return true;
}

static void handleRoot() {
    String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                    "<meta name='viewport' content='width=device-width,initial-scale=1.0'>"
                    "<title>Smart FloraCare Dashboard</title><style>"
                    "body{font-family:sans-serif;text-align:center;background:#f0f4f8;margin:30px;}"
                    ".card{background:white;padding:24px;border-radius:12px;display:inline-block;box-shadow:0 4px 12px rgba(0,0,0,0.1);max-width:380px;}"
                    ".metric{font-size:48px;font-weight:bold;color:#1e4620;margin:12px 0;}"
                    ".btn{background:#2e7d32;color:white;border:none;padding:12px 24px;border-radius:6px;font-size:16px;cursor:pointer;font-weight:bold;}"
                    ".btn:hover{background:#1b5e20;}"
                    ".warn{color:#c62828;font-weight:bold;}"
                    "</style></head><body><div class='card'>"
                    "<h2>Smart FloraCare Dashboard</h2>"
                    "<div class='metric'>");
    if (sensorStatus == SensorHealth::OK) {
        html += String(currentMoisturePct) + "%";
    } else {
        html += "SENSOR ERROR";
    }
    html += F("</div><p>Dep&oacute;sito: <b>");
    html += waterLevelOk ? "Nivel Adecuado (OK)" : "<span class='warn'>VAC&Iacute;O - RELLENAR</span>";
    html += F("</b></p><p>Estado Bomba: <b>");
    html += controller.isPumpActive() ? "<span style='color:#1565c0'>REGANDO...</span>" : "Reposo";
    html += F("</b></p>");

    if (isDefaultOrInsecureToken(API_AUTH_TOKEN)) {
        html += F("<p class='warn'>[Control remoto deshabilitado: configure un token seguro en Credentials.h]</p>");
    } else {
        html += F("<form method='POST' action='/api/water' style='margin-top:16px;'>"
                  "<input type='password' name='token' placeholder='Token de autorizaci&oacute;n' required "
                  "style='padding:8px;border-radius:4px;border:1px solid #ccc;width:80%;max-width:240px;margin-bottom:10px;'><br>"
                  "<button type='submit' class='btn'>Regar 3 Segundos (Manual)</button>"
                  "</form>");
    }

    html += F("</div></body></html>");
    server.send(200, "text/html", html);
}

// --- GESTIÓN DE TASA DE INTENTOS FALLIDOS (RATE LIMITING NO BLOQUEANTE) ---
static uint32_t lastFailedAuthMs = 0;

static void handleApiWater() {
    const uint32_t nowMs = millis();

    // 1. Verificación obligatoria de método HTTP POST
    if (server.method() != HTTP_POST) {
        server.send(405, "application/json", F("{\"error\":\"Method Not Allowed. Use POST.\"}"));
        return;
    }

    // 2. Control de tasa no bloqueante ante fallos previos de autenticación
    // Si hubo un fallo en los últimos 1000 ms, responder con 429 sin bloquear la CPU ni el loop()
    if (lastFailedAuthMs > 0 && (static_cast<uint32_t>(nowMs - lastFailedAuthMs) < 1000)) {
        server.send(429, "application/json", F("{\"error\":\"Too Many Requests. Authentication rate limit active. Please wait.\"}"));
        return;
    }

    // 3. Inspección temprana de tamaño de carga (previene DoS por consumo de Heap)
    if (server.clientContentLength() > 256) {
        server.send(413, "application/json", F("{\"error\":\"Payload Too Large. Request body exceeds maximum allowed size.\"}"));
        return;
    }

    // 4. Verificación de seguridad del token configurado en firmware
    // Si el token es un placeholder o no cumple con la longitud mínima, se deshabilita la API
    if (isDefaultOrInsecureToken(API_AUTH_TOKEN)) {
        server.send(403, "application/json", F("{\"error\":\"Forbidden. Remote API disabled: Insecure or default token configured in Credentials.h\"}"));
        return;
    }

    // 5. Verificación canónica de origen (Defensa en profundidad contra CSRF en navegadores)
    if (!isOriginAllowed()) {
        server.send(403, "application/json", F("{\"error\":\"Forbidden. Cross-Origin request blocked.\"}"));
        return;
    }

    // 6. Extracción y normalización de credenciales (Formulario vs Header Authorization)
    String formToken = server.arg("token");
    String authHeader = server.header("Authorization");
    String headerToken = "";
    bool hasHeaderAuth = false;

    if (authHeader.length() > 0) {
        if (authHeader.length() > (MAX_TOKEN_LEN + 16)) {
            server.send(400, "application/json", F("{\"error\":\"Bad Request. Authorization header exceeds maximum allowed length.\"}"));
            return;
        }
        if (authHeader.startsWith("Bearer ")) {
            headerToken = authHeader.substring(7);
            headerToken.trim();
            hasHeaderAuth = true;
        } else {
            // Esquema de autorización inválido (p. ej. Basic o Bearer sin espacio)
            server.send(401, "application/json", F("{\"error\":\"Unauthorized. Invalid Authorization scheme. Expected 'Bearer <token>'.\"}"));
            return;
        }
    }

    bool hasFormAuth = (formToken.length() > 0);

    // Detección de credenciales contradictorias si se envían simultáneamente por formulario y cabecera
    if (hasFormAuth && hasHeaderAuth) {
        if (formToken != headerToken) {
            server.send(400, "application/json", F("{\"error\":\"Bad Request. Conflicting credentials provided in form and Authorization header.\"}"));
            return;
        }
    }

    String chosenToken = hasHeaderAuth ? headerToken : formToken;

    // Validación de presencia de token
    if (chosenToken.length() == 0) {
        server.send(401, "application/json", F("{\"error\":\"Unauthorized. Missing authentication token.\"}"));
        return;
    }

    // Límite defensivo de longitud de entrada
    if (chosenToken.length() > MAX_TOKEN_LEN) {
        server.send(400, "application/json", F("{\"error\":\"Bad Request. Token exceeds maximum allowed length.\"}"));
        return;
    }

    // 7. Comparación en tiempo constante contra el secreto configurado
    const bool authorized = safeConstantTimeEquals(chosenToken.c_str(), API_AUTH_TOKEN);
    if (!authorized) {
        // Registrar marca de tiempo del fallo para rate limiting no bloqueante (CERO BLOQUEO DEL LOOP)
        lastFailedAuthMs = nowMs;
        server.send(401, "application/json", F("{\"error\":\"Unauthorized. Invalid token.\"}"));
        return;
    }

    // 8. Validación centralizada inmutable a través del controlador de riego
    bool accepted = controller.requestWatering(IrrigationSource::REMOTE_WEB,
                                              nowMs,
                                              waterLevelOk,
                                              sensorStatus,
                                              currentMoisturePct);

    if (accepted) {
        // Redireccionar al dashboard (para formularios de navegador) o responder con 303
        server.sendHeader("Location", "/");
        server.send(303);
    } else {
        // Si fue rechazada por el controlador, responder con 409 Conflict y estado detallado
        String err = F("{\"error\":\"Watering request rejected by safety controller\",\"state\":");
        err += static_cast<int>(controller.getState());
        err += F(",\"lastFault\":");
        err += static_cast<int>(controller.getLastFault());
        err += F("}");
        server.send(409, "application/json", err);
    }
}

// ============================================================================
// CONFIGURACIÓN INICIAL (SETUP)
// ============================================================================
void setup() {
    Serial.begin(115200);

    pinMode(PIN_DRIVER_BOMBA, OUTPUT);
    digitalWrite(PIN_DRIVER_BOMBA, LOW); // Estado seguro incondicional al arranque

    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    pinMode(PIN_FLOTADOR_AGUA, INPUT_PULLUP);
    pinMode(PIN_BOTON_MANUAL, INPUT_PULLUP);

    Wire.begin(21, 22);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("[AVISO] Falla inicializando OLED. Operando sin pantalla."));
        oledAvailable = false;
    } else {
        oledAvailable = true;
    }

    // Conexión WiFi con límite de reintentos (solo si se definieron credenciales en Credentials.h)
    if (WIFI_ENABLED && strlen(WIFI_SSID) > 0) {
        Serial.print(F("[WIFI] Conectando a red local... "));
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        uint8_t retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < 15) {
            delay(300);
            retries++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.print(F("Conectado. IP: "));
            Serial.println(WiFi.localIP());
            const char* headerKeys[] = {"Authorization", "Origin", "Host", "Referer"};
            server.collectHeaders(headerKeys, 4);
            server.on("/", HTTP_GET, handleRoot);
            server.on("/api/water", HTTP_POST, handleApiWater);
            server.begin();
            if (isDefaultOrInsecureToken(API_AUTH_TOKEN)) {
                Serial.println(F("[SEGURIDAD] AVISO: Token de API inseguro o ausente. /api/water rechazara peticiones (HTTP 403)."));
            }
        } else {
            Serial.println(F("Fallo de conexion. Operando en modo local autonomo."));
        }
    } else {
        Serial.println(F("[SEGURIDAD] Sin Credentials.h configurado o SSID vacio. Wi-Fi y API remota deshabilitados."));
        WiFi.mode(WIFI_OFF);
    }

    // Muestreo inicial
    uint16_t raw = sampleAnalogFast();
    SensorValidator::calculateMoisturePercent(raw, ESP32_CALIBRATION, currentMoisturePct, sensorStatus);
    waterLevelOk = (digitalRead(PIN_FLOTADOR_AGUA) == LOW);
}

// ============================================================================
// BUCLE PRINCIPAL DETERMINISTA Y NO BLOQUEANTE
// ============================================================================
void loop() {
    server.handleClient();
    const uint32_t nowMs = millis();

    // 1. TAREA CRÍTICA: Lectura periódica y debouncing del flotador
    // El flotador se evalúa cada 50ms para respuesta inmediata si el nivel cae
    static uint32_t lastFloatCheckMs = 0;
    if (static_cast<uint32_t>(nowMs - lastFloatCheckMs) >= 50) {
        lastFloatCheckMs = nowMs;
        // LOW = Flotador arriba (Agua OK), HIGH = Abajo (Vacío)
        waterLevelOk = (digitalRead(PIN_FLOTADOR_AGUA) == LOW);
    }

    // 2. MUESTREO DE HUMEDAD: Cada 1000ms
    if (static_cast<uint32_t>(nowMs - lastSampleTimeMs) >= 1000) {
        lastSampleTimeMs = nowMs;
        uint16_t rawAdc = sampleAnalogFast();
        SensorValidator::calculateMoisturePercent(rawAdc, ESP32_CALIBRATION, currentMoisturePct, sensorStatus);

        // Si el sensor está en fallo, evaluar corte automático
        if (sensorStatus != SensorHealth::OK && controller.getState() == ControllerState::WATERING) {
            controller.stop(StopReason::SENSOR_FAULT, nowMs);
        }

        // Intento automático de riego si se cumplen las condiciones
        if (sensorStatus == SensorHealth::OK && currentMoisturePct <= IRRIGATION_CFG.autoStartMoisturePct) {
            controller.requestWatering(IrrigationSource::AUTO_CYCLE,
                                      nowMs,
                                      waterLevelOk,
                                      sensorStatus,
                                      currentMoisturePct);
        }
    }

    // 3. ENTRADA DEL BOTÓN FÍSICO CON DEBOUNCING NO BLOQUEANTE
    const bool currentBtnReading = digitalRead(PIN_BOTON_MANUAL);
    if (currentBtnReading != lastButtonState) {
        lastButtonDebounceMs = nowMs;
        lastButtonState = currentBtnReading;
    }
    if ((static_cast<uint32_t>(nowMs - lastButtonDebounceMs) >= 40) && (currentBtnReading == LOW)) {
        // Disparo centralizado por botón
        controller.requestWatering(IrrigationSource::MANUAL_BUTTON,
                                  nowMs,
                                  waterLevelOk,
                                  sensorStatus,
                                  currentMoisturePct);
    }

    // 4. ACTUALIZACIÓN DEL CONTROLADOR CENTRALIZADO DE RIEGO
    const bool pumpShouldRun = controller.update(nowMs, waterLevelOk, sensorStatus, currentMoisturePct);
    digitalWrite(PIN_DRIVER_BOMBA, pumpShouldRun ? HIGH : LOW);

    // 5. GESTIÓN NO BLOQUEANTE DEL BUZZER EN ESTADO DE ALERTA
    if (!waterLevelOk) {
        if (static_cast<uint32_t>(nowMs - lastBeepTimeMs) >= 6000) {
            lastBeepTimeMs = nowMs;
            tone(PIN_BUZZER, 850, 100); // Duración de 100ms gestionada por hardware timer
        }
    }

    // 6. ACTUALIZACIÓN DE PANTALLA OLED CADA 500ms
    if (static_cast<uint32_t>(nowMs - lastDisplayTimeMs) >= 500) {
        lastDisplayTimeMs = nowMs;
        const char* msg = "OPTIMO";
        if (!waterLevelOk) {
            msg = "SIN AGUA!";
        } else if (controller.isPumpActive()) {
            msg = "REGANDO...";
        } else if (sensorStatus != SensorHealth::OK) {
            msg = "SENSOR ERR";
        } else if (currentMoisturePct <= IRRIGATION_CFG.autoStartMoisturePct) {
            msg = "SUELO SECO";
        }
        updateDisplay(msg);
    }
}
