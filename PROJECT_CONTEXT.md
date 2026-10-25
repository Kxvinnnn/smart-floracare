# 🧠 Contexto del Proyecto y Registro de Estado (Living Context)

> **Documento Vivo de Estado y Contexto de Ingeniería.**  
> Este archivo mantiene la memoria activa, decisiones técnicas, estado de avance, estándares aplicados y hoja de ruta para contextualización inmediata de cualquier agente de IA o desarrollador.

---

## 📌 1. Visión y Alcance del Producto

- **Nombre del Proyecto:** Smart FloraCare (Regador Automático Inteligente de Plantas).
- **Misión:** Proporcionar un sistema de autorriego por goteo robusto, seguro, con arquitectura de software embebido modular de estándar internacional, optimizado para ser confiable, silencioso, seguro contra inundaciones y extremadamente fácil de entender y usar para personas mayores (caso de uso específico: abuela del creador).
- **Origen:** Ingeniería inversa del diseño de Wirecora (YouTube Short `KClvikj9jAk`), perfeccionado con componentes de nivel lógico, sensores físicos de protección de bomba, avisos acústicos discretos de frecuencia calibrada y arquitectura en capas.

---

## 🏛️ 2. Arquitectura del Sistema (Estándar Internacional)

El firmware adopta una **Arquitectura en Capas (Layered Embedded Architecture)** desacoplada:

```text
+-------------------------------------------------------------+
| 5. Capa de Presentación (UI / Audio Feedback)               |
|    - DisplayView (OLED SSD1306, iconos grandes, telemetría) |
|    - AcousticAlert (Buzzer pasivo piezoeléctrico, tone())   |
+-------------------------------------------------------------+
| 4. Capa de Aplicación / Dominio                             |
|    - IrrigationController (FSM no bloqueante con millis)     |
|    - SafetyWatchdog (Anti-inundación, interlock de nivel)   |
+-------------------------------------------------------------+
| 3. Capa de Servicios y Procesamiento de Señales             |
|    - SignalFilter (Sobremuestreo 16x, promediado móvil)     |
|    - DebounceFilter (Anti-rebote para botón y flotador)     |
+-------------------------------------------------------------+
| 2. Capa de Controladores de Dispositivos (Drivers)          |
|    - PumpDriver (MOSFET IRLZ44N/FQP30N06L con soft-cutoff)   |
|    - CapacitiveSensorDriver (Sensor v1.2, mapeo calibrado)  |
|    - FloatSwitchDriver (Interruptor magnético hermético)    |
+-------------------------------------------------------------+
| 1. Capa de Abstracción de Hardware (HAL / Board Target)     |
|    - Arduino Nano (ATmega328P 5V) / ESP32 DevKit V1 (3.3V)  |
+-------------------------------------------------------------+
```

---

## 🔧 3. Decisiones de Hardware y Componentes Definitivos

| Subsistema | Componente Seleccionado | Justificación de Ingeniería |
| :--- | :--- | :--- |
| **Driver de Potencia** | **MOSFET IRLZ44N** o **FQP30N06L** | *Logic-Level Gate*: se satura con $\ge 3.3V$. Resistencia $R_{DS(on)} < 0.03\,\Omega$. Caída $< 0.05V$. Cero disipación térmica. |
| **Sensor de Suelo** | **Capacitivo v1.2** | Sin electroquímica galvánica (cero sulfatación). Barnizado de componentes superiores con resina/laca. |
| **Nivel de Depósito** | **Flotador Horizontal (CS-CO058)** o **Vertical** | Detección electromecánica infalible por contacto seco con `INPUT_PULLUP`. Previene cavitación y marcha en seco. |
| **Alarma Acústica** | **Buzzer Pasivo TMB12A05** + R=220 Ω | Frecuencia grave a 850 Hz vía `tone()`. Atenuación acústica de confort para no perturbar a adultos mayores. |
| **Interacción Manual** | **Pulsador Tact 6x6 mm** (4 pines) | Riego manual de 3 segundos bajo demanda. Conexión diagonal a GND con `INPUT_PULLUP`. |
| **Protección Motor** | **Diodo 1N4007** (Flyback) | Supresión de fuerza contraelectromotriz (fem inductiva inversa). |

---

## 📜 4. Estándares y Convenciones de Ingeniería Adoptadas

1. **Estilo y Calidad de Código:**
   - Tipos de datos con ancho explícito (`uint8_t`, `uint16_t`, `uint32_t`) según estándar MISRA-C / ISO C99/C++11.
   - Prohibido el uso de memoria dinámica en ejecución (`malloc`, `free`, `new`, `String` en microcontroladores de 2KB RAM). Uso exclusivo de buffers estáticos o tipos primitivos.
   - Prohibición de funciones bloqueantes (`delay()`) en el lazo principal; uso estricto de temporización por deltas con `millis()`.
2. **Control de Versiones y Documentación:**
   - Semantic Versioning (SemVer 2.0.0): `MAJOR.MINOR.PATCH`.
   - Keep a Changelog para trazabilidad histórica.
   - Architecture Decision Records (ADRs) para cada cambio de diseño significativo.
   - Análisis modal de fallos y efectos (FMEA).

---

## 📈 5. Registro Histórico de Versiones y Estado de Hitos

- [x] **v0.1.0:** Ingeniería inversa del video de Wirecora y extracción del esquema original.
- [x] **v0.2.0:** Rediseño con MOSFET de nivel lógico, sensor de flotador, buzzer suave y botón manual.
- [x] **v1.0.0:** Formalización de estándares de ingeniería internacional, estructura modular en capas, ADRs, FMEA, archivo de contexto vivo (`PROJECT_CONTEXT.md`) y configuración multiplataforma PlatformIO.
- [x] **v1.1.0 (Auditoría Técnica y Corrección de Defectos):**
  - Corrección de `platformio.ini` (aislamiento explícito de entornos con `build_src_filter`).
  - Creación de dominio centralizado desacoplado de hardware: `include/IrrigationController.h` y `include/SensorValidator.h`.
  - Eliminación total de llamadas bloqueantes `delay()` en el firmware ESP32.
  - Aseguramiento de API REST: transición de GET inseguro a POST con token de autorización en `/api/water` y rate-limiting de 5 segundos.
  - Creación de suite automatizada de 11 pruebas unitarias (`tests/test_irrigation_suite.py`) cubriendo histéresis, calibración, desbordamiento de temporizadores, idempotencia de parada y tolerancia a fallos.
  - Actualización rigurosa de FMEA (IEC 60812) diferenciando mitigaciones probadas, pendientes y limitaciones físicas no mitigables por software.
