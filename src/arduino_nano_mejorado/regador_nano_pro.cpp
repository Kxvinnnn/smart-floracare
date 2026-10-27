/**
 * ============================================================================
 * SMART FLORACARE - FIRMWARE ARDUINO NANO PRO (ARQUITECTURA MODULAR)
 * ============================================================================
 * Placa: Arduino Nano V3 (ATmega328P, 5V, 16MHz)
 * ============================================================================
 */

#include <Arduino.h>
#include <stdint.h>
#include <avr/wdt.h>
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
    // Deshabilitar y reconfigurar WDT en el arranque para evitar bucles de reset en AVR
    MCUSR = 0;
    wdt_disable();
    wdt_enable(WDTO_4S); // Watchdog hardware activo (4 segundos)

    Serial.begin(115200);

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
// TELEMETRÍA SERIE PERIÓDICA ATÓMICA Y NO BLOQUEANTE (ORDEN DE TRABAJO UART)
// ============================================================================
static void emitPeriodicTelemetry(uint32_t nowMs) {
    static uint32_t lastTelemetryMs = 0;
    if (static_cast<uint32_t>(nowMs - lastTelemetryMs) < 1000) {
        return;
    }
    lastTelemetryMs = nowMs;

    // Búfer local en pila (0 reserva dinámica en heap, 64 bytes acotados)
    char buf[64];
    int len = 0;

    if (controller.getState() == ControllerState::FAULT) {
        const char* faultStr = "OTHER";
        switch (controller.getLastFault()) {
            case FaultReason::NO_WATER_LEVEL:       faultStr = "NO_WATER_LEVEL"; break;
            case FaultReason::MAX_TIMEOUT_EXCEEDED: faultStr = "TIMEOUT"; break;
            case FaultReason::SENSOR_MALFUNCTION:   faultStr = "SENSOR_ERR"; break;
            case FaultReason::INVALID_REQUEST:      faultStr = "INVALID_REQ"; break;
            default:                                faultStr = "NONE"; break;
        }
        len = snprintf(buf, sizeof(buf), "[STATE] FAULT | FAULT:%s | %s | %s\n",
                       faultStr,
                       waterLevelOk ? "WATER:OK" : "WATER:LOW",
                       controller.isPumpActive() ? "PUMP:ON" : "PUMP:OFF");
    } else {
        const char* stateStr = "IDLE";
        switch (controller.getState()) {
            case ControllerState::IDLE:       stateStr = "IDLE"; break;
            case ControllerState::VALIDATING: stateStr = "VALIDATING"; break;
            case ControllerState::WATERING:   stateStr = "WATERING"; break;
            case ControllerState::STOPPED:    stateStr = "STOPPED"; break;
            case ControllerState::RECOVERY:   stateStr = "RECOVERY"; break;
            default:                          stateStr = "IDLE"; break;
        }
        if (sensorStatus == SensorHealth::OK) {
            len = snprintf(buf, sizeof(buf), "[STATE] %s | MOIST:%u | %s | %s\n",
                           stateStr,
                           static_cast<unsigned int>(currentMoisturePct),
                           waterLevelOk ? "WATER:OK" : "WATER:LOW",
                           controller.isPumpActive() ? "PUMP:ON" : "PUMP:OFF");
        } else {
            len = snprintf(buf, sizeof(buf), "[STATE] %s | MOIST:ERR | %s | %s\n",
                           stateStr,
                           waterLevelOk ? "WATER:OK" : "WATER:LOW",
                           controller.isPumpActive() ? "PUMP:ON" : "PUMP:OFF");
        }
    }

    // Descarte atómico seguro: Si el búfer serie no tiene espacio suficiente para
    // la trama ENTERA (incluido fin de línea), abortar inmediatamente sin transmitir
    // nada. Garantiza 0 µs de bloqueo y evita fragmentaciones de tramas.
    if (len <= 0 || len >= static_cast<int>(sizeof(buf)) || Serial.availableForWrite() < len) {
        return;
    }

    Serial.write(reinterpret_cast<const uint8_t*>(buf), static_cast<size_t>(len));
}

// ============================================================================
// LAZO PRINCIPAL NO BLOQUEANTE
// ============================================================================
void loop() {
    wdt_reset(); // Alimentar perro guardián hardware en cada iteración
    const uint32_t nowMs = millis();

    // 1. TAREA CRÍTICA: Antirrebote e integración temporal de boya (REQ-07, ACT-05.3a)
    static FloraCare::FloatSwitchDebouncer floatDebouncer(digitalRead(PIN_FLOTADOR_AGUA) == LOW);
    waterLevelOk = floatDebouncer.update(digitalRead(PIN_FLOTADOR_AGUA) == LOW, nowMs);

    // Auto-recuperación de FAULT ante normalización física verificable (H-01 Opción A)
    if (controller.getState() == ControllerState::FAULT) {
        controller.attemptRecovery(nowMs, waterLevelOk, sensorStatus);
    }

    // 2. MUESTREO DE HUMEDAD: Cada 1000ms
    if (static_cast<uint32_t>(nowMs - lastSampleTimeMs) >= 1000) {
        lastSampleTimeMs = nowMs;
        uint16_t rawAdc = sampleAnalogFast();
        SensorValidator::calculateMoisturePercent(rawAdc, NANO_CALIBRATION, currentMoisturePct, sensorStatus);

        // Si el sensor falla durante un ciclo automático, parar inmediatamente (H-05)
        if (sensorStatus != SensorHealth::OK &&
            controller.getState() == ControllerState::WATERING &&
            controller.getActiveSource() == IrrigationSource::AUTO_CYCLE) {
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

    // 3. ENTRADA DEL BOTÓN MANUAL CON DETECCIÓN DE FLANCO Y REARME DE FAULT (H-01, H-06)
    static bool buttonStableState = HIGH;
    const bool currentBtn = digitalRead(PIN_BOTON_MANUAL);
    if (currentBtn != lastButtonReading) {
        lastButtonDebounceMs = nowMs;
        lastButtonReading = currentBtn;
    }
    if (static_cast<uint32_t>(nowMs - lastButtonDebounceMs) >= 40) {
        if (currentBtn != buttonStableState) {
            buttonStableState = currentBtn;
            if (buttonStableState == LOW) { // Flanco descendente
                if (controller.getState() == ControllerState::FAULT) {
                    // Procedimiento explícito de rearme ante fallo
                    if (controller.resetFault(nowMs, waterLevelOk, sensorStatus)) {
                        tone(PIN_BUZZER_PASIVO, 1500, 100); // Beep rearme exitoso
                    } else {
                        tone(PIN_BUZZER_PASIVO, 600, 120);  // Beep rechazo (condición insegura)
                    }
                } else if (controller.getState() == ControllerState::IDLE) {
                    if (controller.requestWatering(IrrigationSource::MANUAL_BUTTON,
                                                  nowMs,
                                                  waterLevelOk,
                                                  sensorStatus,
                                                  currentMoisturePct)) {
                        tone(PIN_BUZZER_PASIVO, 1200, 50); // Beep confirmación
                    }
                }
            }
        }
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

    // 6. RENDERIZADO OLED CADA 500ms (INDICACIÓN FIDEDIGNA DE ESTADO REAL)
    if (static_cast<uint32_t>(nowMs - lastDisplayTimeMs) >= 500) {
        lastDisplayTimeMs = nowMs;
        const __FlashStringHelper* msg = F("OPTIMO");
        if (controller.getState() == ControllerState::FAULT) {
            if (controller.getLastFault() == FaultReason::NO_WATER_LEVEL) {
                msg = F("SIN AGUA!");
            } else if (controller.getLastFault() == FaultReason::SENSOR_MALFUNCTION) {
                msg = F("SENSOR ERR");
            } else if (controller.getLastFault() == FaultReason::MAX_TIMEOUT_EXCEEDED) {
                msg = (controller.getLastStopReason() == StopReason::SAFETY_TIMEOUT)
                      ? F("WDT 25S ERR")
                      : F("TIMEOUT 20S");
            } else {
                msg = F("FALLO SEGURO");
            }
        } else if (!waterLevelOk) {
            msg = F("SIN AGUA!");
        } else if (controller.isPumpActive()) {
            msg = F("REGANDO...");
        } else if (sensorStatus != SensorHealth::OK) {
            msg = F("SENSOR ERR");
        } else if (currentMoisturePct <= NANO_IRRIGATION_CFG.autoStartMoisturePct) {
            msg = F("SUELO SECO");
        }
        renderDisplay(msg);
    }

    // 7. TELEMETRÍA SERIE PERIÓDICA NO BLOQUEANTE (1000ms, PRIORIDAD BAJA)
    emitPeriodicTelemetry(nowMs);
}
