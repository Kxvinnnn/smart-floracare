# Changelog

Todos los cambios notables de este proyecto están documentados en este archivo.

El formato se basa en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/)
y este proyecto se adhiere a [Semantic Versioning](https://semver.org/lang/es/).

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
