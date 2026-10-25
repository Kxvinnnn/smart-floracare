/**
 * ============================================================================
 * SMART FLORACARE - CENTRALIZED IRRIGATION CONTROLLER
 * ============================================================================
 * Archivo: IrrigationController.h
 * Descripción: Controlador centralizado de riego (FSM de dominio).
 *              Centraliza la toma de decisiones para solicitudes automáticas,
 *              manuales y remotas (web), aplicando límites inmutables,
 *              enclavamientos de seguridad y parada idempotente.
 * ============================================================================
 */

#ifndef IRRIGATION_CONTROLLER_H
#define IRRIGATION_CONTROLLER_H

#include <stdint.h>
#include "SensorValidator.h"

namespace FloraCare {

enum class ControllerState : uint8_t {
    IDLE,        // Inactivo / Monitoreando en reposo
    VALIDATING,  // Validación de condiciones previas al encendido
    WATERING,    // Bomba encendida activamente
    STOPPED,     // Estado de parada transitorio (idempotente)
    FAULT,       // Fallo de seguridad (corte incondicional)
    RECOVERY     // Proceso explícito de recuperación tras fallo
};

enum class IrrigationSource : uint8_t {
    NONE,
    AUTO_CYCLE,
    MANUAL_BUTTON,
    REMOTE_WEB
};

enum class FaultReason : uint8_t {
    NONE,
    NO_WATER_LEVEL,
    MAX_TIMEOUT_EXCEEDED,
    SENSOR_MALFUNCTION,
    INVALID_REQUEST
};

enum class StopReason : uint8_t {
    NORMAL_TARGET_REACHED,
    MANUAL_BURST_COMPLETE,
    SAFETY_TIMEOUT,
    WATER_DEPLETED,
    USER_INTERRUPT,
    SENSOR_FAULT
};

struct ControllerConfig {
    uint32_t maxAutoDurationMs;    // Tiempo máx para riego automático (ej. 20000 ms)
    uint32_t manualBurstDurationMs;// Tiempo para riego manual/web (ej. 3000 ms)
    uint32_t maxSafetyWatchdogMs;  // Límite físico absoluto e inmutable (ej. 25000 ms)
    uint32_t cooldownPeriodMs;     // Periodo mínimo entre riegos (ej. 5000 ms)
    uint8_t  autoStartMoisturePct; // Umbral de inicio auto (ej. 20%)
    uint8_t  autoStopMoisturePct;  // Umbral de corte auto (ej. 70%)
};

class IrrigationController {
public:
    explicit IrrigationController(const ControllerConfig& cfg)
        : config_(cfg),
          state_(ControllerState::IDLE),
          activeSource_(IrrigationSource::NONE),
          lastFault_(FaultReason::NONE),
          lastStopReason_(StopReason::NORMAL_TARGET_REACHED),
          pumpStartTimeMs_(0),
          lastStopMs_(0) {}

    // --- CONSULTAS DE ESTADO ---
    ControllerState getState() const { return state_; }
    bool isPumpActive() const { return state_ == ControllerState::WATERING; }
    FaultReason getLastFault() const { return lastFault_; }
    StopReason getLastStopReason() const { return lastStopReason_; }
    IrrigationSource getActiveSource() const { return activeSource_; }

    /**
     * Parada idempotente: se puede invocar múltiples veces sin inconsistencias.
     * Si la bomba ya está apagada, no produce efectos colaterales.
     */
    void stop(StopReason reason, uint32_t nowMs) {
        if (state_ == ControllerState::WATERING || state_ == ControllerState::VALIDATING) {
            state_ = ControllerState::STOPPED;
            lastStopReason_ = reason;
            lastStopMs_ = nowMs;
            activeSource_ = IrrigationSource::NONE;

            // Transición a reposo o fallo según la causa
            if (reason == StopReason::SAFETY_TIMEOUT) {
                lastFault_ = FaultReason::MAX_TIMEOUT_EXCEEDED;
                state_ = ControllerState::FAULT;
            } else if (reason == StopReason::WATER_DEPLETED) {
                lastFault_ = FaultReason::NO_WATER_LEVEL;
                state_ = ControllerState::FAULT;
            } else if (reason == StopReason::SENSOR_FAULT) {
                lastFault_ = FaultReason::SENSOR_MALFUNCTION;
                state_ = ControllerState::FAULT;
            } else {
                state_ = ControllerState::IDLE;
            }
        }
    }

    /**
     * Solicitud centralizada de riego. Toda ruta (automática, botón físico, web)
     * DEBE invocar esta función. Ninguna interfaz puede activar la bomba directamente.
     */
    bool requestWatering(IrrigationSource source,
                         uint32_t nowMs,
                         bool waterLevelOk,
                         SensorHealth sensorHealth,
                         uint8_t currentMoisturePct) {
        // Regla 1: En estado de fallo, se rechaza cualquier solicitud
        if (state_ == ControllerState::FAULT) {
            return false;
        }

        // Regla 2: Si ya está regando, se rechaza para evitar reiniciar el temporizador
        if (state_ == ControllerState::WATERING || state_ == ControllerState::VALIDATING) {
            return false;
        }

        // Regla 3: Si no hay agua física en el depósito, corte e ingreso a fallo
        if (!waterLevelOk) {
            lastFault_ = FaultReason::NO_WATER_LEVEL;
            state_ = ControllerState::FAULT;
            return false;
        }

        // Regla 4: El sensor debe reportar salud OK para el modo automático
        if (source == IrrigationSource::AUTO_CYCLE && sensorHealth != SensorHealth::OK) {
            lastFault_ = FaultReason::SENSOR_MALFUNCTION;
            state_ = ControllerState::FAULT;
            return false;
        }

        // Regla 5: Enfriamiento obligatorio entre ciclos (anti-inundación)
        if (lastStopMs_ > 0 && (static_cast<uint32_t>(nowMs - lastStopMs_) < config_.cooldownPeriodMs)) {
            return false;
        }

        // Regla 6: Validación de umbral en modo automático
        if (source == IrrigationSource::AUTO_CYCLE && currentMoisturePct > config_.autoStartMoisturePct) {
            return false;
        }

        // Transición: VALIDATING -> WATERING
        state_ = ControllerState::VALIDATING;
        activeSource_ = source;
        pumpStartTimeMs_ = nowMs;
        state_ = ControllerState::WATERING;
        return true;
    }

    /**
     * Ciclo periódico de actualización (evaluación de condiciones de corte y seguridad).
     * Retorna el estado físico requerido para la salida del actuador (true = encendido).
     */
    bool update(uint32_t nowMs,
                bool waterLevelOk,
                SensorHealth sensorHealth,
                uint8_t currentMoisturePct) {
        // Enclavamiento inmediato: si el flotador se abre, corte sin importar el modo
        if (!waterLevelOk) {
            if (state_ == ControllerState::WATERING) {
                stop(StopReason::WATER_DEPLETED, nowMs);
            } else if (state_ != ControllerState::FAULT) {
                lastFault_ = FaultReason::NO_WATER_LEVEL;
                state_ = ControllerState::FAULT;
            }
            return false;
        }

        // Si estamos en riego activo, verificar límites temporales y condiciones de fin
        if (state_ == ControllerState::WATERING) {
            const uint32_t elapsedMs = static_cast<uint32_t>(nowMs - pumpStartTimeMs_);

            // Límite físico absoluto (watchdog de seguridad global)
            if (elapsedMs >= config_.maxSafetyWatchdogMs) {
                stop(StopReason::SAFETY_TIMEOUT, nowMs);
                return false;
            }

            // Límite por modalidad
            if (activeSource_ == IrrigationSource::MANUAL_BUTTON || activeSource_ == IrrigationSource::REMOTE_WEB) {
                if (elapsedMs >= config_.manualBurstDurationMs) {
                    stop(StopReason::MANUAL_BURST_COMPLETE, nowMs);
                    return false;
                }
            } else if (activeSource_ == IrrigationSource::AUTO_CYCLE) {
                if (elapsedMs >= config_.maxAutoDurationMs) {
                    stop(StopReason::SAFETY_TIMEOUT, nowMs);
                    return false;
                }
                // Si el sensor falla durante el riego, detener inmediatamente
                if (sensorHealth != SensorHealth::OK) {
                    stop(StopReason::SENSOR_FAULT, nowMs);
                    return false;
                }
                // Meta alcanzada
                if (currentMoisturePct >= config_.autoStopMoisturePct) {
                    stop(StopReason::NORMAL_TARGET_REACHED, nowMs);
                    return false;
                }
            }
            return true;
        }

        return false;
    }

    /**
     * Intento explícito de recuperación tras un estado de fallo.
     * Requiere que la condición causante esté físicamente normalizada.
     */
    bool attemptRecovery(uint32_t nowMs, bool waterLevelOk, SensorHealth sensorHealth) {
        if (state_ != ControllerState::FAULT) {
            return true;
        }

        // Transición a RECOVERY
        state_ = ControllerState::RECOVERY;

        if (lastFault_ == FaultReason::NO_WATER_LEVEL && !waterLevelOk) {
            state_ = ControllerState::FAULT; // No se ha rellenado el agua
            return false;
        }

        if (lastFault_ == FaultReason::SENSOR_MALFUNCTION && sensorHealth != SensorHealth::OK) {
            state_ = ControllerState::FAULT; // El sensor sigue dañado/desconectado
            return false;
        }

        // Recuperación exitosa
        lastFault_ = FaultReason::NONE;
        lastStopMs_ = nowMs;
        state_ = ControllerState::IDLE;
        return true;
    }

private:
    ControllerConfig config_;
    ControllerState  state_;
    IrrigationSource activeSource_;
    FaultReason      lastFault_;
    StopReason       lastStopReason_;
    uint32_t         pumpStartTimeMs_;
    uint32_t         lastStopMs_;
};

} // namespace FloraCare

#endif // IRRIGATION_CONTROLLER_H
