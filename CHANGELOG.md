# Changelog

Todos los cambios notables de este proyecto están documentados en este archivo.

El formato se basa en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/)
y este proyecto se adhiere a [Semantic Versioning](https://semver.org/lang/es/).

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
