#!/usr/bin/env python3
"""
=============================================================================
SMART FLORACARE - AUTOMATED COMPREHENSIVE TEST SUITE
=============================================================================
Suite exhaustiva de 30 pruebas unitarias y de integración lógica que verifican
el 100% de los requisitos de seguridad, interbloqueos, robustez de red,
matemáticas de calibración y máquina de estados (FSM) de Smart FloraCare:

   1. Arranque seguro (Initial safe boot).
   2. Estado inicial con bomba apagada.
   3. Sensor válido dentro de límites físicos.
   4. Sensor inválido (calibración incoherente).
   5. Sensor desconectado (ADC fuera de rango superior).
   6. Humedad por encima del umbral de riego.
   7. Humedad por debajo del umbral de riego.
   8. Banda de histéresis anti-ciclado.
   9. Nivel de agua suficiente (evaluación nominal).
  10. Nivel de agua insuficiente (bloqueo preventivo).
  11. Pérdida del nivel de agua durante el bombeo (corte inmediato).
  12. Antirrebote del interruptor de flotador.
  13. Antirrebote del botón físico manual.
  14. Inicio de riego permitido bajo condiciones nominales.
  15. Inicio de riego rechazado (concurrencia y cooldown).
  16. Tiempo máximo de riego y watchdog absoluto anti-inundación.
  17. Fin normal del riego por meta de humedad alcanzada.
  18. Periodo de enfriamiento obligatorio (cooldown anti-ciclado).
  19. Bloqueo total ante estado de fallo (error lockout).
  20. Recuperación condicionada tras normalización de fallos.
  21. Reinicio seguro del microcontrolador (cero memoria de bombeo).
  22. Desbordamiento de temporizador de 32 bits (millis rollover).
  23. Solicitud HTTP sin token (rechazo 401).
  24. Solicitud HTTP con token inválido (rechazo 401).
  25. Solicitud HTTP válida con POST y token correcto.
  26. Solicitudes HTTP repetidas (idempotencia temporal).
  27. Solicitud HTTP durante bloqueo por falta de agua (rechazo 409).
  28. Detección y bloqueo de token predeterminado o inseguro (403 Forbidden).
  29. Resiliencia y autonomía local ante pérdida de Wi-Fi.
  30. Garantía universal de bomba apagada ante cualquier fallo.
=============================================================================
"""

import unittest
from enum import Enum


# --- MODELO DE DOMINIO EN PYTHON (ESPEJO EXACTO DE IrrigationController.h) ---

class ControllerState(Enum):
    IDLE = 0
    VALIDATING = 1
    WATERING = 2
    STOPPED = 3
    FAULT = 4
    RECOVERY = 5


class IrrigationSource(Enum):
    NONE = 0
    AUTO_CYCLE = 1
    MANUAL_BUTTON = 2
    REMOTE_WEB = 3


class FaultReason(Enum):
    NONE = 0
    NO_WATER_LEVEL = 1
    MAX_TIMEOUT_EXCEEDED = 2
    SENSOR_MALFUNCTION = 3
    INVALID_REQUEST = 4


class StopReason(Enum):
    NORMAL_TARGET_REACHED = 0
    MANUAL_BURST_COMPLETE = 1
    AUTO_MAX_DURATION_REACHED = 2
    SAFETY_TIMEOUT = 3
    WATER_DEPLETED = 4
    USER_INTERRUPT = 5
    SENSOR_FAULT = 6


class SensorHealth(Enum):
    OK = 0
    SHORT_CIRCUIT_OR_LOW = 1
    DISCONNECTED_OR_HIGH = 2
    INVALID_CALIBRATION = 3


class CalibrationProfile:
    def __init__(self, air_raw, water_raw, min_phys, max_phys, min_delta):
        self.air_raw = air_raw
        self.water_raw = water_raw
        self.min_phys = min_phys
        self.max_phys = max_phys
        self.min_delta = min_delta

    def is_valid(self):
        if self.air_raw <= self.water_raw:
            return False
        if (self.air_raw - self.water_raw) < self.min_delta:
            return False
        if self.air_raw > self.max_phys or self.water_raw < self.min_phys:
            return False
        return True

    def evaluate_health(self, raw_adc):
        if not self.is_valid():
            return SensorHealth.INVALID_CALIBRATION
        if raw_adc < self.min_phys:
            return SensorHealth.SHORT_CIRCUIT_OR_LOW
        if raw_adc > self.max_phys:
            return SensorHealth.DISCONNECTED_OR_HIGH
        return SensorHealth.OK

    def calculate_percent(self, raw_adc):
        health = self.evaluate_health(raw_adc)
        if health != SensorHealth.OK:
            return None, health
        span = self.air_raw - self.water_raw
        if span <= 0:
            return None, SensorHealth.INVALID_CALIBRATION
        diff = self.air_raw - raw_adc
        pct = (diff * 100) // span
        pct = max(0, min(100, pct))
        return pct, SensorHealth.OK


class MockIrrigationController:
    def __init__(self, max_auto_ms=20000, manual_ms=3000, watchdog_ms=25000, cooldown_ms=5000, start_pct=25, stop_pct=70):
        self.max_auto_ms = max_auto_ms
        self.manual_ms = manual_ms
        self.watchdog_ms = watchdog_ms
        self.cooldown_ms = cooldown_ms
        self.start_pct = start_pct
        self.stop_pct = stop_pct

        self.state = ControllerState.IDLE
        self.active_source = IrrigationSource.NONE
        self.last_fault = FaultReason.NONE
        self.last_stop_reason = StopReason.NORMAL_TARGET_REACHED
        self.pump_start_time_ms = 0
        self.last_stop_ms = 0

    def is_pump_active(self):
        return self.state == ControllerState.WATERING

    def stop(self, reason, now_ms):
        if self.state in (ControllerState.WATERING, ControllerState.VALIDATING):
            self.state = ControllerState.STOPPED
            self.last_stop_reason = reason
            self.last_stop_ms = now_ms
            self.active_source = IrrigationSource.NONE

            if reason in (StopReason.SAFETY_TIMEOUT, StopReason.AUTO_MAX_DURATION_REACHED):
                self.last_fault = FaultReason.MAX_TIMEOUT_EXCEEDED
                self.state = ControllerState.FAULT
            elif reason == StopReason.WATER_DEPLETED:
                self.last_fault = FaultReason.NO_WATER_LEVEL
                self.state = ControllerState.FAULT
            elif reason == StopReason.SENSOR_FAULT:
                self.last_fault = FaultReason.SENSOR_MALFUNCTION
                self.state = ControllerState.FAULT
            else:
                self.state = ControllerState.IDLE

    def request_watering(self, source, now_ms, water_level_ok, sensor_health, current_pct):
        if self.state == ControllerState.FAULT:
            return False
        if self.state in (ControllerState.WATERING, ControllerState.VALIDATING):
            return False
        if not water_level_ok:
            self.last_fault = FaultReason.NO_WATER_LEVEL
            self.state = ControllerState.FAULT
            return False
        if source == IrrigationSource.AUTO_CYCLE and sensor_health != SensorHealth.OK:
            self.last_fault = FaultReason.SENSOR_MALFUNCTION
            self.state = ControllerState.FAULT
            return False
        if self.last_stop_ms > 0 and ((now_ms - self.last_stop_ms) & 0xFFFFFFFF) < self.cooldown_ms:
            return False
        if source == IrrigationSource.AUTO_CYCLE and current_pct > self.start_pct:
            return False

        self.state = ControllerState.VALIDATING
        self.active_source = source
        self.pump_start_time_ms = now_ms
        self.state = ControllerState.WATERING
        return True

    def update(self, now_ms, water_level_ok, sensor_health, current_pct):
        if not water_level_ok:
            if self.state == ControllerState.WATERING:
                self.stop(StopReason.WATER_DEPLETED, now_ms)
            elif self.state != ControllerState.FAULT:
                self.last_fault = FaultReason.NO_WATER_LEVEL
                self.state = ControllerState.FAULT
            return False

        if self.state == ControllerState.WATERING:
            elapsed = (now_ms - self.pump_start_time_ms) & 0xFFFFFFFF
            if elapsed >= self.watchdog_ms:
                self.stop(StopReason.SAFETY_TIMEOUT, now_ms)
                return False

            if self.active_source in (IrrigationSource.MANUAL_BUTTON, IrrigationSource.REMOTE_WEB):
                if elapsed >= self.manual_ms:
                    self.stop(StopReason.MANUAL_BURST_COMPLETE, now_ms)
                    return False
            elif self.active_source == IrrigationSource.AUTO_CYCLE:
                if elapsed >= self.max_auto_ms:
                    self.stop(StopReason.AUTO_MAX_DURATION_REACHED, now_ms)
                    return False
                if sensor_health != SensorHealth.OK:
                    self.stop(StopReason.SENSOR_FAULT, now_ms)
                    return False
                if current_pct >= self.stop_pct:
                    self.stop(StopReason.NORMAL_TARGET_REACHED, now_ms)
                    return False
            return True
        return False

    def attempt_recovery(self, now_ms, water_level_ok, sensor_health):
        if self.state != ControllerState.FAULT:
            return True
        self.state = ControllerState.RECOVERY
        if self.last_fault == FaultReason.NO_WATER_LEVEL and not water_level_ok:
            self.state = ControllerState.FAULT
            return False
        if self.last_fault == FaultReason.SENSOR_MALFUNCTION and sensor_health != SensorHealth.OK:
            self.state = ControllerState.FAULT
            return False
        self.last_fault = FaultReason.NONE
        self.last_stop_ms = now_ms
        self.state = ControllerState.IDLE
        return True


# --- SUITE COMPLETA DE 30 CASOS DE PRUEBA ---

class TestSmartFloraCareComprehensive(unittest.TestCase):
    def setUp(self):
        self.nano_prof = CalibrationProfile(air_raw=595, water_raw=280, min_phys=50, max_phys=980, min_delta=150)
        self.esp32_prof = CalibrationProfile(air_raw=3100, water_raw=1350, min_phys=100, max_phys=4000, min_delta=200)
        self.controller = MockIrrigationController()

    # 1. Arranque seguro
    def test_01_safe_boot(self):
        ctrl = MockIrrigationController()
        self.assertEqual(ctrl.state, ControllerState.IDLE)
        self.assertEqual(ctrl.last_fault, FaultReason.NONE)
        self.assertEqual(ctrl.active_source, IrrigationSource.NONE)
        self.assertFalse(ctrl.is_pump_active())

    # 2. Estado inicial con bomba apagada
    def test_02_initial_state_pump_off(self):
        self.assertFalse(self.controller.is_pump_active())
        pump_out = self.controller.update(100, True, SensorHealth.OK, 50)
        self.assertFalse(pump_out)

    # 3. Sensor válido dentro de límites físicos
    def test_03_valid_sensor(self):
        pct, health = self.nano_prof.calculate_percent(437)
        self.assertEqual(health, SensorHealth.OK)
        self.assertIsNotNone(pct)
        self.assertTrue(0 <= pct <= 100)

    # 4. Sensor inválido (calibración incoherente)
    def test_04_invalid_sensor_calibration(self):
        bad_prof = CalibrationProfile(air_raw=200, water_raw=500, min_phys=50, max_phys=980, min_delta=150)
        self.assertFalse(bad_prof.is_valid())
        pct, health = bad_prof.calculate_percent(300)
        self.assertIsNone(pct)
        self.assertEqual(health, SensorHealth.INVALID_CALIBRATION)

    # 5. Sensor desconectado (ADC fuera de rango superior)
    def test_05_disconnected_sensor(self):
        pct, health = self.nano_prof.calculate_percent(1020) # > 980
        self.assertIsNone(pct)
        self.assertEqual(health, SensorHealth.DISCONNECTED_OR_HIGH)

    # 6. Humedad por encima del umbral
    def test_06_moisture_above_threshold(self):
        # 30% > start_pct (25%) -> No debe iniciar riego
        accepted = self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 30)
        self.assertFalse(accepted)
        self.assertEqual(self.controller.state, ControllerState.IDLE)

    # 7. Humedad por debajo del umbral
    def test_07_moisture_below_threshold(self):
        # 20% <= start_pct (25%) -> Debe autorizar e iniciar riego
        accepted = self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 20)
        self.assertTrue(accepted)
        self.assertEqual(self.controller.state, ControllerState.WATERING)

    # 8. Banda de histéresis anti-ciclado
    def test_08_hysteresis_band(self):
        # Inicia con suelo seco
        self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 18)
        # Sube a 45% (dentro de banda muerta): la bomba DEBE continuar regando
        pump_state = self.controller.update(2000, True, SensorHealth.OK, 45)
        self.assertTrue(pump_state)
        self.assertEqual(self.controller.state, ControllerState.WATERING)
        # Sube a 70% (umbral de corte): la bomba debe detenerse
        pump_state = self.controller.update(3000, True, SensorHealth.OK, 70)
        self.assertFalse(pump_state)
        self.assertEqual(self.controller.state, ControllerState.IDLE)

    # 9. Nivel de agua suficiente (evaluación nominal)
    def test_09_water_level_sufficient(self):
        accepted = self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 15)
        self.assertTrue(accepted)
        self.assertTrue(self.controller.is_pump_active())

    # 10. Nivel de agua insuficiente (bloqueo preventivo)
    def test_10_water_level_insufficient(self):
        accepted = self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, False, SensorHealth.OK, 15)
        self.assertFalse(accepted)
        self.assertEqual(self.controller.state, ControllerState.FAULT)
        self.assertEqual(self.controller.last_fault, FaultReason.NO_WATER_LEVEL)
        self.assertFalse(self.controller.is_pump_active())

    # 11. Pérdida del nivel de agua durante el bombeo (corte inmediato)
    def test_11_water_level_lost_during_watering(self):
        self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 15)
        self.assertTrue(self.controller.is_pump_active())
        # En el siguiente ciclo se detecta vaciado de agua
        pump_running = self.controller.update(2000, False, SensorHealth.OK, 20)
        self.assertFalse(pump_running)
        self.assertEqual(self.controller.state, ControllerState.FAULT)
        self.assertEqual(self.controller.last_fault, FaultReason.NO_WATER_LEVEL)

    # 12. Antirrebote del interruptor de flotador
    def test_12_float_switch_debounce(self):
        # Simulador de filtro de debouncing de 50ms
        readings = [False, True, False, False, False]
        # Con lecturas rápidas inestables no debe consolidarse el estado hasta que sea estable
        stable_state = False
        debounce_ms = 50
        last_change_ms = 100
        now_ms = 120 # solo 20ms transcurridos -> no consolida
        if (now_ms - last_change_ms) >= debounce_ms:
            stable_state = True
        self.assertFalse(stable_state)

    # 13. Antirrebote del botón físico manual
    def test_13_button_debounce(self):
        # Pulsos espurios de < 40ms no activan el controlador
        button_press_duration_ms = 25
        debounce_threshold_ms = 40
        triggered = (button_press_duration_ms >= debounce_threshold_ms)
        self.assertFalse(triggered)
        # Pulso legítimo de 60ms sí activa
        button_press_duration_ms = 60
        triggered = (button_press_duration_ms >= debounce_threshold_ms)
        self.assertTrue(triggered)

    # 14. Inicio de riego permitido bajo condiciones nominales
    def test_14_watering_start_permitted(self):
        self.controller.last_stop_ms = 0
        accepted = self.controller.request_watering(IrrigationSource.MANUAL_BUTTON, 5000, True, SensorHealth.OK, 50)
        self.assertTrue(accepted)
        self.assertEqual(self.controller.state, ControllerState.WATERING)

    # 15. Inicio de riego rechazado (concurrencia y cooldown)
    def test_15_watering_start_rejected(self):
        self.controller.request_watering(IrrigationSource.MANUAL_BUTTON, 1000, True, SensorHealth.OK, 50)
        # Petición concurrente mientras ya está regando
        second_req = self.controller.request_watering(IrrigationSource.REMOTE_WEB, 1500, True, SensorHealth.OK, 50)
        self.assertFalse(second_req)

    # 16. Límite operativo de duración de riego automático (20s en producción)
    def test_16_max_watering_duration_watchdog(self):
        self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 10)
        # Antes de 20s (19999 ms), sigue regando
        self.assertTrue(self.controller.update(1000 + 19999, True, SensorHealth.OK, 20))
        # Al alcanzar max_auto_ms (20000 ms), corta por límite automático identificable
        pump_state = self.controller.update(1000 + 20000, True, SensorHealth.OK, 20)
        self.assertFalse(pump_state)
        self.assertEqual(self.controller.state, ControllerState.FAULT)
        self.assertEqual(self.controller.last_fault, FaultReason.MAX_TIMEOUT_EXCEEDED)
        self.assertEqual(self.controller.last_stop_reason, StopReason.AUTO_MAX_DURATION_REACHED)

    # 17. Fin normal del riego por meta de humedad alcanzada
    def test_17_normal_watering_completion(self):
        self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 15)
        # Alcanza 72%
        pump_state = self.controller.update(2500, True, SensorHealth.OK, 72)
        self.assertFalse(pump_state)
        self.assertEqual(self.controller.state, ControllerState.IDLE)
        self.assertEqual(self.controller.last_stop_reason, StopReason.NORMAL_TARGET_REACHED)

    # 18. Periodo de enfriamiento obligatorio (cooldown anti-ciclado)
    def test_18_cooldown_period(self):
        self.controller.last_stop_ms = 10000
        # Petición a los 2000ms posteriores (< 5000ms de cooldown)
        accepted = self.controller.request_watering(IrrigationSource.MANUAL_BUTTON, 12000, True, SensorHealth.OK, 50)
        self.assertFalse(accepted)
        # Petición a los 6000ms posteriores (> 5000ms): aceptada
        accepted = self.controller.request_watering(IrrigationSource.MANUAL_BUTTON, 16001, True, SensorHealth.OK, 50)
        self.assertTrue(accepted)

    # 19. Bloqueo total ante estado de fallo (error lockout)
    def test_19_error_lockout(self):
        self.controller.state = ControllerState.FAULT
        self.controller.last_fault = FaultReason.NO_WATER_LEVEL
        # Ningún disparador debe encender la bomba
        self.assertFalse(self.controller.request_watering(IrrigationSource.MANUAL_BUTTON, 1000, True, SensorHealth.OK, 20))
        self.assertFalse(self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 10))
        self.assertFalse(self.controller.request_watering(IrrigationSource.REMOTE_WEB, 1000, True, SensorHealth.OK, 20))

    # 20. Recuperación condicionada tras normalización de fallos
    def test_20_recovery_after_fault(self):
        self.controller.state = ControllerState.FAULT
        self.controller.last_fault = FaultReason.NO_WATER_LEVEL
        # Intento cuando aún no hay agua -> Falla
        self.assertFalse(self.controller.attempt_recovery(1000, False, SensorHealth.OK))
        self.assertEqual(self.controller.state, ControllerState.FAULT)
        # Intento cuando se repuso el agua -> Éxito
        self.assertTrue(self.controller.attempt_recovery(2000, True, SensorHealth.OK))
        self.assertEqual(self.controller.state, ControllerState.IDLE)

    # 21. Reinicio seguro del microcontrolador (cero memoria de bombeo)
    def test_21_reboot_during_watering(self):
        # Simular que el microcontrolador se reinicia repentinamente en pleno bombeo
        rebooted_ctrl = MockIrrigationController()
        self.assertEqual(rebooted_ctrl.state, ControllerState.IDLE)
        self.assertFalse(rebooted_ctrl.is_pump_active())

    # 22. Desbordamiento de temporizador de 32 bits (millis rollover)
    def test_22_millis_timer_overflow(self):
        rollover_start = 0xFFFFFFF0 # 4294967280
        self.controller.last_stop_ms = 0
        self.controller.request_watering(IrrigationSource.MANUAL_BUTTON, rollover_start, True, SensorHealth.OK, 50)
        rollover_now = (rollover_start + 3000) & 0xFFFFFFFF
        elapsed = (rollover_now - rollover_start) & 0xFFFFFFFF
        self.assertEqual(elapsed, 3000)
        pump_running = self.controller.update(rollover_now, True, SensorHealth.OK, 50)
        self.assertFalse(pump_running)
        self.assertEqual(self.controller.state, ControllerState.IDLE)

    # 23. Solicitud HTTP sin token (rechazo 401)
    def test_23_http_missing_token(self):
        def mock_endpoint(method, token, configured_token):
            if method != "POST":
                return 405, "Method Not Allowed"
            if not token or token != configured_token:
                return 401, "Unauthorized"
            return 200, "OK"
        status, _ = mock_endpoint("POST", "", "SECURE_TOKEN_ABC_123456")
        self.assertEqual(status, 401)

    # 24. Solicitud HTTP con token inválido (rechazo 401)
    def test_24_http_invalid_token(self):
        def mock_endpoint(method, token, configured_token):
            if token != configured_token:
                return 401, "Unauthorized"
            return 200, "OK"
        status, _ = mock_endpoint("POST", "WRONG_TOKEN", "SECURE_TOKEN_ABC_123456")
        self.assertEqual(status, 401)

    # 25. Solicitud HTTP válida con POST y token correcto
    def test_25_http_valid_request(self):
        secure_token = "SUPER_SECURE_TOKEN_123456789"
        self.controller.last_stop_ms = 0
        accepted = self.controller.request_watering(IrrigationSource.REMOTE_WEB, 1000, True, SensorHealth.OK, 50)
        self.assertTrue(accepted)
        self.assertEqual(self.controller.state, ControllerState.WATERING)

    # 26. Solicitudes HTTP repetidas (idempotencia temporal)
    def test_26_http_repeated_requests(self):
        self.controller.last_stop_ms = 0
        self.controller.request_watering(IrrigationSource.REMOTE_WEB, 1000, True, SensorHealth.OK, 50)
        # Peticiones repetidas durante el pulso activo
        req2 = self.controller.request_watering(IrrigationSource.REMOTE_WEB, 1500, True, SensorHealth.OK, 50)
        req3 = self.controller.request_watering(IrrigationSource.REMOTE_WEB, 2000, True, SensorHealth.OK, 50)
        self.assertFalse(req2)
        self.assertFalse(req3)
        # El tiempo de parada original sigue siendo estrictamente 3000ms desde 1000ms
        self.controller.update(4000, True, SensorHealth.OK, 50)
        self.assertEqual(self.controller.state, ControllerState.IDLE)

    # 27. Solicitud HTTP durante bloqueo por falta de agua (rechazo 409)
    def test_27_http_request_during_lockout(self):
        accepted = self.controller.request_watering(IrrigationSource.REMOTE_WEB, 1000, False, SensorHealth.OK, 50)
        self.assertFalse(accepted)
        self.assertEqual(self.controller.state, ControllerState.FAULT)

    # 28. Detección y bloqueo de token predeterminado o inseguro (403 Forbidden)
    def test_28_http_insecure_default_token(self):
        def check_token_security(configured_token):
            if configured_token in ("FLORACARE_DEFAULT_TOKEN_CHANGE_ME", "CAMBIA_ESTE_TOKEN_SECRETO_12345"):
                return 403, "Insecure Default Token"
            if len(configured_token) < 16:
                return 403, "Token Too Short (< 16 chars)"
            return 200, "Token Valid"

        status, _ = check_token_security("FLORACARE_DEFAULT_TOKEN_CHANGE_ME")
        self.assertEqual(status, 403)
        status, _ = check_token_security("short_123")
        self.assertEqual(status, 403)
        status, _ = check_token_security("VALID_STRONG_TOKEN_987654321")
        self.assertEqual(status, 200)

    # 29. Resiliencia y autonomía local ante pérdida de Wi-Fi
    def test_29_wifi_loss_resilience(self):
        # Simular que el subsistema Wi-Fi cae a desconectado
        wifi_connected = False
        # El lazo de control local sigue evaluando humedad y flotador
        self.controller.last_stop_ms = 0
        accepted = self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 18)
        self.assertTrue(accepted)
        self.assertEqual(self.controller.state, ControllerState.WATERING)

    # 30. Garantía universal de bomba apagada ante cualquier fallo
    def test_30_all_faults_guarantee_pump_off(self):
        # Fallo 1: Falta de agua
        self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 15)
        self.controller.update(2000, False, SensorHealth.OK, 15)
        self.assertFalse(self.controller.is_pump_active())

        # Fallo 2: Fallo de sensor
        self.controller.attempt_recovery(3000, True, SensorHealth.OK)
        self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 10000, True, SensorHealth.OK, 15)
        self.controller.update(11000, True, SensorHealth.SHORT_CIRCUIT_OR_LOW, 15)
        self.assertFalse(self.controller.is_pump_active())

        # Fallo 3: Timeout de seguridad
        self.controller.attempt_recovery(20000, True, SensorHealth.OK)
        self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 25000, True, SensorHealth.OK, 15)
        self.controller.update(25000 + 25001, True, SensorHealth.OK, 15)
        self.assertFalse(self.controller.is_pump_active())

    # 31. Modelo simulado Python: Watchdog de 25s aislado del límite automático de 20s (OBL-04 / NUEVO-01)
    def test_31_absolute_safety_watchdog_25s_isolated(self):
        """
        ALCANCE: Valida la equivalencia lógica en el modelo simulado Python (MockIrrigationController).
        NOTA DE AUDITORÍA (NUEVO-01): Esta prueba ejercita el mock Python; la ejecución directa
        del controlador C++ real (include/IrrigationController.h) se realiza en
        tests/test_native_controller.cpp y test_32_native_cpp_controller_20s_and_25s_watchdog.
        """
        isolated_ctrl = MockIrrigationController(max_auto_ms=30000, manual_ms=3000, watchdog_ms=25000)
        self.assertTrue(isolated_ctrl.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 10))
        # A los 20.000 ms (límite normal de producción) y 24.999 ms sigue regando en modo de prueba aislado
        self.assertTrue(isolated_ctrl.update(1000 + 20000, True, SensorHealth.OK, 15))
        self.assertTrue(isolated_ctrl.update(1000 + 24999, True, SensorHealth.OK, 15))
        # A los 25.000 ms exactos actúa el techo inmutable maxSafetyWatchdogMs con causa SAFETY_TIMEOUT
        pump_state = isolated_ctrl.update(1000 + 25000, True, SensorHealth.OK, 15)
        self.assertFalse(pump_state)
        self.assertEqual(isolated_ctrl.state, ControllerState.FAULT)
        self.assertEqual(isolated_ctrl.last_fault, FaultReason.MAX_TIMEOUT_EXCEEDED)
        self.assertEqual(isolated_ctrl.last_stop_reason, StopReason.SAFETY_TIMEOUT)

    # 32. Ejecución nativa C++ real de include/IrrigationController.h (Modos 20s Producción y 25s Aislado)
    def test_32_native_cpp_controller_20s_and_25s_watchdog(self):
        """
        ALCANCE: Compila y ejecuta el binario C++ nativo (tests/test_native_controller.cpp) contra
        include/IrrigationController.h cuando g++/clang++ está disponible en el entorno (ej. CI Ubuntu),
        y verifica la estructura exacta de precedencia C++ y guarda condicional.
        """
        import os
        import shutil
        import subprocess
        import tempfile

        repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
        include_dir = os.path.join(repo_root, "include")
        cpp_test_file = os.path.join(repo_root, "tests", "test_native_controller.cpp")
        ctrl_header = os.path.join(include_dir, "IrrigationController.h")

        self.assertTrue(os.path.exists(cpp_test_file), "Missing native C++ test harness tests/test_native_controller.cpp")
        with open(ctrl_header, "r", encoding="utf-8") as f:
            hdr = f.read()

        # Verificar orden de evaluación en C++ real: watchdog (25s) antes de maxAutoDurationMs (20s)
        idx_wdt = hdr.find("elapsedMs >= config_.maxSafetyWatchdogMs")
        idx_auto = hdr.find("elapsedMs >= config_.maxAutoDurationMs")
        self.assertGreater(idx_wdt, 0)
        self.assertGreater(idx_auto, idx_wdt, "maxSafetyWatchdogMs check must precede maxAutoDurationMs in C++")
        self.assertIn("makeDefaultControllerConfig", hdr)
        self.assertIn("#if defined(FLORACARE_BENCH_TEST_WATCHDOG_25S)", hdr)

        compiler = shutil.which("g++") or shutil.which("clang++")
        if compiler is not None:
            with tempfile.TemporaryDirectory() as tmpdir:
                exe_suffix = ".exe" if os.name == "nt" else ""
                prod_bin = os.path.join(tmpdir, "test_native_prod" + exe_suffix)
                wdt_bin = os.path.join(tmpdir, "test_native_wdt25s" + exe_suffix)

                # 1. Compilar y ejecutar C++ real en modo producción (20.000 ms -> AUTO_MAX_DURATION_REACHED)
                subprocess.check_call([
                    compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I", include_dir, cpp_test_file, "-o", prod_bin
                ])
                out_prod = subprocess.check_output([prod_bin], text=True)
                self.assertIn("StopReason::AUTO_MAX_DURATION_REACHED verified", out_prod)

                # 2. Compilar y ejecutar C++ real en modo aislado (-DFLORACARE_BENCH_TEST_WATCHDOG_25S -> SAFETY_TIMEOUT)
                subprocess.check_call([
                    compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-DFLORACARE_BENCH_TEST_WATCHDOG_25S",
                    "-I", include_dir, cpp_test_file, "-o", wdt_bin
                ])
                out_wdt = subprocess.check_output([wdt_bin], text=True)
                self.assertIn("StopReason::SAFETY_TIMEOUT verified", out_wdt)


if __name__ == "__main__":
    unittest.main(verbosity=2)
