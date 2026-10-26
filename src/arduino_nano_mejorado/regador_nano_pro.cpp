/**
 * ============================================================================
 * SMART FLORACARE - FIRMWARE ARDUINO NANO PRO (ARQUITECTURA MODULAR)
 * ============================================================================
 * Placa: Arduino Nano V3 (ATmega328P, 5V, 16MHz)
 * ============================================================================
 */

#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "SensorValidator.h"
#include "IrrigationController.h"

using namespace FloraCare;

// --- CONFIGURACIÓN DE PANTALLA OLED ---
static constexpr uint8_t SCREEN_WIDTH   = 128;
static constexpr uint8_t SCREEN_HEIGHT  = 64;
static constexpr int8_t  OLED_RESET     = -1;
static constexpr uint8_t SCREEN_ADDRESS = 0x3C;
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- ASIGNACIÓN DE PINES (HAL ARDUINO NANO) ---
static constexpr uint8_t PIN_SENSOR_HUMEDAD = A0; // Entrada Analógica (ADC 10 bits: 0-1023)
static constexpr uint8_t PIN_DRIVER_BOMBA   = 3;  // Salida Digital (Gate MOSFET IRLZ44N)
static constexpr uint8_t PIN_FLOTADOR_AGUA  = 4;  // Entrada Digital (Pull-up interno)
static constexpr uint8_t PIN_BUZZER_PASIVO  = 5;  // Salida PWM / Frecuencia
static constexpr uint8_t PIN_BOTON_MANUAL   = 2;  // Entrada Digital (Pull-up interno)

// --- PERFIL DE CALIBRACIÓN ADC ARDUINO NANO (10 BITS: 0 - 1023) ---
static constexpr CalibrationProfile NANO_CALIBRATION = {
    580, // airRaw (0% humedad estimada)
    270, // waterRaw (100% humedad estimada)
    50,  // minPhysLimit (Bajo 50 implica cortocircuito o tierra)
    980, // maxPhysLimit (Sobre 980 implica desconexión o flotante)
    50   // minDelta requerido
};

// --- CONFIGURACIÓN DEL CONTROLADOR DE RIEGO UNIFICADO ---
static constexpr ControllerConfig NANO_IRRIGATION_CFG = {
#if defined(FLORACARE_BENCH_TEST_WATCHDOG_25S)
    30000, // MODO ENSAYO AISLADO ACT-06.A2: Eleva límite auto para alcanzar el Watchdog de 25s
#else
    20000, // maxAutoDurationMs: 20 segundos máx (Unificado con ESP32)
#endif
    3000,  // manualBurstDurationMs: 3 segundos
    25000, // maxSafetyWatchdogMs: 25 segundos absoluto (Perro guardián de seguridad independiente)
    5000,  // cooldownPeriodMs: 5 segundos de enfriamiento entre riegos
    20,    // autoStartMoisturePct: Iniciar si humedad <= 20%
    70     // autoStopMoisturePct: Detener si humedad >= 70%
};

static IrrigationController controller(NANO_IRRIGATION_CFG);

// --- TELEMETRÍA Y TEMPORIZACIÓN NO BLOQUEANTE ---
static uint32_t lastSampleTimeMs     = 0;
static uint32_t lastDisplayTimeMs    = 0;
static uint32_t lastBeepAlertTimeMs  = 0;
static uint8_t  currentMoisturePct   = 50;
static SensorHealth sensorStatus     = SensorHealth::OK;
static bool     waterLevelOk         = true;
static bool     lastButtonReading    = HIGH;
static uint32_t lastButtonDebounceMs = 0;

// --- ESTADO DEL DISPLAY OLED ---
static bool oledAvailable = false;

// ============================================================================
// SERVICIO DE FILTRADO ANALÓGICO (SOBREMUESTREO RÁPIDO)
// ============================================================================
static uint16_t sampleAnalogFast() {
    uint32_t accum = 0;
    static constexpr uint8_t SAMPLES = 16;
    for (uint8_t i = 0; i < SAMPLES; ++i) {
        accum += analogRead(PIN_SENSOR_HUMEDAD);
        delayMicroseconds(250);
    }
    return static_cast<uint16_t>(accum / SAMPLES);
}

// ============================================================================
// CAPA DE PRESENTACIÓN: RENDERIZADO OLED SSD1306
// ============================================================================
static void renderDisplay(const __FlashStringHelper* statusLabel) {
    if (!oledAvailable) {
        return; // No intentar renderizar si la pantalla falló o no está conectada
    }
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(8, 2);
    display.print(F("SMART FLORACARE PRO"));
    display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

    // Porcentaje grande
    display.setTextSize(3);
    display.setCursor(14, 18);
    if (sensorStatus == SensorHealth::OK) {
        display.print(currentMoisturePct);
        display.print(F("%"));
    } else {
        display.print(F("ERR"));
    }

    // Indicador nivel depósito
    display.setTextSize(1);
    display.setCursor(82, 24);
    display.print(waterLevelOk ? F("[AGUA]") : F("[VACIO]"));

    // Barra de progreso
    display.drawRect(4, 46, 120, 6, SSD1306_WHITE);
    if (sensorStatus == SensorHealth::OK) {
        uint8_t barWidth = static_cast<uint8_t>(map(currentMoisturePct, 0, 100, 0, 120));
        display.fillRect(4, 46, barWidth, 6, SSD1306_WHITE);
    }

    display.setCursor(6, 55);
    display.print(F("ESTADO: "));
    display.print(statusLabel);

    display.display();
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(9600);

    pinMode(PIN_DRIVER_BOMBA, OUTPUT);
    digitalWrite(PIN_DRIVER_BOMBA, LOW);

    pinMode(PIN_BUZZER_PASIVO, OUTPUT);
    noTone(PIN_BUZZER_PASIVO);

    pinMode(PIN_FLOTADOR_AGUA, INPUT_PULLUP);
    pinMode(PIN_BOTON_MANUAL, INPUT_PULLUP);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("[AVISO] OLED SSD1306 no responde. Riego continuo autonomo habilitado sin pantalla."));
        oledAvailable = false;
    } else {
        oledAvailable = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(16, 20);
        display.println(F("INICIALIZANDO..."));
        display.setCursor(20, 35);
        display.println(F("FLORACARE NANO"));
        display.display();
        delay(500);
    }

    uint16_t raw = sampleAnalogFast();
    SensorValidator::calculateMoisturePercent(raw, NANO_CALIBRATION, currentMoisturePct, sensorStatus);
    waterLevelOk = (digitalRead(PIN_FLOTADOR_AGUA) == LOW);
}

// ============================================================================
// LAZO PRINCIPAL NO BLOQUEANTE
// ============================================================================
void loop() {
    const uint32_t nowMs = millis();

    // 1. TAREA CRÍTICA: Lectura rápida del flotador cada 50ms
    static uint32_t lastFloatCheckMs = 0;
    if (static_cast<uint32_t>(nowMs - lastFloatCheckMs) >= 50) {
        lastFloatCheckMs = nowMs;
        waterLevelOk = (digitalRead(PIN_FLOTADOR_AGUA) == LOW);
    }

    // 2. MUESTREO DE HUMEDAD: Cada 1000ms
    if (static_cast<uint32_t>(nowMs - lastSampleTimeMs) >= 1000) {
        lastSampleTimeMs = nowMs;
        uint16_t rawAdc = sampleAnalogFast();
        SensorValidator::calculateMoisturePercent(rawAdc, NANO_CALIBRATION, currentMoisturePct, sensorStatus);

        // Si el sensor falla en pleno riego, parar inmediatamente
        if (sensorStatus != SensorHealth::OK && controller.getState() == ControllerState::WATERING) {
            controller.stop(StopReason::SENSOR_FAULT, nowMs);
        }

        // Intento automático de riego
        if (sensorStatus == SensorHealth::OK && currentMoisturePct <= NANO_IRRIGATION_CFG.autoStartMoisturePct) {
            controller.requestWatering(IrrigationSource::AUTO_CYCLE,
                                      nowMs,
                                      waterLevelOk,
                                      sensorStatus,
                                      currentMoisturePct);
        }
    }

    // 3. ENTRADA DEL BOTÓN MANUAL CON DEBOUNCING
    const bool currentBtn = digitalRead(PIN_BOTON_MANUAL);
    if (currentBtn != lastButtonReading) {
        lastButtonDebounceMs = nowMs;
        lastButtonReading = currentBtn;
    }
    if ((static_cast<uint32_t>(nowMs - lastButtonDebounceMs) >= 40) && (currentBtn == LOW)) {
        if (controller.getState() == ControllerState::IDLE) {
            tone(PIN_BUZZER_PASIVO, 1200, 50); // Beep confirmación
        }
        controller.requestWatering(IrrigationSource::MANUAL_BUTTON,
                                  nowMs,
                                  waterLevelOk,
                                  sensorStatus,
                                  currentMoisturePct);
    }

    // 4. ACTUALIZACIÓN DEL CONTROLADOR CENTRALIZADO DE RIEGO
    const bool pumpShouldRun = controller.update(nowMs, waterLevelOk, sensorStatus, currentMoisturePct);
    digitalWrite(PIN_DRIVER_BOMBA, pumpShouldRun ? HIGH : LOW);

    // 5. ALARMA AUDITIVA SUAVE POR BUZZER PASIVO
    if (!waterLevelOk) {
        if (static_cast<uint32_t>(nowMs - lastBeepAlertTimeMs) >= 6000) {
            lastBeepAlertTimeMs = nowMs;
            tone(PIN_BUZZER_PASIVO, 850, 80);
        }
    }

    // 6. RENDERIZADO OLED CADA 500ms
    if (static_cast<uint32_t>(nowMs - lastDisplayTimeMs) >= 500) {
        lastDisplayTimeMs = nowMs;
        const __FlashStringHelper* msg = F("OPTIMO");
        if (!waterLevelOk) {
            msg = F("SIN AGUA!");
        } else if (controller.isPumpActive()) {
            msg = F("REGANDO...");
        } else if (sensorStatus != SensorHealth::OK) {
            msg = F("SENSOR ERR");
        } else if (controller.getState() == ControllerState::FAULT &&
                   controller.getLastFault() == FaultReason::MAX_TIMEOUT_EXCEEDED) {
            msg = (controller.getLastStopReason() == StopReason::SAFETY_TIMEOUT)
                  ? F("WDT 25S ERR")
                  : F("TIMEOUT 20S");
        } else if (currentMoisturePct <= NANO_IRRIGATION_CFG.autoStartMoisturePct) {
            msg = F("SUELO SECO");
        }
        renderDisplay(msg);
    }
}
