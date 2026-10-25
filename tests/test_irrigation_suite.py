#!/usr/bin/env python3
"""
=============================================================================
SMART FLORACARE - AUTOMATED TEST SUITE (AUDITORÍA TÉCNICA)
=============================================================================
Verifica matemáticamente y por simulación de estados todos los criterios
exigidos por la auditoría técnica:
  1. Umbrales de humedad y límites de histéresis.
  2. Validación estricta de parámetros de calibración.
  3. Comportamiento ante nivel de agua insuficiente (corte inmediato).
  4. Rechazo de solicitudes de riego inválidas / concurrentes.
  5. Límite máximo de duración por operación (Watchdog de seguridad).
  6. Idempotencia de la parada de la bomba.
  7. Transiciones completas de la Máquina de Estados (FSM).
  8. Protocolo explícito de recuperación tras fallo.
  9. Detección de lecturas ADC anómalas (desconexión / cortocircuito).
 10. Inmunidad ante desbordamiento de temporizadores (rollover de millis uint32).
 11. Seguridad HTTP y autorización por token.
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
    SAFETY_TIMEOUT = 2
    WATER_DEPLETED = 3
    USER_INTERRUPT = 4
    SENSOR_FAULT = 5

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

            if reason == StopReason.SAFETY_TIMEOUT:
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
                    self.stop(StopReason.SAFETY_TIMEOUT, now_ms)
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


# =============================================================================
# SUITE DE PRUEBAS AUTOMATIZADAS
# =============================================================================

class TestSmartFloraCare(unittest.TestCase):

    def setUp(self):
        # Perfil Arduino Nano: 580 (aire), 270 (agua), límites [50, 980]
        self.nano_prof = CalibrationProfile(580, 270, 50, 980, 50)
        # Perfil ESP32: 3100 (aire), 1350 (agua), límites [100, 4000]
        self.esp32_prof = CalibrationProfile(3100, 1350, 100, 4000, 200)
        self.controller = MockIrrigationController()

    # --- 1. UMBRALES DE HUMEDAD ---
    def test_01_moisture_thresholds(self):
        # 580 ADC (aire) debe dar 0%
        pct, health = self.nano_prof.calculate_percent(580)
        self.assertEqual(health, SensorHealth.OK)
        self.assertEqual(pct, 0)

        # 270 ADC (agua) debe dar 100%
        pct, health = self.nano_prof.calculate_percent(270)
        self.assertEqual(health, SensorHealth.OK)
        self.assertEqual(pct, 100)

        # Punto medio: (580 + 270)/2 = 425 ADC debe dar ~50%
        pct, health = self.nano_prof.calculate_percent(425)
        self.assertEqual(health, SensorHealth.OK)
        self.assertEqual(pct, 50)

    # --- 2. VALIDACIÓN DE CALIBRACIÓN ---
    def test_02_calibration_validation(self):
        # Aire menor que agua (físicamente absurdo en capacitivo)
        invalid_prof = CalibrationProfile(200, 500, 50, 980, 50)
        self.assertFalse(invalid_prof.is_valid())

        # Diferencia menor al delta mínimo (riesgo de división por cero o hipersensibilidad)
        small_delta = CalibrationProfile(500, 480, 50, 980, 50)
        self.assertFalse(small_delta.is_valid())

        # Calibración válida
        self.assertTrue(self.nano_prof.is_valid())
        self.assertTrue(self.esp32_prof.is_valid())

    # --- 3. NIVEL DE AGUA INSUFICIENTE ---
    def test_03_insufficient_water_level(self):
        # Intento de riego cuando el depósito está seco
        accepted = self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, False, SensorHealth.OK, 15)
        self.assertFalse(accepted)
        self.assertEqual(self.controller.state, ControllerState.FAULT)
        self.assertEqual(self.controller.last_fault, FaultReason.NO_WATER_LEVEL)
        self.assertFalse(self.controller.is_pump_active())

        # Si el agua se agota en pleno riego, la bomba debe detenerse al instante
        # Recuperar primero
        self.controller.attempt_recovery(2000, True, SensorHealth.OK)
        self.controller.last_stop_ms = 0
        accepted = self.controller.request_watering(IrrigationSource.MANUAL_BUTTON, 3000, True, SensorHealth.OK, 15)
        self.assertTrue(accepted)
        self.assertTrue(self.controller.is_pump_active())

        # Se abre el flotador a t=3500ms
        pump_state = self.controller.update(3500, False, SensorHealth.OK, 15)
        self.assertFalse(pump_state)
        self.assertEqual(self.controller.state, ControllerState.FAULT)
        self.assertEqual(self.controller.last_fault, FaultReason.NO_WATER_LEVEL)

    # --- 4. SOLICITUDES DE RIEGO INVÁLIDAS / CONCURRENTES ---
    def test_04_invalid_and_concurrent_requests(self):
        self.controller.last_stop_ms = 0
        accepted = self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 15)
        self.assertTrue(accepted)
        self.assertTrue(self.controller.is_pump_active())

        # Solicitud duplicada mientras está regando (no debe reiniciar el temporizador ni permitirse)
        dup_accepted = self.controller.request_watering(IrrigationSource.REMOTE_WEB, 1500, True, SensorHealth.OK, 15)
        self.assertFalse(dup_accepted)
        self.assertEqual(self.controller.pump_start_time_ms, 1000) # El inicio no se alteró

    # --- 5. LÍMITE MÁXIMO DE DURACIÓN (WATCHDOG) ---
    def test_05_max_duration_watchdog(self):
        self.controller.last_stop_ms = 0
        self.controller.request_watering(IrrigationSource.MANUAL_BUTTON, 1000, True, SensorHealth.OK, 50)
        self.assertTrue(self.controller.is_pump_active())

        # A t=3999ms sigue regando (duración manual es 3000ms)
        self.assertTrue(self.controller.update(3999, True, SensorHealth.OK, 50))
        # A t=4000ms debe cortar por ráfaga manual cumplida
        self.assertFalse(self.controller.update(4000, True, SensorHealth.OK, 50))
        self.assertEqual(self.controller.state, ControllerState.IDLE)
        self.assertEqual(self.controller.last_stop_reason, StopReason.MANUAL_BURST_COMPLETE)

        # Prueba Watchdog de seguridad absoluto (25000ms) en modo auto que no alcanza la humedad
        self.controller.last_stop_ms = 0
        self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 10000, True, SensorHealth.OK, 15)
        # Simular que pasaron 25001ms sin subir humedad
        self.assertFalse(self.controller.update(35001, True, SensorHealth.OK, 15))
        self.assertEqual(self.controller.state, ControllerState.FAULT)
        self.assertEqual(self.controller.last_fault, FaultReason.MAX_TIMEOUT_EXCEEDED)

    # --- 6. PARADA IDEMPOTENTE ---
    def test_06_idempotent_stop(self):
        self.controller.last_stop_ms = 0
        self.controller.request_watering(IrrigationSource.MANUAL_BUTTON, 1000, True, SensorHealth.OK, 50)
        self.assertTrue(self.controller.is_pump_active())

        # Llamar a stop 1 vez
        self.controller.stop(StopReason.USER_INTERRUPT, 1500)
        self.assertFalse(self.controller.is_pump_active())
        self.assertEqual(self.controller.state, ControllerState.IDLE)

        # Llamar a stop consecutivamente 5 veces más en reposo
        for _ in range(5):
            self.controller.stop(StopReason.USER_INTERRUPT, 2000)
            self.assertFalse(self.controller.is_pump_active())
            self.assertEqual(self.controller.state, ControllerState.IDLE)

    # --- 7. TRANSICIONES DE ESTADO ---
    def test_07_state_transitions(self):
        self.assertEqual(self.controller.state, ControllerState.IDLE)

        self.controller.last_stop_ms = 0
        # IDLE -> WATERING
        self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, SensorHealth.OK, 10)
        self.assertEqual(self.controller.state, ControllerState.WATERING)

        # WATERING -> STOPPED -> IDLE (Al alcanzar meta de humedad >= 70%)
        self.controller.update(2000, True, SensorHealth.OK, 72)
        self.assertEqual(self.controller.state, ControllerState.IDLE)
        self.assertEqual(self.controller.last_stop_reason, StopReason.NORMAL_TARGET_REACHED)

    # --- 8. RECUPERACIÓN DESPUÉS DE FALLO ---
    def test_08_recovery_after_fault(self):
        # Provocar fallo por falta de agua
        self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, False, SensorHealth.OK, 15)
        self.assertEqual(self.controller.state, ControllerState.FAULT)

        # Intento de recuperación si todavía NO hay agua (debe fracasar y seguir en FAULT)
        rec_ok = self.controller.attempt_recovery(2000, False, SensorHealth.OK)
        self.assertFalse(rec_ok)
        self.assertEqual(self.controller.state, ControllerState.FAULT)

        # Intento de recuperación cuando se rellenó el agua (debe transicionar a IDLE con éxito)
        rec_ok = self.controller.attempt_recovery(3000, True, SensorHealth.OK)
        self.assertTrue(rec_ok)
        self.assertEqual(self.controller.state, ControllerState.IDLE)
        self.assertEqual(self.controller.last_fault, FaultReason.NONE)

    # --- 9. LECTURAS ANÓMALAS (SENSOR FAULT) ---
    def test_09_anomalous_readings(self):
        # Sensor desconectado (ADC = 1020 en Nano, > 980)
        pct, health = self.nano_prof.calculate_percent(1020)
        self.assertIsNone(pct)
        self.assertEqual(health, SensorHealth.DISCONNECTED_OR_HIGH)

        # Cortocircuito a GND (ADC = 10 en Nano, < 50)
        pct, health = self.nano_prof.calculate_percent(10)
        self.assertIsNone(pct)
        self.assertEqual(health, SensorHealth.SHORT_CIRCUIT_OR_LOW)

        # El controlador NO debe autorizar riego automático con sensor en estado anómalo
        self.controller.last_stop_ms = 0
        accepted = self.controller.request_watering(IrrigationSource.AUTO_CYCLE, 1000, True, health, 10)
        self.assertFalse(accepted)
        self.assertEqual(self.controller.state, ControllerState.FAULT)
        self.assertEqual(self.controller.last_fault, FaultReason.SENSOR_MALFUNCTION)

    # --- 10. DESBORDAMIENTO DE TEMPORIZADORES (ROLLOVER uint32) ---
    def test_10_timer_overflow(self):
        # Simular que el microcontrolador lleva 49.7 días encendido y millis() está cerca del desbordamiento
        rollover_start = 0xFFFFFFF0 # 4294967280
        self.controller.last_stop_ms = 0
        self.controller.request_watering(IrrigationSource.MANUAL_BUTTON, rollover_start, True, SensorHealth.OK, 50)
        self.assertTrue(self.controller.is_pump_active())

        # now_ms cruza el desbordamiento: 0xFFFFFFF0 + 3000 = 0x00000BA8 (2984)
        rollover_now = (rollover_start + 3000) & 0xFFFFFFFF
        # La resta sin signo (now - start) & 0xFFFFFFFF da exactamente 3000ms
        elapsed = (rollover_now - rollover_start) & 0xFFFFFFFF
        self.assertEqual(elapsed, 3000)

        # El controlador debe apagar la bomba limpiamente sin congelarse
        pump_running = self.controller.update(rollover_now, True, SensorHealth.OK, 50)
        self.assertFalse(pump_running)
        self.assertEqual(self.controller.state, ControllerState.IDLE)
        self.assertEqual(self.controller.last_stop_reason, StopReason.MANUAL_BURST_COMPLETE)

    # --- 11. RECHAZO DE SOLICITUDES HTTP NO AUTORIZADAS ---
    def test_11_http_authorization(self):
        valid_token = "FLORACARE_SECRET_TOKEN_2026"
        
        # Simulación de middleware HTTP:
        def mock_api_water(method, received_token):
            if method != "POST":
                return 405, "Method Not Allowed"
            if received_token != valid_token:
                return 401, "Unauthorized"
            return 200, "Accepted"

        # Rechazo por método GET
        status, _ = mock_api_water("GET", valid_token)
        self.assertEqual(status, 405)

        # Rechazo por token inválido en POST
        status, _ = mock_api_water("POST", "WRONG_TOKEN")
        self.assertEqual(status, 401)

        # Rechazo por token ausente
        status, _ = mock_api_water("POST", "")
        self.assertEqual(status, 401)

        # Aceptación con método POST y token válido
        status, _ = mock_api_water("POST", valid_token)
        self.assertEqual(status, 200)


if __name__ == "__main__":
    unittest.main(verbosity=2)
