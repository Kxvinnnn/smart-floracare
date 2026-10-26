# Changelog

Todos los cambios notables de este proyecto están documentados en este archivo.

El formato se basa en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/)
y este proyecto se adhiere a [Semantic Versioning](https://semver.org/lang/es/).

---

## [1.2.1] - 2026-10-26

### Seguridad y Endurecimiento IoT (Auditoría y Cierre de Firmware ESP32)
- **Mitigación de Ataques de Temporización (Timing Attacks):** Implementación de la función `safeConstantTimeEquals()` en `src/esp32_mejorado_wifi/regador_esp32.cpp`, garantizando una evaluación de longitud y caracteres en tiempo constante mediante operaciones XOR bit a bit volátiles sin ramas dependientes del contenido del secreto.
- **Parser Canónico y Riguroso de Origen y Referer (Defensa Anti-CSRF):** Separación estricta entre RFC 6454 (`Origin` sin rutas, queries ni fragmentos) y RFC 7231 (`Referer` con extracción de autoridad previa a delimitadores de ruta). Validación exhaustiva: esquema obligatorio `http://` (insensible a mayúsculas), rechazo estricto de `https://`, `ftp://`, `ws://`, rechazo de puertos no canónicos (ej. `:080`, `:+80`, `:8080`, `:`), rechazo de caracteres de control, userinfo `@`, corchetes IPv6 y espacios. Validación exclusiva contra la IP física asignada (`WiFi.localIP()`), eliminación de aliases mDNS no configurados y verificación de consistencia entre Origin y Referer si ambos están presentes.
- **Límite Estricto de Entrada contra DoS de Memoria (Heap Exhaustion):** Imposición de `MAX_TOKEN_LEN = 128` bytes sobre los tokens recibidos y rechazo temprano con `413 Payload Too Large` de cargas útiles superiores a 256 bytes en el endpoint `/api/water`.
- **Normalización de Credenciales y Rechazo de Ambigüedad:** Validación estricta del esquema `Authorization: Bearer <token>`. Rechazo de esquemas no soportados (ej. `Basic`) con `401 Unauthorized`. Si una petición envía simultáneamente credenciales por formulario y por encabezado pero con valores contradictorios, se rechaza de forma determinista con `400 Bad Request`.
- **Bloqueo Activo de Marcadores de Posición (Placeholders):** Ampliación de la política de rechazo a todos los marcadores conocidos de las plantillas (`GENERA_UN_TOKEN_SECRETO_DE_16_O_MAS_CARACTERES`, `CAMBIA_ESTE_TOKEN_SECRETO_12345`, `1234567890123456`), deshabilitando el endpoint con `403 Forbidden`.
- **Rate Limiting Defensivo No Bloqueante (HTTP 429):** Reemplazo de retardos bloqueantes por control de frecuencia no bloqueante basado en `lastFailedAuthMs`. Reintentos fallidos en menos de 1000 ms devuelven inmediatamente `429 Too Many Requests` en $< 0.1\,\text{ms}$, preservando el determinismo de la máquina de estados y la respuesta del lazo de control.
- **Protección de Salida y Erradicación de Fuga de Secretos:** Confirmación de que las respuestas HTTP y los registros del puerto serie (`Serial.print`) jamás imprimen variables de contraseñas ni tokens recibidos.
- **Suite Integral de 33 Pruebas de Seguridad HTTP:** Creación de `tests/test_http_auth_security.py` cubriendo los 33 escenarios de configuración, transporte, CSRF, autenticación e integridad del controlador, elevando la cobertura total del repositorio a **75 pruebas automatizadas**.
- **Pipeline de Integración Continua (CI):** Actualización de `.github/workflows/ci.yml` para descubrir y ejecutar automáticamente la suite completa de 75 pruebas (`python -m unittest discover tests`) en cada push y PR.

### Corregido
- **Corrección Crítica de Esquemas Eléctricos ASCII (Driver ESP32):** En `docs/03_DIAGRAMA_Y_CONEXIONES.md`, rectificación de los diagramas ASCII de las Secciones 3.2 y 4.1, incorporando físicamente la resistencia de acoplamiento $R_2 = 2.2\,	ext{k}\Omega$ entre Colector Q1 y Base Q2, y corrigiendo el trazado del riel de $+5\,	ext{V}$ directo hacia el Emisor de Q2, eliminando el riesgo de cortocircuito destructivo de la unión base-emisor del transistor PNP.
- **Contradicción Eléctrica en Guía de Montaje ESP32:** En `docs/05_GUIA_DE_DESARROLLO_Y_FASE_A_FASE.md`, sustitución de la instrucción obsoleta de accionamiento directo desde GPIO 18 con R=1 kΩ por la especificación del **Driver No Inversor Bipolar de 2 Etapas (Q1 NPN 2N2222A + Q2 PNP 2N3906 + Q3 IRLZ44N)**, detallando nodo por nodo las conexiones, resistencias de polarización ($2.2\,	ext{k}\Omega$) y pull-downs de seguridad ($10\,	ext{k}\Omega$).
- **Ajuste Riguroso de Especificación de Datasheet (IRLZ44N):** En `docs/01_ARQUITECTURA_DEL_SISTEMA.md`, `docs/02_LISTA_DE_MATERIALES_Y_COSTOS.md`, `docs/03_DIAGRAMA_Y_CONEXIONES.md` y `docs/adr/ADR-001-MOSFET-SELECTION.md`, sustitución de afirmaciones absolutas ("100% garantizado") por la cita estandarizada de la hoja de datos Vishay Siliconix Doc #91328 ($V_{GS}=5.0\,	ext{V}, I_D=25\,	ext{A}, T_J=25^\circ	ext{C} \implies \le 0.035\,\Omega$; $V_{GS}=4.0\,	ext{V} \implies \le 0.040\,\Omega$), acotando el límite de catálogo entre $0.035\,\Omega$ y $0.040\,\Omega$ para $V_{OH} pprox 4.2\,	ext{V}-4.9\,	ext{V}$ a $25^\circ	ext{C}$ y condicionándolo a la medición real de $V_{GS}$ y $V_{DS}$ en banco.
- **Reconciliación de BOM con Arquitectura Actual:** En `docs/02_LISTA_DE_MATERIALES_Y_COSTOS.md`, reclasificación del módulo HW-517 como alternativa comercial pendiente de evaluación en banco, actualización de notas del diodo flyback 1N5819 como selección provisional y ratificación de la prohibición de inmovilizar el rotor de la microbomba.
- **Alineación de Criterios Térmicos y Duración de Ensayo ACT-05:** En `docs/05_GUIA_DE_DESARROLLO_Y_FASE_A_FASE.md`, clarificación de que los 20 segundos de bombeo evalúan la estabilidad del ciclo máximo permitido por el watchdog del firmware y no el equilibrio térmico estacionario continuo; especificación de que $\Delta T \le 15^\circ	ext{C}$ y $T_{	ext{case}} \le 45^\circ	ext{C}$ deben cumplirse de manera independiente.
- **Eliminación de Ambigüedades sobre FQP30N06L y Relés:** Descarte formal y eliminación en `README.md`, `PROJECT_CONTEXT.md`, `docs/01_ARQUITECTURA_DEL_SISTEMA.md`, `docs/02_LISTA_DE_MATERIALES_Y_COSTOS.md` y `docs/adr/ADR-001-MOSFET-SELECTION.md` de toda recomendación del MOSFET FQP30N06L como equivalente y de los módulos de relé electromecánicos, estableciendo al IRLZ44N como estándar previsto para 5V y al driver NPN+PNP como diseño previsto para 3.3V.
- **Rutas y Referencias de Prueba:** Actualización en `README.md` del comando oficial de pruebas hacia la suite completa de 75 tests (`python -m unittest discover -v -s tests`), corrigiendo referencias históricas desactualizadas.
- **Sintaxis YAML en CI (GitHub Actions):** Corrección en `.github/workflows/ci.yml` del nombre del paso de pruebas entrecomillando la cadena con dos puntos (`"Run Full Automated Test Suite (75 Tests: Domain, Security, and HTTP Auth)"`), resolviendo el fallo de validación de mapeo YAML que abortaba la ejecución remota 40285441265 en 0 segundos.
- **Alineación de Comentarios en Firmware:** Corrección de comentarios de cabecera en `src/arduino_nano_mejorado/regador_nano_pro.cpp` (eliminación de mención a FQP30N06L) y `src/esp32_mejorado_wifi/regador_esp32.cpp` (especificación de entrada a pre-driver NPN+PNP).

### Añadido
- **Matriz Ontológica y Estructura de Memoria Viva:** Reestructuración integral de `PROJECT_CONTEXT.md` conforme a los 20 requisitos de onboarding para IA y tabla de seguimiento con 6 estados inequívocos (`DOCUMENTADO`, `CALCULADO`, `VERIFICADO POR SOFTWARE`, `MEDIDO EN HARDWARE`, `PENDIENTE DE VALIDACIÓN`, `NO AUTORIZADO`), delimitando la base Git verificada (`fc4baf8`) respecto a los 10 archivos modificados en la copia local.
- **Bloqueo Formal de Operación Desatendida:** Incorporación en `docs/05_GUIA_DE_DESARROLLO_Y_FASE_A_FASE.md` y `PROJECT_CONTEXT.md` de advertencias explícitas que condicionan la puesta en marcha autónoma a la obtención y registro de las mediciones físicas de laboratorio.

---

## [1.2.0] - 2026-10-25

### Corregido
- **Bloqueo Crítico por Fallo de Pantalla OLED en Nano:** Eliminación del bucle infinito `while(true){}` en `setup()`. Se introdujo la bandera `oledAvailable`, permitiendo que el sistema continúe supervisando y regando de forma autónoma aunque la pantalla sufra un fallo de comunicación I2C.
- **Recolección de Encabezado HTTP de Autorización en ESP32:** Registro explícito de `server.collectHeaders(headerKeys, 1)` para capturar correctamente el encabezado `Authorization: Bearer <token>`.
- **Fuga de Credencial en Formulario Web:** Eliminación del token secreto como valor plano embebido en `<input type='hidden'>` en la página HTML. Se reemplazó por campo interactivo protegido (`type='password'`).
- **Inhabilitación de API ante Token Inseguro:** El endpoint `POST /api/water` rechaza con HTTP 403 Forbidden cualquier comando remoto si el sistema está ejecutando con el token predeterminado de ejemplo o una clave de longitud inferior a 16 caracteres.

### Añadido
- **Suite Completa de 30 Pruebas Automatizadas:** Expansión exhaustiva de `tests/test_irrigation_suite.py` cubriendo arranque seguro, antirrebotes, histéresis, fallos de sensor, desbordamiento de `millis()`, seguridad HTTP y garantías universales de apagado.
- **Análisis de Modos de Fallo (FMEA) F-10, F-11 y F-12:** Incorporación de análisis y mitigaciones para fallo de OLED, fuga de token y desconexión física del cable de flotador.
- **Especificación de Compuerta MOSFET y Conmutación Segura:** Documentación de constante de tiempo RC de compuerta ($\tau = 1.7\,\mu\text{s}$) y procedimiento de intercambio seguro de controladores en `docs/03_DIAGRAMA_Y_CONEXIONES.md`.

---

## [1.1.0] - 2026-10-25

### Corregido
- **PlatformIO Multi-Entorno:** Corrección del parámetro global `src_dir` que obligaba a `[env:esp32dev]` a compilar el firmware del Nano. Implementación de `build_src_filter` con aislamiento estricto por placa.
- **Bloqueos de CPU en ESP32:** Eliminación de llamadas `delay(3000)` en el endpoint web y en el botón físico, así como de esperas en bucles de muestreo. Reemplazo por máquina de estados cooperativa no bloqueante.
- **Vulnerabilidad en Servidor Web:** Reemplazo del endpoint `GET /regar` por `POST /api/water`, incorporando autorización obligatoria por token (`API_AUTH_TOKEN`) y enfriamiento mínimo (cooldown) de 5 segundos.
- **Control Fragmentado de la Bomba:** Centralización total de solicitudes de riego a través de la clase de dominio `IrrigationController`, garantizando que ninguna interfaz física o remota evada los enclavamientos de seguridad.
- **Validación de Calibración y Sensores:** Detección de calibraciones inválidas (prevención de división por cero en `map()`) y verificación de integridad física del sensor analógico (detección de desconexión y cortocircuito a masa).

### Añadido
- **Módulos de Dominio Puro:**
  - `include/SensorValidator.h`: Funciones de validación de límites físicos de ADC, coherencia matemática y cálculo de humedad.
  - `include/IrrigationController.h`: FSM centralizada con estados `IDLE`, `VALIDATING`, `WATERING`, `STOPPED`, `FAULT` y `RECOVERY`.
- **Suite de Pruebas Automatizadas:** `tests/test_irrigation_suite.py` con 11 casos de prueba cubriendo histéresis, calibración, desbordamiento de `millis()`, idempotencia de parada y tolerancia a fallos.
- **FMEA Auditado:** Actualización de `docs/FMEA_SEGURIDAD_Y_RIESGOS.md` bajo norma IEC 60812 con clasificación explícita entre mitigaciones probadas, pendientes de banco y limitaciones físicas fuera del microcontrolador.

---

## [1.0.0] - 2026-10-25

### Añadido
- **Arquitectura en Capas:** Desacoplamiento formal de Hardware, Drivers, Filtros de Señal, Máquina de Estados (FSM) y Capa de Presentación.
- **Sistema de Memoria y Contexto Vivo:** Creación de `PROJECT_CONTEXT.md` para persistir el estado de decisiones técnicas entre iteraciones de ingeniería.
- **Architecture Decision Records (ADRs):**
  - `ADR-001`: Elección de MOSFET de nivel lógico (IRLZ44N / FQP30N06L) sobre BJT Darlington (TIP120).
  - `ADR-002`: Implementación de Máquina de Estados no bloqueante basada en `millis()` y eliminación de `delay()`.
  - `ADR-003`: Adopción de arquitectura de firmware en capas con tipos de ancho explícito (`<stdint.h>`).
- **Análisis de Seguridad FMEA:** Documento `docs/FMEA_SEGURIDAD_Y_RIESGOS.md` que analiza modos de fallo (falta de agua, sobre-riego, descalibración, efecto sifón) y sus mitigaciones.
- **Estándares de Codificación:** Documento `docs/CODING_STANDARDS.md` alineado con subconjuntos seguros de MISRA C++ y Google C++ Style Guide para sistemas embebidos.
- **Soporte de Compilación Multiplataforma:** Archivo `platformio.ini` para compilación industrial con PlatformIO y toolchains GCC modernos.

### Modificado
- Refactorización de las fuentes en C++ para eliminar el uso de variables globales desordenadas y tipos genéricos `int` por tipos con ancho de bits explícito (`uint8_t`, `uint16_t`, `uint32_t`).
- Inclusión del sensor de flotador electromecánico (CS-CO058 / vertical) y zumbador piezoeléctrico pasivo TMB12A05 con resistencia atenuadora de 220 Ω.

---

## [0.2.0] - 2026-10-25

### Añadido
- Identificación y homologación de componentes en el mercado de electrónica colombiano (COP) con enlaces a Electronilab, Mecabot y RoosTech.
- Código preliminar de regador mejorado con buzzer y botón manual.

---

## [0.1.0] - 2026-10-25

### Añadido
- Ingeniería inversa completa del proyecto viral de Wirecora (YouTube Short `KClvikj9jAk`).
- Documentación base de componentes, esquemático conceptual y código base para Arduino Nano.
