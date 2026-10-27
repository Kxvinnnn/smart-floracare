/**
 * ============================================================================
 * SMART FLORACARE - SENSOR VALIDATION & CALIBRATION DOMAIN
 * ============================================================================
 * Archivo: SensorValidator.h
 * Descripción: Funciones puras de validación de rangos ADC, calibración y
 *              conversión determinista sin divisiones por cero ni heap.
 * ============================================================================
 */

#ifndef SENSOR_VALIDATOR_H
#define SENSOR_VALIDATOR_H

#include <stdint.h>

namespace FloraCare {

enum class SensorHealth : uint8_t {
    OK,
    SHORT_CIRCUIT_OR_LOW,
    DISCONNECTED_OR_HIGH,
    INVALID_CALIBRATION
};

struct CalibrationProfile {
    uint16_t airRaw;          // Lectura base seca (0% de escala: aire libre o sustrato seco)
    uint16_t waterRaw;        // Lectura base húmeda (100% de escala: agua líquida o sustrato a capacidad de campo)
    uint16_t minPhysLimit;    // Límite inferior físico admisible del ADC (menor a esto implica corto/masa)
    uint16_t maxPhysLimit;    // Límite superior físico admisible del ADC (mayor a esto implica desconexión)
    uint16_t minDelta;        // Excursión mínima (airRaw - waterRaw) para validar que el sensor no esté degradado
};

class SensorValidator {
public:
    /**
     * Valida la coherencia matemática de la calibración del sensor.
     * En sensores capacitivos v1.2, la tensión en aire seco DEBE ser
     * mayor a la tensión en agua (airRaw > waterRaw).
     * Compatible con estándar estricto C++11 y C++14/17.
     */
    static constexpr bool isCalibrationValid(const CalibrationProfile& prof) {
        return (prof.airRaw > prof.waterRaw) &&
               (static_cast<uint16_t>(prof.airRaw - prof.waterRaw) >= prof.minDelta) &&
               (prof.airRaw <= prof.maxPhysLimit) &&
               (prof.waterRaw >= prof.minPhysLimit);
    }

    /**
     * Diagnostica la integridad física del sensor a partir del valor ADC bruto.
     */
    static SensorHealth evaluateHealth(uint16_t rawAdc, const CalibrationProfile& prof) {
        if (!isCalibrationValid(prof)) {
            return SensorHealth::INVALID_CALIBRATION;
        }
        if (rawAdc < prof.minPhysLimit) {
            return SensorHealth::SHORT_CIRCUIT_OR_LOW;
        }
        if (rawAdc > prof.maxPhysLimit) {
            return SensorHealth::DISCONNECTED_OR_HIGH;
        }
        return SensorHealth::OK;
    }

    /**
     * Convierte de forma segura y delimitada el valor ADC a porcentaje [0 - 100].
     * Si la calibración es inválida o el sensor está roto, retorna -1 y false en outValid.
     */
    static bool calculateMoisturePercent(uint16_t rawAdc,
                                         const CalibrationProfile& prof,
                                         uint8_t& outPercent,
                                         SensorHealth& outHealth) {
        outHealth = evaluateHealth(rawAdc, prof);
        if (outHealth != SensorHealth::OK) {
            outPercent = 0;
            return false;
        }

        const int32_t deltaSpan = static_cast<int32_t>(prof.airRaw) - static_cast<int32_t>(prof.waterRaw);
        if (deltaSpan <= 0) {
            outPercent = 0;
            return false;
        }

        // Mapeo inverso: airRaw -> 0%, waterRaw -> 100%
        int32_t diff = static_cast<int32_t>(prof.airRaw) - static_cast<int32_t>(rawAdc);
        int32_t calculated = (diff * 100) / deltaSpan;

        if (calculated < 0) {
            calculated = 0;
        } else if (calculated > 100) {
            calculated = 100;
        }

        outPercent = static_cast<uint8_t>(calculated);
        return true;
    }
};

/**
 * Antirrebote e integrador temporal para sensor de nivel por boya magnética.
 * Cumple con REQ-07 y ACT-05.3a:
 * - Corte rápido ante vaciado: 3 muestras consecutivas x 15 ms = 45 ms (<= 60 ms).
 * - Confirmación sostenida ante llenado: 10 muestras consecutivas x 15 ms = 150 ms
 *   para evitar oscilaciones falsas por oleaje/salpicaduras durante el rellenado.
 * - Rechaza pulsos espurios y glitches EMI de conmutación < 30 ms.
 */
class FloatSwitchDebouncer {
public:
    explicit FloatSwitchDebouncer(bool initialWaterOk = false,
                                  uint32_t sampleIntervalMs = 15,
                                  uint8_t emptyConfirmSamples = 3,
                                  uint8_t fillConfirmSamples = 10)
        : sampleIntervalMs_(sampleIntervalMs),
          emptyConfirmSamples_(emptyConfirmSamples),
          fillConfirmSamples_(fillConfirmSamples),
          consecutiveReads_(0),
          lastSampleMs_(0),
          candidateReading_(initialWaterOk),
          debouncedWaterOk_(initialWaterOk) {}

    bool update(bool rawReadingWaterOk, uint32_t nowMs) {
        if (lastSampleMs_ > 0 && static_cast<uint32_t>(nowMs - lastSampleMs_) < sampleIntervalMs_) {
            return debouncedWaterOk_;
        }
        lastSampleMs_ = nowMs;

        if (rawReadingWaterOk != debouncedWaterOk_) {
            if (rawReadingWaterOk == candidateReading_) {
                consecutiveReads_++;
            } else {
                candidateReading_ = rawReadingWaterOk;
                consecutiveReads_ = 1;
            }

            const uint8_t requiredSamples = candidateReading_ ? fillConfirmSamples_ : emptyConfirmSamples_;
            if (consecutiveReads_ >= requiredSamples) {
                debouncedWaterOk_ = candidateReading_;
                consecutiveReads_ = 0;
            }
        } else {
            consecutiveReads_ = 0;
            candidateReading_ = debouncedWaterOk_;
        }

        return debouncedWaterOk_;
    }

    bool isWaterOk() const { return debouncedWaterOk_; }

private:
    uint32_t sampleIntervalMs_;
    uint8_t  emptyConfirmSamples_;
    uint8_t  fillConfirmSamples_;
    uint8_t  consecutiveReads_;
    uint32_t lastSampleMs_;
    bool     candidateReading_;
    bool     debouncedWaterOk_;
};

} // namespace FloraCare

#endif // SENSOR_VALIDATOR_H
