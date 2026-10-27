/**
 * ============================================================================
 * SMART FLORACARE - NATIVE C++ CONTROLLER VERIFICATION SUITE
 * ============================================================================
 * Archivo: tests/test_native_controller.cpp
 * Descripción: Ejecutable nativo C++17 que compila y valida directamente
 *              include/IrrigationController.h e include/SensorValidator.h
 *              (sin mocks en Python).
 *
 * Modos de compilación verificados en CI:
 *   1. Producción (por defecto):
 *      g++ -std=c++17 -Wall -Wextra -Werror -I include tests/test_native_controller.cpp -o build_test_prod
 *      Verifica el corte operativo automático a 20.000 ms con
 *      StopReason::AUTO_MAX_DURATION_REACHED.
 *
 *   2. Ensayo aislado de Watchdog de 25 s (-DFLORACARE_BENCH_TEST_WATCHDOG_25S):
 *      g++ -std=c++17 -Wall -Wextra -Werror -DFLORACARE_BENCH_TEST_WATCHDOG_25S -I include tests/test_native_controller.cpp -o build_test_wdt25s
 *      Verifica que el controlador C++ real permanece en WATERING a 20.000 ms
 *      y 24.999 ms, y corta exactamente a 25.000 ms con StopReason::SAFETY_TIMEOUT.
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "IrrigationController.h"
#include "SensorValidator.h"

using namespace FloraCare;

#define ASSERT_TRUE(cond, msg)                                                      \
    do {                                                                            \
        if (!(cond)) {                                                              \
            fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, (msg));       \
            return 1;                                                               \
        }                                                                           \
    } while (0)

static int test_common_safety_invariants() {
    const ControllerConfig cfg = makeDefaultControllerConfig(20, 70);
    IrrigationController ctrl(cfg);

    // 1. Estado inicial seguro (apagado)
    ASSERT_TRUE(ctrl.getState() == ControllerState::IDLE, "Initial state must be IDLE");
    ASSERT_TRUE(!ctrl.isPumpActive(), "Pump must be inactive at boot");

    // 2. Ráfaga manual de 3.000 ms
    ASSERT_TRUE(ctrl.requestWatering(IrrigationSource::MANUAL_BUTTON, 1000U, true, SensorHealth::OK, 50),
                "Manual watering request must succeed");
    ASSERT_TRUE(ctrl.update(3999U, true, SensorHealth::OK, 50), "Manual burst must stay active at 2999 ms");
    ASSERT_TRUE(!ctrl.update(4000U, true, SensorHealth::OK, 50), "Manual burst must stop at 3000 ms");
    ASSERT_TRUE(ctrl.getState() == ControllerState::IDLE, "State after manual burst must be IDLE");
    ASSERT_TRUE(ctrl.getLastStopReason() == StopReason::MANUAL_BURST_COMPLETE,
                "StopReason must be MANUAL_BURST_COMPLETE");

    // 3. Enclavamiento inmediato por falta de agua (WATER_DEPLETED)
    ASSERT_TRUE(ctrl.requestWatering(IrrigationSource::REMOTE_WEB, 10000U, true, SensorHealth::OK, 50),
                "Remote watering after cooldown must succeed");
    ASSERT_TRUE(!ctrl.update(11000U, false, SensorHealth::OK, 50),
                "Float switch open must immediately stop pump");
    ASSERT_TRUE(ctrl.getState() == ControllerState::FAULT, "State must be FAULT on water depletion");
    ASSERT_TRUE(ctrl.getLastFault() == FaultReason::NO_WATER_LEVEL, "FaultReason must be NO_WATER_LEVEL");
    ASSERT_TRUE(ctrl.getLastStopReason() == StopReason::WATER_DEPLETED, "StopReason must be WATER_DEPLETED");

    // 4. Recuperación condicionada ante vaciado (H-01 Opción A)
    ASSERT_TRUE(!ctrl.attemptRecovery(12000U, false, SensorHealth::OK),
                "Recovery must fail while tank is still empty");
    ASSERT_TRUE(ctrl.attemptRecovery(12500U, true, SensorHealth::OK),
                "Recovery must succeed once tank level is restored");
    ASSERT_TRUE(ctrl.getState() == ControllerState::IDLE, "State must return to IDLE after recovery");

    // 5. Verificación de rearme explícito vs auto-recuperación ante MAX_TIMEOUT_EXCEEDED (H-01 Req 5)
    // Forzar fallo de tiempo máximo (avanzando más allá del límite absoluto de seguridad en cualquier modo)
    const uint32_t tStartTimeout = 20000U;
    ASSERT_TRUE(ctrl.requestWatering(IrrigationSource::AUTO_CYCLE, tStartTimeout, true, SensorHealth::OK, 10),
                "Watering after cooldown must start");
    const uint32_t tAfterTimeout = tStartTimeout + cfg.maxSafetyWatchdogMs + 1000U;
    ASSERT_TRUE(!ctrl.update(tAfterTimeout, true, SensorHealth::OK, 10),
                "Watering must stop on max timeout");
    ASSERT_TRUE(ctrl.getState() == ControllerState::FAULT, "State must be FAULT");
    ASSERT_TRUE(ctrl.getLastFault() == FaultReason::MAX_TIMEOUT_EXCEEDED, "Fault must be MAX_TIMEOUT_EXCEEDED");

    // attemptRecovery() NO debe borrar MAX_TIMEOUT_EXCEEDED automáticamente
    ASSERT_TRUE(!ctrl.attemptRecovery(tAfterTimeout + 1000U, true, SensorHealth::OK),
                "attemptRecovery must NEVER auto-clear MAX_TIMEOUT_EXCEEDED");
    ASSERT_TRUE(ctrl.getState() == ControllerState::FAULT, "State must remain FAULT");

    // resetFault() (procedimiento explícito de rearme) sí lo desbloquea si las condiciones son seguras
    ASSERT_TRUE(!ctrl.resetFault(tAfterTimeout + 2000U, false, SensorHealth::OK),
                "resetFault must reject rearme if water is missing");
    ASSERT_TRUE(ctrl.resetFault(tAfterTimeout + 3000U, true, SensorHealth::OK),
                "resetFault must succeed when physical conditions are valid");
    ASSERT_TRUE(ctrl.getState() == ControllerState::IDLE, "State must return to IDLE after resetFault");
    ASSERT_TRUE(ctrl.getLastFault() == FaultReason::NONE, "Fault must be cleared after resetFault");

    // 6. Verificación unitaria de FloatSwitchDebouncer (REQ-07, ACT-05.3a, H-03)
    FloatSwitchDebouncer debouncer(true, 15U, 3U, 10U);
    ASSERT_TRUE(debouncer.isWaterOk(), "Initial state must be water OK");

    // Pulso espurio/glitch de 15 ms no debe cambiar el estado
    ASSERT_TRUE(debouncer.update(false, 100U), "Glitch sample 1 must be filtered");
    ASSERT_TRUE(debouncer.update(true, 115U), "Return to true resets candidate counter");
    ASSERT_TRUE(debouncer.isWaterOk(), "Water level must remain OK after 15ms glitch");

    // 3 muestras consecutivas en false (45 ms <= 60 ms) confirman vaciado
    debouncer.update(false, 200U); // Muestra 1
    debouncer.update(false, 215U); // Muestra 2
    bool emptyConfirmed = debouncer.update(false, 230U); // Muestra 3 (t = 45 ms)
    ASSERT_TRUE(!emptyConfirmed, "Debouncer must transition to empty on 3rd sample (45 ms)");
    ASSERT_TRUE(!debouncer.isWaterOk(), "Debouncer state must be false");

    // Oleaje de llenado: 5 muestras en true (75 ms < 150 ms) no son suficientes
    for (uint32_t t = 300U; t < 375U; t += 15U) {
        ASSERT_TRUE(!debouncer.update(true, t), "Transient sloshing < 150 ms must not declare water OK");
    }

    // 10 muestras consecutivas (150 ms) confirman restablecimiento seguro
    for (uint32_t t = 400U; t < 535U; t += 15U) {
        debouncer.update(true, t);
    }
    bool waterRestored = debouncer.update(true, 550U); // 10ª muestra
    ASSERT_TRUE(waterRestored, "10 consecutive samples (150 ms) must restore water OK");
    ASSERT_TRUE(debouncer.isWaterOk(), "Debouncer state must be restored to true");

    return 0;
}

#if !defined(FLORACARE_BENCH_TEST_WATCHDOG_25S)
static int test_production_20s_auto_limit_cpp() {
    // Configuración de producción Nano (20% start) y ESP32 (25% start)
    const ControllerConfig cfg = makeDefaultControllerConfig(20, 70);
    ASSERT_TRUE(cfg.maxAutoDurationMs == 20000U, "Production maxAutoDurationMs must be 20000 ms");
    ASSERT_TRUE(cfg.maxSafetyWatchdogMs == 25000U, "Production maxSafetyWatchdogMs must be 25000 ms");

    IrrigationController ctrl(cfg);

    // Iniciar ciclo automático en t = 1000 ms
    ASSERT_TRUE(ctrl.requestWatering(IrrigationSource::AUTO_CYCLE, 1000U, true, SensorHealth::OK, 15),
                "AUTO_CYCLE must start when moisture <= 20%");
    ASSERT_TRUE(ctrl.getState() == ControllerState::WATERING, "State must be WATERING");

    // En t = 20999 ms (elapsed = 19999 ms), sigue regando
    ASSERT_TRUE(ctrl.update(20999U, true, SensorHealth::OK, 30),
                "Production AUTO_CYCLE must remain active at 19999 ms");
    ASSERT_TRUE(ctrl.getState() == ControllerState::WATERING, "State must still be WATERING at 19999 ms");

    // En t = 21000 ms (elapsed = 20000 ms), debe cortar por AUTO_MAX_DURATION_REACHED
    ASSERT_TRUE(!ctrl.update(21000U, true, SensorHealth::OK, 30),
                "Production AUTO_CYCLE must stop at 20000 ms");
    ASSERT_TRUE(!ctrl.isPumpActive(), "Pump must be inactive at 20000 ms");
    ASSERT_TRUE(ctrl.getState() == ControllerState::FAULT,
                "Controller must enter FAULT on 20000 ms auto timeout");
    ASSERT_TRUE(ctrl.getLastFault() == FaultReason::MAX_TIMEOUT_EXCEEDED,
                "FaultReason must be MAX_TIMEOUT_EXCEEDED");
    ASSERT_TRUE(ctrl.getLastStopReason() == StopReason::AUTO_MAX_DURATION_REACHED,
                "StopReason must be AUTO_MAX_DURATION_REACHED (20 s), distinct from SAFETY_TIMEOUT (25 s)");

    // Verificar aritmética modular unsigned de 32 bits en desbordamiento de millis()
    IrrigationController wrapCtrl(cfg);
    const uint32_t tStart = 0xFFFFFF00U; // 256 ms antes de overflow
    ASSERT_TRUE(wrapCtrl.requestWatering(IrrigationSource::AUTO_CYCLE, tStart, true, SensorHealth::OK, 15),
                "Wraparound AUTO_CYCLE must start");
    const uint32_t tAt19999 = tStart + 19999U;
    const uint32_t tAt20000 = tStart + 20000U;
    ASSERT_TRUE(wrapCtrl.update(tAt19999, true, SensorHealth::OK, 25),
                "Wraparound AUTO_CYCLE must remain active at 19999 ms");
    ASSERT_TRUE(!wrapCtrl.update(tAt20000, true, SensorHealth::OK, 25),
                "Wraparound AUTO_CYCLE must stop at 20000 ms");
    ASSERT_TRUE(wrapCtrl.getLastStopReason() == StopReason::AUTO_MAX_DURATION_REACHED,
                "Wraparound StopReason must be AUTO_MAX_DURATION_REACHED");

    printf("[PASS] Native C++ Production Mode: 20000 ms -> StopReason::AUTO_MAX_DURATION_REACHED verified.\n");
    return 0;
}
#else
static int test_isolated_25s_watchdog_limit_cpp() {
    // Configuración compilada con -DFLORACARE_BENCH_TEST_WATCHDOG_25S
    const ControllerConfig cfg = makeDefaultControllerConfig(20, 70);
    ASSERT_TRUE(cfg.maxAutoDurationMs == 30000U,
                "Bench-test maxAutoDurationMs must be 30000 ms under FLORACARE_BENCH_TEST_WATCHDOG_25S");
    ASSERT_TRUE(cfg.maxSafetyWatchdogMs == 25000U,
                "Bench-test maxSafetyWatchdogMs must remain 25000 ms");

    IrrigationController ctrl(cfg);

    // Iniciar ciclo automático en t = 1000 ms
    ASSERT_TRUE(ctrl.requestWatering(IrrigationSource::AUTO_CYCLE, 1000U, true, SensorHealth::OK, 15),
                "AUTO_CYCLE must start in isolated watchdog test");

    // En t = 21000 ms (elapsed = 20000 ms), NO debe cortar porque maxAutoDurationMs = 30000 ms
    ASSERT_TRUE(ctrl.update(21000U, true, SensorHealth::OK, 30),
                "Isolated watchdog test must stay active at 20000 ms");
    ASSERT_TRUE(ctrl.getState() == ControllerState::WATERING,
                "State must remain WATERING at 20000 ms");

    // En t = 25999 ms (elapsed = 24999 ms), sigue regando
    ASSERT_TRUE(ctrl.update(25999U, true, SensorHealth::OK, 30),
                "Isolated watchdog test must stay active at 24999 ms");

    // En t = 26000 ms (elapsed = 25000 ms), el watchdog maestro corta con SAFETY_TIMEOUT
    ASSERT_TRUE(!ctrl.update(26000U, true, SensorHealth::OK, 30),
                "Master safety watchdog must cut at 25000 ms");
    ASSERT_TRUE(!ctrl.isPumpActive(), "Pump must be inactive at 25000 ms");
    ASSERT_TRUE(ctrl.getState() == ControllerState::FAULT,
                "Controller must enter FAULT at 25000 ms");
    ASSERT_TRUE(ctrl.getLastFault() == FaultReason::MAX_TIMEOUT_EXCEEDED,
                "FaultReason must be MAX_TIMEOUT_EXCEEDED");
    ASSERT_TRUE(ctrl.getLastStopReason() == StopReason::SAFETY_TIMEOUT,
                "StopReason must be SAFETY_TIMEOUT (25 s master watchdog)");

    // Rechazar nuevas solicitudes mientras permanezca en FAULT
    ASSERT_TRUE(!ctrl.requestWatering(IrrigationSource::MANUAL_BUTTON, 35000U, true, SensorHealth::OK, 15),
                "Requests must be blocked while in FAULT state");

    printf("[PASS] Native C++ Isolated Watchdog Mode (-DFLORACARE_BENCH_TEST_WATCHDOG_25S): 25000 ms -> StopReason::SAFETY_TIMEOUT verified.\n");
    return 0;
}
#endif

int main() {
    if (test_common_safety_invariants() != 0) {
        return 1;
    }
#if !defined(FLORACARE_BENCH_TEST_WATCHDOG_25S)
    if (test_production_20s_auto_limit_cpp() != 0) {
        return 1;
    }
#else
    if (test_isolated_25s_watchdog_limit_cpp() != 0) {
        return 1;
    }
#endif
    return 0;
}
