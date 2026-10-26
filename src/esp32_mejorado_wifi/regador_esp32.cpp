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

// Inclusión segura de credenciales locales (ignorado por Git si existe)
#if __has_include("Credentials.h")
  #include "Credentials.h"
#endif

using namespace FloraCare;

// --- CONFIGURACIÓN WIFI Y SEGURIDAD HTTP ---
#ifndef SECRET_WIFI_SSID
  #define SECRET_WIFI_SSID "FLORACARE_SETUP_AP"
#endif

#ifndef SECRET_WIFI_PASS
  #define SECRET_WIFI_PASS "plantas123"
#endif

#ifndef SECRET_API_TOKEN
  #define SECRET_API_TOKEN "FLORACARE_DEFAULT_TOKEN_CHANGE_ME"
#endif

static const char* WIFI_SSID      = SECRET_WIFI_SSID;
static const char* WIFI_PASS      = SECRET_WIFI_PASS;
static const char* API_AUTH_TOKEN = SECRET_API_TOKEN;

static WebServer server(80);

// --- PANTALLA OLED SSD1306 (128x64 I2C) ---
static constexpr uint8_t SCREEN_WIDTH   = 128;
static constexpr uint8_t SCREEN_HEIGHT  = 64;
static constexpr int8_t  OLED_RESET     = -1;
static constexpr uint8_t SCREEN_ADDRESS = 0x3C;
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- ASIGNACIÓN DE PINES (ESP32 HAL) ---
static constexpr uint8_t PIN_SENSOR_HUMEDAD = 34; // ADC1_CH6 (Solo entrada)
static constexpr uint8_t PIN_DRIVER_BOMBA   = 18; // Salida digital a Gate MOSFET
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
// SERVIDOR WEB Y API REST PROTEGIDA
// ============================================================================
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
    html += F("</b></p>"
              "<form method='POST' action='/api/water' style='margin-top:16px;'>"
              "<input type='password' name='token' placeholder='Token de autorizaci&oacute;n' required "
              "style='padding:8px;border-radius:4px;border:1px solid #ccc;width:80%;max-width:240px;margin-bottom:10px;'><br>"
              "<button type='submit' class='btn'>Regar 3 Segundos (Manual)</button>"
              "</form></div></body></html>");

    server.send(200, "text/html", html);
}

static void handleApiWater() {
    // 1. Verificación obligatoria de método HTTP POST
    if (server.method() != HTTP_POST) {
        server.send(405, "application/json", F("{\"error\":\"Method Not Allowed. Use POST.\"}"));
        return;
    }

    // 2. Verificación de robustez del token de configuración
    // Si el token es el placeholder por defecto o tiene menos de 16 caracteres, la API se inhabilita por seguridad
    if (strcmp(API_AUTH_TOKEN, "FLORACARE_DEFAULT_TOKEN_CHANGE_ME") == 0 ||
        strcmp(API_AUTH_TOKEN, "CAMBIA_ESTE_TOKEN_SECRETO_12345") == 0 ||
        strlen(API_AUTH_TOKEN) < 16) {
        server.send(403, "application/json", F("{\"error\":\"Forbidden. Remote API disabled: Insecure or default token configured in Credentials.h\"}"));
        return;
    }

    // 3. Validación de autorización por Token (previene accesos no autorizados / CSRF)
    String token = server.arg("token");
    String authHeader = server.header("Authorization");
    if (authHeader.startsWith("Bearer ")) {
        authHeader = authHeader.substring(7);
    }
    const bool authorized = (token == API_AUTH_TOKEN) || (authHeader == API_AUTH_TOKEN);
    if (!authorized) {
        server.send(401, "application/json", F("{\"error\":\"Unauthorized. Invalid token.\"}"));
        return;
    }

    // 4. Validación centralizada a través del controlador de riego
    const uint32_t nowMs = millis();
    bool accepted = controller.requestWatering(IrrigationSource::REMOTE_WEB,
                                              nowMs,
                                              waterLevelOk,
                                              sensorStatus,
                                              currentMoisturePct);

    if (accepted) {
        // Redireccionar al dashboard o responder JSON
        server.sendHeader("Location", "/");
        server.send(303);
    } else {
        String err = F("{\"error\":\"Watering request rejected\",\"state\":\"");
        err += static_cast<int>(controller.getState());
        err += F("\"}");
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

    // Conexión WiFi con límite de reintentos
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint8_t retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
        delay(300);
        retries++;
    }

    // Configuración de endpoints y encabezados del servidor HTTP
    const char* headerKeys[] = {"Authorization"};
    server.collectHeaders(headerKeys, 1);
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/water", HTTP_POST, handleApiWater);
    server.begin();

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
