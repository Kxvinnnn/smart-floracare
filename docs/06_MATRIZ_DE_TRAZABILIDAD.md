# 📋 06. Matriz de Trazabilidad de Requisitos (RTM - Requirements Traceability Matrix)

Esta matriz vincula cada requisito funcional, eléctrico, de seguridad, de software y de documentación con su componente de implementación, prueba asociada, resultado, evidencia verificable y estado técnico de cumplimiento.

---

## Convenciones de Estado de Requisitos:
- **`IMPLEMENTADO / PRUEBA SUPERADA`:** Verificado mediante ejecución automatizada en suite de pruebas o compilador con código de salida cero.
- **`VALIDACIÓN FÍSICA PENDIENTE`:** Requiere instrumentación en banco de pruebas con hardware real (DMM, osciloscopio, depósito de agua).
- **`REGLA FÍSICA EXTERNA`:** Condición inherente a la hidráulica no mitigable por código.
- **`BLOQUEADO`:** Impide la compra o despliegue hasta su resolución.

---

## 📊 Matriz Completa de Requisitos

| ID | Categoría | Descripción del Requisito | Componente / Archivo | Prueba Asociada | Resultado | Evidencia Verificable | Estado |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- | :---: |
| **REQ-01** | Funcional | Arranque seguro con bomba apagada y FSM en IDLE | `IrrigationController.h`, `regador_*.cpp` | `test_01_safe_boot`, `test_02_initial_state_pump_off` | **PASS** | Pin digital inicializado en LOW; estado inicial IDLE | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-02** | Dominio | Histéresis de riego: inicio $\le 25\%$, detención $\ge 70\%$ | `IrrigationController.h` | `test_06_moisture_above_threshold`, `test_07_moisture_below_threshold`, `test_08_hysteresis_band` | **PASS** | Transición a WATERING en $\le 25\%$ y apagado al alcanzar $70\%$ | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-03** | Calibración | Validación matemática de perfil de calibración (`airRaw > waterRaw`) | `SensorValidator.h` | `test_04_invalid_sensor_calibration` | **PASS** | Rechazo de calibraciones corruptas con INVALID_CALIBRATION | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-04** | Diagnóstico | Detección de averías de sensor (desconexión o cortocircuito a masa) | `SensorValidator.h` | `test_03_valid_sensor`, `test_05_disconnected_sensor`, `test_30_all_faults_guarantee_pump_off` | **PASS** | Clasificación en `DISCONNECTED_OR_HIGH` o `SHORT_CIRCUIT_OR_LOW` | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-05** | Seguridad | Enclavamiento por nivel de agua bajo (corte preventivo y en marcha) | `IrrigationController.h`, `regador_*.cpp` | `test_09_water_level_sufficient`, `test_10_water_level_insufficient`, `test_11_water_level_lost_during_watering` | **PASS** | Bomba bloqueada incondicionalmente si el flotador se abre | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-06** | Hardware | Convención Fail-Safe ante rotura o desconexión del cable de flotador | `03_DIAGRAMA_Y_CONEXIONES.md` | `test_10_water_level_insufficient` | **PASS** | Pull-up fuerza HIGH si el cable se corta, simulando vacío | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-07** | Filtrado | Antirrebote digital de flotador (50 ms) y botón táctil (40 ms) | `regador_*.cpp` | `test_12_float_switch_debounce`, `test_13_button_debounce` | **PASS** | Pulsos espurios $< 40\,\text{ms}$ ignorados; señal estable procesada | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-08** | Seguridad | Watchdog absoluto inmutable de tiempo de bombeo (25s máx) | `IrrigationController.h` | `test_16_max_watering_duration_watchdog` | **PASS** | Corte forzado incondicional al transcurrir 25s | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-09** | Fiabilidad | Periodo obligatorio de enfriamiento (cooldown 5s) | `IrrigationController.h` | `test_18_cooldown_period`, `test_15_watering_start_rejected` | **PASS** | Solicitudes dentro de los 5s posteriores a la parada son rechazadas | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-10** | Fiabilidad | Idempotencia y garantía de apagado ante cualquier fallo | `IrrigationController.h` | `test_30_all_faults_guarantee_pump_off` | **PASS** | Llamadas repetidas a `stop()` mantienen la bomba apagada | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-11** | Resiliencia | Protocolo explícito de recuperación tras resolver la causa física | `IrrigationController.h` | `test_20_recovery_after_fault` | **PASS** | `attemptRecovery()` solo restaura a IDLE si el agua se repuso | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-12** | Robustez | Inmunidad ante desbordamiento de 32 bits de `millis()` (49.7 días) | `IrrigationController.h` | `test_22_millis_timer_overflow` | **PASS** | Aritmética modular sin signo `(now - start) >= duration` calculada | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-13** | Seguridad | Determinismo tras reinicio súbito (cero memoria residual de bombeo) | `regador_*.cpp` | `test_21_reboot_during_watering` | **PASS** | Boot arranca siempre en IDLE con pin en LOW | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-14** | IoT / Red | Autenticación obligatoria por token en endpoint `POST /api/water` | `regador_esp32.cpp` | `test_23_http_missing_token`, `test_24_http_invalid_token`, `test_25_http_valid_request` | **PASS** | HTTP 401 si falta o no coincide el token | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-15** | IoT / Red | Rechazo de tokens por defecto de fábrica o débiles (< 16 caracteres) | `regador_esp32.cpp` | `test_28_http_insecure_default_token` | **PASS** | HTTP 403 Forbidden si el token es placeholder o corto | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-16** | IoT / Red | No exposición de secretos en el código fuente HTML del dashboard | `regador_esp32.cpp` | Inspección de código en `handleRoot()` | **PASS** | Formulario web utiliza `<input type='password'>` sin valor embebido | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-17** | Robustez | Operación autónoma ciega si la pantalla OLED no responde | `regador_nano_pro.cpp`, `regador_esp32.cpp` | Compilación PlatformIO y bypass en `renderDisplay()` | **PASS** | Eliminado bucle `while(true){}`; `oledAvailable = false` | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-18** | Potencia | MOSFET con saturación garantizada por datasheet a 5.0V (Nano) | `03_DIAGRAMA_Y_CONEXIONES.md`, `ADR-001` | Hoja de datos Vishay IRLZ44N | **PASS** | $R_{DS(on)} \le 0.035\,\Omega$ garantizado a $V_{GS} = 5.0\,\text{V}$ | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-19** | Potencia | Driver no inversor de 2 etapas NPN+PNP para compuerta MOSFET en ESP32 (3.3V) | `03_DIAGRAMA_Y_CONEXIONES.md`, `ADR-001` | Análisis nodal, saturación $V_{GS} \approx 4.85\,\text{V}$ y pull-downs | **CALCULADO** | Driver NPN 2N2222A + PNP 2N3906 con $R_2=2.2\,\text{k}\Omega$; HW-517 alternativo pendiente | `VALIDACIÓN FÍSICA PENDIENTE` |
| **REQ-20** | Protección | Supresión inductiva Back-EMF de la bomba mediante diodo Schottky | `03_DIAGRAMA_Y_CONEXIONES.md` | Hoja de datos 1N5819 ($1\text{A}, 40\text{V}, V_F \approx 0.35\text{V}$) | **DOCUMENTADO** | Conexión en antiparalelo directo a bornes del motor | `VALIDACIÓN FÍSICA PENDIENTE` |
| **REQ-21** | Hidráulica | Prevención del efecto sifón continuo mediante Air Gap | `05_GUIA_DE_DESARROLLO_Y_FASE_A_FASE.md` | Procedimiento de banco hidráulico | **DOCUMENTADO** | Suspensión libre $\ge 2\,\text{cm}$ sobre la tierra y depósito a nivel | `REGLA FÍSICA EXTERNA` |
| **REQ-22** | Calibración | Calibración empírica del lote comercial de sensores de suelo | `04_CALIBRACION_Y_CONFIGURACION.md` | Protocolo de calibración seca y húmeda | **DOCUMENTADO** | Medición requerida de `airRaw` y `waterRaw` en el lote real | `VALIDACIÓN FÍSICA PENDIENTE` |
| **REQ-23** | CI | Compilación automatizada y ejecución de pruebas en GitHub Actions | `.github/workflows/ci.yml` | Workflow GitHub Actions | **PASS** | Compila Nano, ESP32 y corre la suite completa de 75 pruebas | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-24** | Seguridad | Mitigación CSRF mediante validación canónica de Origin/Referer y Rate Limiting | `regador_esp32.cpp` | `tests/test_http_auth_security.py` (Tests 19, 21, 22) | **PASS** | `validateSingleAuthority()` exige esquema http://, puerto 80 canónico y rate limiting 429 | `IMPLEMENTADO / PRUEBA SUPERADA` |
| **REQ-25** | Seguridad | Comparación en tiempo constante y límite temprano de carga útil (413) | `regador_esp32.cpp` | `tests/test_http_auth_security.py` (Tests 18, 32), `test_firmware_security.py` | **PASS** | `safeConstantTimeEquals()` branchless y rechazo temprano de cuerpos > 256 bytes | `IMPLEMENTADO / PRUEBA SUPERADA` |
