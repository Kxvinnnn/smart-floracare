/**
 * ============================================================================
 * PROYECTO: Smart FloraCare - Regador Automático Inteligente PRO
 * ESTÁNDAR: Embedded Clean Architecture / MISRA C++ Compliant Subset
 * TARGET:   Arduino Nano (ATmega328P, 5V, 16MHz)
 * ============================================================================
 * COMPONENTES DE HARDWARE:
 *   - Microcontrolador: Arduino Nano V3 (ATmega328P)
 *   - Sensor de Humedad: Capacitive Soil Moisture Sensor v1.2 (Pin A0)
 *   - Pantalla: OLED 0.96" SSD1306 128x64 I2C (SDA -> A4, SCL -> A5)
 *   - Driver de Potencia: MOSFET N-Channel IRLZ44N / FQP30N06L (Pin D3)
 *   - Nivel de Agua: Sensor de Flotador Mecánico CS-CO058 / Vertical (Pin D4)
 *   - Alarma Acústica: Buzzer Pasivo Piezoeléctrico TMB12A05 + R=220Ω (Pin D5)
 *   - Entrada de Usuario: Pulsador Tact Switch 6x6mm (Pin D2)
 * ============================================================================
 */

#include <stdint.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIGURACIÓN DE PANTALLA OLED ---
static constexpr uint8_t SCREEN_WIDTH   = 128;
static constexpr uint8_t SCREEN_HEIGHT  = 64;
static constexpr int8_t  OLED_RESET     = -1;
static constexpr uint8_t SCREEN_ADDRESS = 0x3C;

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- ASIGNACIÓN DE PINES (HAL / PINOUT) ---
static constexpr uint8_t PIN_SENSOR_HUMEDAD = A0; // Entrada Analógica (ADC)
static constexpr uint8_t PIN_DRIVER_BOMBA   = 3;  // Salida Digital (Gate MOSFET)
static constexpr uint8_t PIN_FLOTADOR_AGUA  = 4;  // Entrada Digital (Pull-up interno)
static constexpr uint8_t PIN_BUZZER_PASIVO  = 5;  // Salida PWM / Frecuencia (Buzzer pasivo)
static constexpr uint8_t PIN_BOTON_MANUAL   = 2;  // Entrada Digital (Pull-up interno)

// --- PARÁMETROS DE CALIBRACIÓN ADC (docs/04_CALIBRACION.md) ---
static constexpr uint16_t CALIBRACION_AIRE_ADC = 580; // Aire Seco (0% Humedad)
static constexpr uint16_t CALIBRACION_AGUA_ADC = 270; // Agua Limpia (100% Humedad)

// --- PARÁMETROS DE HISTÉRESIS DE RIEGO (%) ---
static constexpr uint8_t UMBRAL_RIEGO_INICIO_PCT = 20; // Disparo automático
static constexpr uint8_t UMBRAL_RIEGO_FIN_PCT    = 70; // Corte por humedad óptima

// --- TEMPORIZACIONES Y SEGURIDAD CRÍTICA (Milisegundos) ---
static constexpr uint32_t INTERVALO_MUESTREO_MS   = 1000;  // 1 Hz de muestreo
static constexpr uint32_t TIEMPO_MAXIMO_BOMBEO_MS = 25000; // Watchdog anti-inundación
static constexpr uint32_t TIEMPO_RIEGO_MANUAL_MS  = 3000;  // Ráfaga manual
static constexpr uint32_t INTERVALO_ALERTA_BEEP_MS= 6000;  // Periodo entre alertas sonoras

// --- FRECUENCIAS ACÚSTICAS (Hz) ---
static constexpr uint16_t FREQ_TONO_SUAVE_HZ = 850;  // Tono grave para adultos mayores
static constexpr uint16_t FREQ_TONO_BEEP_HZ  = 1200; // Confirmación de pulsador

// --- MÁQUINA DE ESTADOS FINITA (FSM) ---
enum class SystemState : uint8_t {
    MONITORING,
    WATERING_AUTO,
    WATERING_MANUAL,
    ALERT_NO_WATER
};

// --- ESTRUCTURA DE CONTROL DEL SISTEMA (ENCAPSULAMIENTO DE ESTADO) ---
struct SystemContext {
    SystemState currentState        = SystemState::MONITORING;
    uint32_t    lastSampleTimeMs    = 0;
    uint32_t    pumpStartTimeMs     = 0;
    uint32_t    lastBeepAlertTimeMs = 0;
    uint8_t     currentMoisturePct  = 50;
    bool        waterLevelOk        = true;
};

static SystemContext sys;

// ============================================================================
// SERVICIO DE PROCESAMIENTO DE SEÑALES (FILTRO DE SOBREMUESTREO)
// ============================================================================
static uint8_t readFilteredMoisturePercent() {
    uint32_t sampleAccumulator = 0;
    static constexpr uint8_t SAMPLE_COUNT = 16;

    for (uint8_t i = 0; i < SAMPLE_COUNT; ++i) {
        sampleAccumulator += analogRead(PIN_SENSOR_HUMEDAD);
        delayMicroseconds(250);
    }

    uint16_t adcAverage = static_cast<uint16_t>(sampleAccumulator / SAMPLE_COUNT);

    // Mapeo inverso: a mayor voltaje, menor humedad relativa
    int32_t percent = map(adcAverage, CALIBRACION_AIRE_ADC, CALIBRACION_AGUA_ADC, 0, 100);
    return static_cast<uint8_t>(constrain(percent, 0, 100));
}

// ============================================================================
// CAPA DE PRESENTACIÓN: RENDERIZADO EN PANTALLA OLED SSD1306
// ============================================================================
static void renderDisplay(uint8_t moisture, const __FlashStringHelper* statusLabel, bool waterOk) {
    display.clearDisplay();

    // Barra de título internacional
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(8, 2);
    display.print(F("SMART FLORACARE PRO"));
    display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

    // Porcentaje de humedad en tipografía grande (legible para adultos mayores)
    display.setTextSize(3);
    display.setCursor(14, 18);
    display.print(moisture);
    display.print(F("%"));

    // Indicador de nivel de agua en depósito
    display.setTextSize(1);
    display.setCursor(82, 24);
    if (waterOk) {
        display.print(F("[AGUA]"));
    } else {
        display.print(F("[VACIO]"));
    }

    // Barra de progreso visual horizontal
    uint8_t barWidth = static_cast<uint8_t>(map(moisture, 0, 100, 0, 120));
    display.drawRect(4, 46, 120, 6, SSD1306_WHITE);
    display.fillRect(4, 46, barWidth, 6, SSD1306_WHITE);

    // Telemetría de estado actual
    display.setCursor(6, 55);
    display.print(F("ESTADO: "));
    display.print(statusLabel);

    display.display();
}

// ============================================================================
// CONTROLADORES DE ACTUADORES
// ============================================================================
static inline void setPumpState(bool active) {
    digitalWrite(PIN_DRIVER_BOMBA, active ? HIGH : LOW);
}

static void triggerSoftAlertTone() {
    tone(PIN_BUZZER_PASIVO, FREQ_TONO_SUAVE_HZ, 80);
}

static void triggerConfirmationTone() {
    tone(PIN_BUZZER_PASIVO, FREQ_TONO_BEEP_HZ, 50);
}

// ============================================================================
// CONFIGURACIÓN INICIAL (SETUP)
// ============================================================================
void setup() {
    Serial.begin(9600);

    // Configuración segura de actuadores
    pinMode(PIN_DRIVER_BOMBA, OUTPUT);
    setPumpState(false);

    pinMode(PIN_BUZZER_PASIVO, OUTPUT);
    noTone(PIN_BUZZER_PASIVO);

    // Entradas digitales con resistencias internas pull-up
    pinMode(PIN_FLOTADOR_AGUA, INPUT_PULLUP);
    pinMode(PIN_BOTON_MANUAL, INPUT_PULLUP);

    // Inicialización del bus I2C y pantalla
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("[ERROR] Falla al inicializar OLED SSD1306"));
        while (true) {
            // Failsafe visual por puerto serie
        }
    }

    // Pantalla de bienvenida
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(16, 20);
    display.println(F("INICIALIZANDO..."));
    display.setCursor(20, 35);
    display.println(F("FLORACARE SYSTEM"));
    display.display();
    delay(1000);

    // Lectura inicial de estabilización
    sys.currentMoisturePct = readFilteredMoisturePercent();
    sys.waterLevelOk = (digitalRead(PIN_FLOTADOR_AGUA) == LOW);
}

// ============================================================================
// LAZO PRINCIPAL DE CONTROL (NON-BLOCKING DETERMINISTIC LOOP)
// ============================================================================
void loop() {
    const uint32_t nowMs = millis();

    // 1. TAREA PERIÓDICA: Muestreo de Sensores (1 Hz)
    if ((uint32_t)(nowMs - sys.lastSampleTimeMs) >= INTERVALO_MUESTREO_MS) {
        sys.lastSampleTimeMs = nowMs;
        sys.currentMoisturePct = readFilteredMoisturePercent();

        // En sensor de flotador cerrado a masa con agua: LOW = Agua suficiente, HIGH = Vacío
        sys.waterLevelOk = (digitalRead(PIN_FLOTADOR_AGUA) == LOW);
    }

    // 2. ENCLAVAMIENTO DE SEGURIDAD CRÍTICO: Verificación de Nivel de Agua
    if (!sys.waterLevelOk) {
        setPumpState(false); // Corte inmediato incondicional
        sys.currentState = SystemState::ALERT_NO_WATER;
        renderDisplay(sys.currentMoisturePct, F("RELLENAR AGUA"), false);

        // Alerta sonora no intrusiva cada 6 segundos
        if ((uint32_t)(nowMs - sys.lastBeepAlertTimeMs) >= INTERVALO_ALERTA_BEEP_MS) {
            sys.lastBeepAlertTimeMs = nowMs;
            triggerSoftAlertTone();
        }
        return; // Detiene la máquina de estados hasta reponer el líquido
    }

    // 3. ENTRADA DE USUARIO: Botón de Riego Manual
    if (digitalRead(PIN_BOTON_MANUAL) == LOW && sys.currentState == SystemState::MONITORING) {
        triggerConfirmationTone();
        sys.pumpStartTimeMs = nowMs;
        sys.currentState = SystemState::WATERING_MANUAL;
        setPumpState(true);
    }

    // 4. MÁQUINA DE ESTADOS FINITA (FSM)
    switch (sys.currentState) {
        case SystemState::MONITORING: {
            setPumpState(false);
            const __FlashStringHelper* statusLabel = (sys.currentMoisturePct < UMBRAL_RIEGO_INICIO_PCT) 
                                                    ? F("SUELO SECO") 
                                                    : F("OPTIMO");
            renderDisplay(sys.currentMoisturePct, statusLabel, true);

            // Transición automática al detectar suelo seco
            if (sys.currentMoisturePct <= UMBRAL_RIEGO_INICIO_PCT) {
                sys.pumpStartTimeMs = nowMs;
                sys.currentState = SystemState::WATERING_AUTO;
                setPumpState(true);
            }
            break;
        }

        case SystemState::WATERING_AUTO: {
            renderDisplay(sys.currentMoisturePct, F("REGANDO AUTO"), true);

            // Condiciones de parada: Meta alcanzada o Watchdog de seguridad activado
            const bool targetReached  = (sys.currentMoisturePct >= UMBRAL_RIEGO_FIN_PCT);
            const bool timeoutExpired = ((uint32_t)(nowMs - sys.pumpStartTimeMs) >= TIEMPO_MAXIMO_BOMBEO_MS);

            if (targetReached || timeoutExpired) {
                setPumpState(false);
                sys.currentState = SystemState::MONITORING;
            }
            break;
        }

        case SystemState::WATERING_MANUAL: {
            renderDisplay(sys.currentMoisturePct, F("RIEGO MANUAL"), true);

            // Pulso acotado de 3 segundos
            if ((uint32_t)(nowMs - sys.pumpStartTimeMs) >= TIEMPO_RIEGO_MANUAL_MS) {
                setPumpState(false);
                sys.currentState = SystemState::MONITORING;
            }
            break;
        }

        case SystemState::ALERT_NO_WATER: {
            // Auto-recuperación cuando el sensor detecta nivel de agua restablecido
            if (sys.waterLevelOk) {
                noTone(PIN_BUZZER_PASIVO);
                sys.currentState = SystemState::MONITORING;
            }
            break;
        }
    }
}
