# 📋 06. Matriz de Trazabilidad de Requisitos (RTM - Requirements Traceability Matrix)

Esta matriz vincula cada requisito funcional, eléctrico, de seguridad, de software y de documentación con su componente de implementación, categoría de prueba, resultado, evidencia verificable y estado técnico de cumplimiento.

---

## 🔬 Taxonomía Formal de Evidencias y Pruebas:
Para evitar cualquier ambigüedad técnica, las evidencias del sistema se clasifican rigurosamente en las siguientes categorías:
- **`CONFIRMADO POR CÓDIGO`:** Verificado mediante inspección estática del código fuente en el repositorio.
- **`CONFIRMADO POR CI`:** Verificado mediante ejecución remota en GitHub Actions con código de salida 0.
- **`CONFIRMADO POR DOCUMENTACIÓN`:** Registrado en guías, esquemas o notas de aplicación, sin validar en hardware.
- **`VERIFICADO POR CÁLCULO`:** Derivado mediante ecuaciones nodales reproducibles con supuestos explícitos.
- **`SIMULADO`:** Verificado mediante modelos lógicos o mocks en Python (no invoca silicio real).
- **`PENDIENTE DE BANCO`:** Requiere ejecución y medición física en banco instrumentado (ACT-01 a ACT-06).
- **`PENDIENTE DE DECISIÓN DEL PROPIETARIO`:** Requiere definición de alcance o especificación del usuario.
- **`REGLA FÍSICA EXTERNA`:** Condición hidráulica o mecánica dependiente de la instalación física (e.g. Air Gap anti-sifón).

---

## 📊 Matriz Completa de Requisitos

| ID | Categoría | Descripción del Requisito | Componente / Archivo | Nivel / Tipo de Prueba | Resultado | Evidencia Verificable | Estado |
| :---: | :--- | :--- | :--- | :--- | :---: | :--- | :---: |
| **REQ-01** | Funcional | Arranque seguro con bomba apagada y FSM en IDLE | `IrrigationController.h`, `regador_*.cpp` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | Pin digital inicializado en LOW; estado inicial IDLE | `CONFIRMADO POR CI` |
| **REQ-02** | Dominio | Histéresis de riego: inicio $\le 20\%$, detención $\ge 70\%$ | `IrrigationController.h` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | Transición a WATERING en $\le 20\%$ y corte al alcanzar $70\%$ | `CONFIRMADO POR CI` |
| **REQ-03** | Calibración | Validación matemática de perfil de calibración (`airRaw > waterRaw`) | `SensorValidator.h` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | Rechazo de calibraciones corruptas con INVALID_CALIBRATION | `CONFIRMADO POR CI` |
| **REQ-04** | Diagnóstico | Detección de averías de sensor (desconexión o cortocircuito a masa) | `SensorValidator.h` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | Clasificación en `DISCONNECTED_OR_HIGH` o `SHORT_CIRCUIT_OR_LOW` | `CONFIRMADO POR CI` |
| **REQ-05** | Seguridad | Enclavamiento por nivel de agua bajo (corte preventivo y en marcha) | `IrrigationController.h`, `regador_*.cpp` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | Bomba bloqueada incondicionalmente si el flotador se abre | `CONFIRMADO POR CI` |
| **REQ-06** | Hardware | Convención Fail-Safe ante rotura o desconexión del cable de flotador | `03_DIAGRAMA_Y_CONEXIONES.md` | Nivel B1: Estático (`test_firmware_security.py`) | **PASS** | Pull-up fuerza HIGH si el cable se corta, simulando vacío | `CONFIRMADO POR CÓDIGO` |
| **REQ-07** | Filtrado | Antirrebote digital de flotador (50 ms) y botón táctil (40 ms) | `regador_*.cpp` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | Pulsos espurios $< 40\,\text{ms}$ ignorados; señal estable procesada | `CONFIRMADO POR CI` |
| **REQ-08** | Seguridad | Doble barrera temporal: Límite auto (20s) y Watchdog absoluto inmutable (25s) | `IrrigationController.h`, `regador_*.cpp` | Nivel A: Dominio Lógico (`test_16`, `test_31`) | **PASS** | `AUTO_MAX_DURATION_REACHED` (20s) y `SAFETY_TIMEOUT` (25s aislado) diferenciados | `CONFIRMADO POR CI` (Banco: ACT-06.A1/A2) |
| **REQ-09** | Fiabilidad | Periodo obligatorio de enfriamiento (cooldown 5s) | `IrrigationController.h` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | Solicitudes dentro de los 5s posteriores a la parada son rechazadas | `CONFIRMADO POR CI` |
| **REQ-10** | Fiabilidad | Idempotencia y garantía de apagado ante cualquier fallo | `IrrigationController.h` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | Llamadas repetidas a `stop()` mantienen la bomba apagada | `CONFIRMADO POR CI` |
| **REQ-11** | Resiliencia | Protocolo explícito de recuperación tras resolver la causa física | `IrrigationController.h` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | `attemptRecovery()` solo restaura a IDLE si el agua se repuso | `CONFIRMADO POR CI` |
| **REQ-12** | Robustez | Inmunidad ante desbordamiento de 32 bits de `millis()` (49.7 días) | `IrrigationController.h` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | Aritmética modular sin signo `(now - start) >= duration` calculada | `CONFIRMADO POR CI` |
| **REQ-13** | Seguridad | Determinismo tras reinicio súbito (cero memoria residual de bombeo) | `regador_*.cpp` | Nivel A: Dominio Lógico (`test_irrigation_suite.py`) | **PASS** | Boot arranca siempre en IDLE con pin en LOW | `CONFIRMADO POR CI` |
| **REQ-14** | IoT / Red | Autenticación obligatoria por token en endpoint `POST /api/water` | `regador_esp32.cpp` | Nivel B2: Simulación HTTP (`test_http_auth_security.py`) | **PASS (SIM)** | HTTP 401 si falta o no coincide el token | `SIMULADO` |
| **REQ-15** | IoT / Red | Rechazo de tokens por defecto de fábrica o débiles (< 16 caracteres) | `regador_esp32.cpp` | Nivel B2: Simulación HTTP (`test_http_auth_security.py`) | **PASS (SIM)** | HTTP 403 Forbidden si el token es placeholder o corto | `SIMULADO` |
| **REQ-16** | IoT / Red | No exposición de secretos en el código fuente HTML del dashboard | `regador_esp32.cpp` | Nivel B1: Estático (`test_firmware_security.py`) | **PASS** | Formulario web utiliza `<input type='password'>` sin valor embebido | `CONFIRMADO POR CÓDIGO` |
| **REQ-17** | Robustez | Operación autónoma ciega si la pantalla OLED no responde | `regador_nano_pro.cpp`, `regador_esp32.cpp` | Nivel B1: Estático (`test_firmware_security.py`) | **PASS** | Eliminado bucle `while(true){}`; `oledAvailable = false` | `CONFIRMADO POR CÓDIGO` |
| **REQ-18** | Potencia | MOSFET con saturación garantizada por datasheet a 5.0V (Nano) | `03_DIAGRAMA_Y_CONEXIONES.md`, `ADR-001` | Hoja de datos Vishay IRLZ44N (Doc #91328) | **VERIFICADO** | $R_{DS(on)} \le 0.035\,\Omega$ garantizado a $V_{GS} = 5.0\,\text{V}, 25\,\text{A}$ | `CONFIRMADO POR DOCUMENTACIÓN` |
| **REQ-19** | Potencia | Driver no inversor de 2 etapas NPN+PNP para compuerta MOSFET en ESP32 (3.3V) | `03_DIAGRAMA_Y_CONEXIONES.md`, `ADR-001` | Análisis nodal, saturación $V_{GS} \approx 4.85\,\text{V}$ y pull-downs | **CALCULADO** | Driver NPN 2N2222A + PNP 2N3906 con $R_2=2.2\,\text{k}\Omega$; corte seguro en reset | `PENDIENTE DE BANCO (ACT-01, ACT-02)` |
| **REQ-20** | Protección | Supresión inductiva Back-EMF de la bomba mediante diodo Schottky | `03_DIAGRAMA_Y_CONEXIONES.md` | Hoja de datos 1N5819 ($1\text{A}, 40\text{V}, V_F \le 0.45\text{V}$) | **DOCUMENTADO** | Conexión en antiparalelo directo a bornes del motor | `PENDIENTE DE BANCO (ACT-04)` |
| **REQ-21** | Hidráulica | Prevención del efecto sifón continuo mediante Air Gap | `05_GUIA_DE_DESARROLLO_Y_FASE_A_FASE.md` | Procedimiento de banco hidráulico | **DOCUMENTADO** | Suspensión libre $\ge 2\,\text{cm}$ sobre la tierra y depósito a nivel | `REGLA FÍSICA EXTERNA` |
| **REQ-22** | Calibración | Calibración empírica del lote comercial de sensores de suelo | `04_CALIBRACION_Y_CONFIGURACION.md` | Protocolo de calibración seca y húmeda | **DOCUMENTADO** | Medición requerida de `airRaw` y `waterRaw` en el lote real | `PENDIENTE DE BANCO` |
| **REQ-23** | CI | Compilación automatizada y ejecución de pruebas en GitHub Actions (`main` y `audit/**`) | `.github/workflows/ci.yml` | Workflow GitHub Actions | **SUCCESS** | Compila Nano, ESP32 y corre la suite completa de 76 pruebas | `CONFIRMADO POR CI` |
| **REQ-24** | Seguridad | Mitigación CSRF mediante validación canónica de Origin/Referer y Rate Limiting | `regador_esp32.cpp` | Nivel B2: Simulación HTTP (`test_http_auth_security.py`) | **PASS (SIM)** | `validateSingleAuthority()` exige esquema http://, puerto 80 canónico y rate limiting 429 | `SIMULADO` (Banco: ACT-06) |
| **REQ-25** | Seguridad | Comparación en tiempo constante y límite temprano de carga útil (413) | `regador_esp32.cpp` | Nivel B2: Simulación HTTP (`test_http_auth_security.py`) | **PASS (SIM)** | `safeConstantTimeEquals()` branchless y rechazo temprano de cuerpos > 256 bytes | `SIMULADO` (Banco: ACT-06) |
| **REQ-26** | Alcance | Unicidad de actuador (microbomba de 5V) y ausencia de electroválvula de 12V | `02_LISTA_DE_MATERIALES_Y_COSTOS.md`, `03_DIAGRAMA_Y_CONEXIONES.md`, `regador_*.cpp` | Inspección conjunta de alcance, repositorio y esquemáticos (MEN-03) | **CONFIRMADO** | Sistema opera exclusivamente con bomba JT-DC3W a 5V; electroválvula 12V no aplica | `CONFIRMADO POR REVISIÓN DE ALCANCE, REPOSITORIO Y DOCUMENTACIÓN (DOC/REPO)` |
