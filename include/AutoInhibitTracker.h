/**
 * ============================================================================
 * SMART FLORACARE - DOMAIN LOGIC: AUTO-INHIBITION TRACKER
 * ============================================================================
 * Archivo: include/AutoInhibitTracker.h
 * Descripción: Gestor desacoplado de interbloqueo preventivo de autorriego
 *              desatendido tras disparo de temporizadores de seguridad (H-156).
 *
 * Política de Seguridad y Rearme Supervisado:
 *   1. Tras una parada por límite máximo de bombeo automático (20 s) o watchdog
 *      maestro (25 s), el controlador entra en FAULT y el inhibidor se enclava
 *      (isInhibited() == true).
 *   2. Un rearme manual o remoto (resetFault) devuelve la FSM a IDLE, pero el
 *      inhibidor PERMANECE enclavado para evitar que el sistema vuelva a bombear
 *      de forma autónoma y desatendida.
 *   3. Liberación mandatoria: La inhibición se desbloquea ÚNICAMENTE si:
 *      a) La humedad de suelo se normaliza físicamente (moisture > threshold).
 *      b) Se completa exitosamente una ráfaga manual LOCAL presencial
 *         (MANUAL_BUTTON) con causa MANUAL_BURST_COMPLETE.
 *      Cualquier solicitud remota (REMOTE_WEB) tiene PROHIBIDO desbloquear la
 *      inhibición autónoma desatendida.
 * ============================================================================
 */

#ifndef AUTO_INHIBIT_TRACKER_H
#define AUTO_INHIBIT_TRACKER_H

#include <stdint.h>
#include "IrrigationController.h"

namespace FloraCare {

class AutoInhibitTracker {
public:
    AutoInhibitTracker()
        : inhibited_(false),
          lastState_(ControllerState::IDLE) {}

    bool isInhibited() const { return inhibited_; }

    void setInhibited(bool val) { inhibited_ = val; }

    /**
     * Actualización síncrona en cada ciclo de la FSM.
     *
     * @param currentState Estado actual retornado por controller.getState()
     * @param lastStopReason Causa de parada retornada por controller.getLastStopReason()
     * @param activeSource Origen del ciclo activo o recién finalizado
     * @param currentMoisturePct Humedad actual del suelo
     * @param autoStartThresholdPct Umbral de inicio automático configurado (ej. 20%)
     */
    void update(ControllerState currentState,
                StopReason lastStopReason,
                IrrigationSource activeSource,
                uint8_t currentMoisturePct,
                uint8_t autoStartThresholdPct) {
        // Normalización física de humedad: el suelo ya no requiere riego
        if (currentMoisturePct > autoStartThresholdPct) {
            inhibited_ = false;
        }

        // Flanco 1: Transición hacia FAULT por disparo de watchdog/límite -> Enclavar
        if (lastState_ == ControllerState::WATERING && currentState == ControllerState::FAULT) {
            if (lastStopReason == StopReason::SAFETY_TIMEOUT ||
                lastStopReason == StopReason::AUTO_MAX_DURATION_REACHED) {
                inhibited_ = true;
            }
        }

        // Flanco 2: Culminación exitosa de ráfaga manual LOCAL supervisada -> Liberar
        // RESTRICCIÓN DE SEGURIDAD (H-156):
        // Exige estrictamente intervención presencial (MANUAL_BUTTON).
        // Las activaciones remotas (REMOTE_WEB) jamás liberan la inhibición autónoma.
        if (lastState_ == ControllerState::WATERING && currentState == ControllerState::IDLE) {
            if (lastStopReason == StopReason::MANUAL_BURST_COMPLETE &&
                activeSource == IrrigationSource::MANUAL_BUTTON) {
                inhibited_ = false;
            }
        }

        lastState_ = currentState;
    }

private:
    bool            inhibited_;
    ControllerState lastState_;
};

} // namespace FloraCare

#endif // AUTO_INHIBIT_TRACKER_H
