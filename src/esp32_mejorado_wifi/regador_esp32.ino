/*
  =============================================================================
  PROYECTO: Regador Automático Inteligente (Versión Optimizada con Seguridad & WiFi)
  PLACA: ESP32 DevKit V1 (o adaptable a Arduino Nano con buzzer y botón)
  =============================================================================
  MEJORAS IMPLEMENTADAS:
    1. Sensor de Flotador Físico (Evita quemar la bomba en seco).
    2. Buzzer Activo de Alarma Suave (Aviso auditivo para abuela cuando falta agua).
    3. Botón Físico de Riego Manual (Riego inmediato de 3s al pulsar).
    4. Servidor Web Local WiFi (Monitoreo desde el celular del nieto o abuela).
  =============================================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIGURACION WIFI (Opcional: Cambia por tu red o déjalo en modo Standalone) ---
const char* WIFI_SSID = "TU_WIFI_AQUI";
const char* WIFI_PASS = "TU_PASSWORD_AQUI";
WebServer server(80);

// --- CONFIGURACION PANTALLA OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- ASIGNACION DE PINES (ESP32) ---
const int PIN_SENSOR_HUMEDAD = 34; // Pin ADC1 (Solo entrada, ideal para sensores)
const int PIN_DRIVER_BOMBA   = 18; // Salida a Gate del MOSFET
const int PIN_FLOTADOR_AGUA  = 19; // Entrada flotador (INPUT_PULLUP: LOW = Hay agua, HIGH = Vacio)
const int PIN_BUZZER         = 23; // Salida al zumbador activo 5V/3.3V
const int PIN_BOTON_MANUAL   = 4;  // Pulsador con INPUT_PULLUP (LOW al presionar)

// --- PARAMETROS DE CALIBRACION ADC ESP32 (12 bits: 0 a 4095) ---
const int CAL_AIRE_ESP32 = 3100; // Valor seco en el aire
const int CAL_AGUA_ESP32 = 1350; // Valor sumergido en agua

// --- UMBRALES DE HUMEDAD (%) ---
const int UMBRAL_SECO   = 25;
const int UMBRAL_OPTIMO = 70;

// --- TIEMPOS ---
const unsigned long MAX_TIEMPO_RIEGO = 20000; // 20s máx
unsigned long ultimoMuestreo = 0;
unsigned long tiempoInicioRiego = 0;
unsigned long ultimoBeep = 0;

int humedadActual = 50;
bool hayAguaEnTanque = true;
bool regando = false;

// =============================================================================
// LECTURA DE SENSORES
// =============================================================================
int leerHumedad() {
  long suma = 0;
  for (int i = 0; i < 20; i++) {
    suma += analogRead(PIN_SENSOR_HUMEDAD);
    delay(4);
  }
  int raw = suma / 20;
  int pct = map(raw, CAL_AIRE_ESP32, CAL_AGUA_ESP32, 0, 100);
  return constrain(pct, 0, 100);
}

void actualizarDisplay(const char* estadoMsg) {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("REGADOR PRO (ESP32)"));
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  // Humedad
  display.setTextSize(3);
  display.setCursor(10, 16);
  display.print(humedadActual);
  display.print(F("%"));

  // Icono o texto de agua
  display.setTextSize(1);
  display.setCursor(85, 20);
  display.print(hayAguaEnTanque ? F("[AGUA]") : F("[VACIO]"));

  // Barra de progreso
  display.drawRect(0, 44, 128, 6, SSD1306_WHITE);
  display.fillRect(0, 44, map(humedadActual, 0, 100, 0, 128), 6, SSD1306_WHITE);

  // Mensaje
  display.setCursor(0, 54);
  display.print(F("INFO: "));
  display.print(estadoMsg);

  display.display();
}

// =============================================================================
// INTERFAZ WEB LOCAL (DASHBOARD)
// =============================================================================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Regador de Plantas</title><style>";
  html += "body{font-family:sans-serif;text-align:center;background:#f0f7f4;margin-top:40px;}";
  html += ".card{background:white;padding:25px;border-radius:15px;display:inline-block;box-shadow:0 4px 10px rgba(0,0,0,0.1);}";
  html += ".hum{font-size:50px;color:#2e7d32;font-weight:bold;}";
  html += ".btn{background:#00897b;color:white;padding:12px 25px;border:none;border-radius:8px;font-size:18px;cursor:pointer;}";
  html += "</style></head><body><div class='card'>";
  html += "<h2>Planta de la Abuela</h2>";
  html += "<div class='hum'>" + String(humedadActual) + "%</div>";
  html += "<p>Humedad del Suelo</p>";
  html += "<p>Estado Tanque: <b>" + String(hayAguaEnTanque ? "Lleno (OK)" : "VACIO - RELLENAR") + "</b></p>";
  html += "<a href='/regar'><button class='btn'>Regar 3 Segundos</button></a>";
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleRegarManualWeb() {
  if (hayAguaEnTanque) {
    digitalWrite(PIN_DRIVER_BOMBA, HIGH);
    delay(3000);
    digitalWrite(PIN_DRIVER_BOMBA, LOW);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_DRIVER_BOMBA, OUTPUT);
  digitalWrite(PIN_DRIVER_BOMBA, LOW);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  pinMode(PIN_FLOTADOR_AGUA, INPUT_PULLUP);
  pinMode(PIN_BOTON_MANUAL, INPUT_PULLUP);

  Wire.begin(21, 22); // SDA, SCL en ESP32
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Fallo OLED"));
  }

  // Intento de conexión WiFi no bloqueante
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 8) {
    delay(500);
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    server.on("/", handleRoot);
    server.on("/regar", handleRegarManualWeb);
    server.begin();
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }

  humedadActual = leerHumedad();
}

// =============================================================================
// LOOP PRINCIPAL
// =============================================================================
void loop() {
  server.handleClient();
  unsigned long ahora = millis();

  // Lectura periódica de sensores
  if (ahora - ultimoMuestreo >= 1000) {
    ultimoMuestreo = ahora;
    humedadActual = leerHumedad();
    // En flotadores típicos normalmente abiertos: LOW = Flotador arriba (hay agua)
    hayAguaEnTanque = (digitalRead(PIN_FLOTADOR_AGUA) == LOW);
  }

  // 1. Control del Buzzer Pasivo si el depósito se quedó sin agua
  if (!hayAguaEnTanque) {
    digitalWrite(PIN_DRIVER_BOMBA, LOW); // Cortar bomba de inmediato
    actualizarDisplay("RELLENAR AGUA!");

    // Pitido suave grave (850 Hz) cada 6 segundos para avisar a la abuela
    if (ahora - ultimoBeep >= 6000) {
      ultimoBeep = ahora;
      tone(PIN_BUZZER, 850, 80);
      delay(120);
      tone(PIN_BUZZER, 850, 80);
    }
    return; // Salir de loop hasta que haya agua
  }

  // 2. Botón manual presionado por la abuela
  if (digitalRead(PIN_BOTON_MANUAL) == LOW && !regando) {
    actualizarDisplay("RIEGO MANUAL");
    digitalWrite(PIN_DRIVER_BOMBA, HIGH);
    delay(3000); // Riega 3 segundos
    digitalWrite(PIN_DRIVER_BOMBA, LOW);
    humedadActual = leerHumedad();
  }

  // 3. Lógica automática de autorriego
  if (!regando && humedadActual <= UMBRAL_SECO) {
    regando = true;
    tiempoInicioRiego = ahora;
    digitalWrite(PIN_DRIVER_BOMBA, HIGH);
  }

  if (regando) {
    actualizarDisplay("REGANDO...");
    if (humedadActual >= UMBRAL_OPTIMO || (ahora - tiempoInicioRiego >= MAX_TIEMPO_RIEGO)) {
      digitalWrite(PIN_DRIVER_BOMBA, LOW);
      regando = false;
    }
  } else {
    actualizarDisplay(humedadActual < UMBRAL_SECO ? "TIERRA SECA" : "HUMEDAD OK");
  }
}
