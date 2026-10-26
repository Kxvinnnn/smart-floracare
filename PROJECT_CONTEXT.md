# 🧠 Contexto del Proyecto y Memoria Activa de Ingeniería (Living Context)

> **Documento Vivo de Estado, Arquitectura, Seguridad, Hardware y Trazabilidad para Onboarding de IA.**
> Este documento permite a cualquier agente de IA o ingeniero sénior retomar de inmediato el proyecto **Smart FloraCare v1.2.1** sin depender de historiales de conversación, eliminando ambigüedades entre el software probado, la seguridad de red y el hardware físico en desarrollo.

---

## 📌 1. Objetivo y Misión del Producto
- **Nombre del Proyecto:** Smart FloraCare (Regador Automático Inteligente de Maceta Única).
- **Misión de Ingeniería:** Proveer un sistema de autorriego por goteo determinista, silencioso, seguro contra inundaciones por cavitación o efecto sifón, con arquitectura desacoplada de estándar internacional, optimizado ergonómicamente para personas mayores (caso de uso de validación: abuela del creador).
- **Alcance Operativo Estricto:** Automatización de una **única maceta de interior** desde un **depósito de agua dedicado** (1.5 L a 3.0 L).
- **Actuador Único Oficial:** **Microbomba sumergible DC de 5 V (JT-DC3W)** impulsada por un riel de 5 V DC regulado externo.
- **Resolución de Alcance (H-14):** **No existe ni está prevista ninguna electroválvula de 12 V en la versión v1.2.1**. El sistema carece de rieles de 12 V y no controla válvulas solenoides.
- **Origen:** Reingeniería profunda del circuito de Wirecora (YouTube Short `KClvikj9jAk`), sustituyendo transistores Darlington con pérdidas y sensores resistivos galvánicos por control de nivel lógico, sensores capacitivos, interlocks de seguridad, API REST securizada y arquitectura en capas.

---

## 🏛️ 2. Arquitectura General y Capas del Sistema
El firmware adopta una **Arquitectura en Capas Desacoplada (Layered Architecture)**:

```text
+-------------------------------------------------------------------------------------------------+
| 5. CAPA DE PRESENTACIÓN / TELEMETRÍA (UI & Feedback)                                            |
|    - DisplayView: Interfaz OLED SSD1306 (128x64 I2C), tipografía grande y métricas claras       |
|    - AcousticAlert: Avisos discretos atenuados (850 Hz, 100 ms) vía zumbador piezoeléctrico     |
|    - WebDashboard (Solo ESP32): Servidor HTTP local con visualización de estado y formulario     |
+-------------------------------------------------------------------------------------------------+
                                                ▲
                                                │ Eventos / Mapeos
+-------------------------------------------------------------------------------------------------+
| 4. CAPA DE APLICACIÓN Y CONTROL DE DOMINIO                                                      |
|    - IrrigationController.h: FSM libre de heap gobernada por deltas de millis()                |
|    - Temporizadores de Seguridad: Límite automático de 20s y Perro Guardián absoluto de 25s     |
|    - Anti-inundación: Interbloqueo estricto por nivel de boya y período de reposo (Cooldown 5s)|
+-------------------------------------------------------------------------------------------------+
                                                ▲
                                                │ Muestras filtradas / Salud de sensor
+-------------------------------------------------------------------------------------------------+
| 3. CAPA DE PROCESAMIENTO DE SEÑALES Y SERVICIOS                                                 |
|    - SensorValidator.h: Validación analógica, plausibilidad física y cálculo inverso de humedad |
|    - SignalFilter: Sobremuestreo 16x y promedio móvil para inmunidad a ruido de conmutación     |
|    - DebounceFilter: Antirrebote no bloqueante para flotador (50 ms) y botón manual (40 ms)     |
+-------------------------------------------------------------------------------------------------+
                                                ▲
                                                │ Lecturas ADC / Control digital
+-------------------------------------------------------------------------------------------------+
| 2. CAPA DE CONTROLADORES DE DISPOSITIVOS (Drivers)                                              |
|    - PumpDriver: Accionamiento de potencia activo en HIGH con corte forzado en arranque/reset   |
|    - CapacitiveSensorDriver: Interfaz para sensor v1.2 sin corrosión                            |
|    - FloatSwitchDriver: Interfaz para contacto seco magnético en modo INPUT_PULLUP              |
+-------------------------------------------------------------------------------------------------+
                                                ▲
                                                │ HAL / Registro de pines
+-------------------------------------------------------------------------------------------------+
| 1. CAPA DE ABSTRACCIÓN DE HARDWARE (HAL / Board Target)                                         |
|    - Arduino AVR (ATmega328P, 5.0V nativo, 16 MHz, 2KB RAM, 30KB Flash)                         |
|    - Espressif ESP32 (Xtensa Dual-Core 32-bit, 3.3V nativo, 240 MHz, Wi-Fi 802.11 b/g/n)        |
+-------------------------------------------------------------------------------------------------+
```

---

## 🔌 3. Variantes de Hardware y Circuito del Driver (H-09 y H-10)

### 3.1 Arduino Nano V3 (5.0V Nativo)
- **Microcontrolador:** ATmega328P (16 MHz, lógica 5.0 V).
- **Sensor de suelo capacitivo v1.2:** Alimentado a **5.0V** (salida $0.0\,\text{V}-5.0\,\text{V}$ a `A0`).
- **Driver de bomba:** MOSFET Logic-Level **Vishay IRLZ44N** excitado directamente desde pin `D3` vía $R_{\text{gate}} = 1\,\text{k}\Omega$ y $R_{pd} = 10\,\text{k}\Omega$ a GND.
- **Diodo Flyback:** **1N5819 Schottky** ($40\,\text{V}, 1\,\text{A}$) en antiparalelo directo en bornes de la bomba.
- **Buzzer y Botón:** Buzzer pasivo en `D5` (vía $220\,\Omega$); pulsador en `D2` (`INPUT_PULLUP`); boya en `D4` (`INPUT_PULLUP`).

### 3.2 ESP32 DevKit V1 (3.3V Nativo)
- **Microcontrolador:** ESP-WROOM-32 (30 pines, lógica 3.3 V).
- **Sensor de suelo capacitivo v1.2:** **Obligatoriamente alimentado a 3.3V** (salida analógica $\le 2.9\,\text{V}$ conectada a `GPIO 34` / ADC1_CH6 para proteger el conversor del microcontrolador).
- **Driver de bomba (Etapa Bipolar No Inversora NPN+PNP de 2 Etapas):**
  - **Q1 (Transistor NPN - Traslador de Nivel - CRIT-01 / MEN-05):**
    * *Secuencia Mandatoria de Identificación (6 Pasos):* (1) Registrar marcaje exacto; (2) Identificar fabricante y referencia (`P2N2222A` C-B-E vs `PN2222A` E-B-C); (3) Consultar hoja de datos oficial exacta; (4) Confirmar orientación física por plano del fabricante; (5) Comprobar uniones PN con multímetro en modo diodo como verificación complementaria (sin asumir desigualdad universal entre $V_{BE}$ y $V_{BC}$); y (6) Registrar el resultado antes de montar o energizar.
    * Base: Conectada a `GPIO 18` vía $R_1 = 2.2\,\text{k}\Omega$; pull-down $R_{pd1} = 10\,\text{k}\Omega$ a GND.
    * Emisor: A GND común.
    * Colector: A Base de Q2 vía $R_2 = 2.2\,\text{k}\Omega$.
  - **Q2 (Transistor PNP - Conmutador de Potencia de Compuerta a +5V - CRIT-01 / MEN-05):**
    * *Secuencia Mandatoria de Identificación:* Igual procedimiento de 6 pasos con hoja de datos primaria antes del montaje.
    * Emisor: Al riel de potencia de `+5V`.
    * Base: Conectada al Colector de Q1 vía $R_2 = 2.2\,\text{k}\Omega$; pull-up $R_{pu} = 10\,\text{k}\Omega$ a `+5V`.
    * Colector: Al Gate del MOSFET Q3.
  - **Q3 (N-MOSFET Vishay IRLZ44N TO-220):**
    * Gate: Conectado a Colector de Q2; pull-down de descarga $R_{pd2} = 10\,\text{k}\Omega$ a GND.
    * Drain: Conectado al polo negativo de la microbomba.
    * Source: A GND común.
  - **Comportamiento Seguro:** Durante boot, reset o desconexión del GPIO, $R_{pd1}$ mantiene Q1 en corte $\to$ $R_{pu}$ mantiene Q2 en corte $\to$ $R_{pd2}$ drena la compuerta de Q3 a $0\,\text{V}$ $\implies$ Bomba incondicionalmente **OFF**.
  - **Excitación en Activo:** Cuando GPIO 18 pasa a 3.3V, Q1 y Q2 saturan profundamente, elevando la compuerta a $V_{GS} \approx 4.85\,\text{V} - 4.90\,\text{V}$, garantizando que el IRLZ44N opere dentro de su especificación de $R_{DS(on)} \le 0.035\,\Omega$.
  - **Condicionamiento de Diagnóstico Nodal (H-11):**
    * N2 ($0.70\,\text{V}$) solo confirma polarización de juntura; la saturación se evalúa con N3 ($V_{CE1} \le 0.20\,\text{V}$).
    * N4 mide $\approx 4.30\,\text{V}$ en conducción normal ($5.0\,\text{V} - V_{EB2}$), o $\approx 0.98\,\text{V}$ bajo divisor Thévenin pasivo si la base no carga el nodo.
    * N5 ($V_{GS} \ge 4.50\,\text{V}$) demuestra sobrevoltaje de compuerta pero no conducción de carga; la conducción se valida en N6 ($V_{DS} \le 50\,\text{mV}$ bajo carga nominal en agua).
    * N6 mide $+5.0\,\text{V}$ en reposo únicamente mediante pull-up de la carga conectada (LED o motor).

---

## 🛡️ 4. Seguridad HTTP y Limitaciones de Memoria en ESP32 (SW-02)

### 4.1 Mecanismos de Seguridad Implementados
1. **Autenticación Bearer y Formulario:** Exige token secreto en cabecera `Authorization: Bearer <TOKEN>` o en cuerpo `token=<TOKEN>`.
2. **Comparación en Tiempo Constante (`safeConstantTimeEquals`):** Implementa comparación bit a bit con acumulador volátil XOR sobre la longitud de entrada, reduciendo diferencias temporales basadas en el primer carácter discrepante.
3. **Validación Canónica de Origen (RFC 6454 / RFC 7231):** Función `validateSingleAuthority()` exige esquema estricto `http://`, compara el Host exclusivamente contra la IP local asignada `WiFi.localIP()` y restringe el puerto estrictamente a `80`.
4. **Control de Tasa No Bloqueante (Rate Limiting):** Marca temporal `lastFailedAuthMs` que rechaza peticiones sucesivas con `429 Too Many Requests` durante 1000 ms tras un intento fallido, sin retardos bloqueantes.
5. **Guardia Temprana de Carga Útil:** Rechaza peticiones con `clientContentLength() > 256` con código `413 Payload Too Large`.

### 4.2 Limitaciones Arquitecturales y Riesgo Residual
- **Asignación Previa de Heap en WebServer:** La biblioteca síncrona `WebServer` de Arduino-ESP32 procesa sockets TCP y ejecuta `malloc()` para almacenar cabeceras antes de transferir el control a `handleApiWater()`. Por lo tanto, el código de aplicación no puede prevenir ataques de denegación de servicio a nivel de red (Slowloris o agotamiento de sockets).
- **Mitigación Mandatoria:** El dispositivo **debe operar exclusivamente en una red local privada (VLAN IoT)**, con prohibición absoluta de reenvío de puertos WAN a Internet.

---

## 📊 5. Estado Actual de Git, Ramas, CI y Compilación (OBL-03 / CI-01)

- **Rama Base Estable:** `main`
  * **Commit Base de Referencia:** `45fff8516f0996d9008c5fbe73a0fcde5ecbccf9` (`fix(ci): quote test step name to fix YAML syntax error in workflow`)
  * **Estado de CI en GitHub Actions (`main`):** Run ID `40367863193` — **`SUCCESS`** (Completado en 1m 6s el 2026-10-26T17:21:35Z).
- **Rama de Revisión y Auditoría Activa:** `audit/documentation-review`
  * **Commits Previos Auditados en GitHub:**
    - `bca15cd9a76b83f9f1ea6b8528a92878eb66e2d4` (Primera auditoría: CRIT-01, OBL-01..03, MEN-01..03).
    - `7576c0fb10153634004ff62a6c9ab48ecfcb60a5` (Segunda auditoría: OBL-04 separación 20s/25s, CRIT-01/MEN-05 secuencia 6 pasos, MEN-04 medición térmica sin contacto, CI-01).
  * **Estado de CI en rama de auditoría (CI-01):** Flujo `.github/workflows/ci.yml` actualizado con `branches: [ main, "audit/**" ]` para ejecutar automáticamente las **76 pruebas automatizadas** y la compilación dual (`nanoatmega328` y `esp32dev`) en cada push a `audit/documentation-review`.
- **Métricas de Compilación PlatformIO:**
  - `nanoatmega328`: Flash: ~57.2%, RAM: 30.0% — **SUCCESS**
  - `esp32dev`: Flash: ~82.6%, RAM: 15.1% — **SUCCESS**
- **Estado de Pruebas Físicas e Integración Eléctrica:**
  - Protocolo ACT-01 a ACT-06 formalizado como `LISTA PARA BANCO`, pero **estrictamente pendiente de ejecución y medición física en silicio**.
  - **Circuito físico:** Completamente desenergizado (0 V).
  - **Microbomba sumergible JT-DC3W:** Físicamente desconectada de la etapa de potencia.

---

## 🧪 6. Jerarquía y Taxonomía Formal de Pruebas (SW-01 / OBL-04)

La suite del repositorio contiene un total de **76 pruebas automatizadas**, clasificadas formalmente en:

```text
+-------------------------------------------------------------------------------------------------+
|                        TAXONOMÍA DE PRUEBAS DE SMART FLORACARE v1.2.1                           |
+-------------------------------------------------------------------------------------------------+
| 1. Pruebas de Dominio Lógico (Nivel A): 31 pruebas                                              |
|    - Archivo: tests/test_irrigation_suite.py                                                    |
|    - Valida: FSM de IrrigationController, antirrebote, límite auto 20s (test_16 con             |
|      AUTO_MAX_DURATION_REACHED), watchdog aislado 25s (test_31 con SAFETY_TIMEOUT),             |
|      cooldown (5s), desbordamiento de millis(), calibración y corte por falta de agua.          |
|    - Naturaleza: Código Python puro que emula las estructuras de dominio C++.                   |
|    - Limitación: NO demuestra que el hardware físico funcione.                                  |
+-------------------------------------------------------------------------------------------------+
| 2. Pruebas de Análisis Estático de Firmware (Nivel B1): 12 pruebas                              |
|    - Archivo: tests/test_firmware_security.py                                                   |
|    - Valida: Cero credenciales hardcodeadas, recolección de cabeceras HTTP, polaridad           |
|      INPUT_PULLUP de boya, presencia de códigos 4xx/5xx, StopReasons diferenciados y            |
|      ausencia de bucles infinitos.                                                              |
|    - Naturaleza: Expresiones regulares e inspección de texto sobre archivos C++.                |
|    - Limitación: NO valida el comportamiento en tiempo de ejecución en silicio.                 |
+-------------------------------------------------------------------------------------------------+
| 3. Pruebas HTTP Simuladas con Mocks (Nivel B2): 33 pruebas                                      |
|    - Archivo: tests/test_http_auth_security.py                                                  |
|    - Valida: Modelo MockHttpServerEndpoint con lógica de handleApiWater(), token Bearer,        |
|      reglas RFC 6454 / RFC 7231 de Origin/Referer, rate limiting 429, payload 413, y            |
|      Test 32 (14 subescenarios de ataque donde la bomba permanece apagada).                     |
|    - Naturaleza: Modelo de prueba en Python que simula peticiones HTTP.                         |
|    - Limitación: NO invoca el servidor WebServer real ni prueba asignaciones de heap en silicio.|
+-------------------------------------------------------------------------------------------------+
| 4. Compilaciones Reproducibles en CI: Dual-Target PlatformIO                                    |
|    - nanoatmega328 y esp32dev compilan con 0 errores.                                           |
|    - Limitación: La compilación exitosa no es una prueba funcional ni certifica el circuito.    |
+-------------------------------------------------------------------------------------------------+
| 5. Protocolo de Pruebas Físicas en Banco Instrumentado: ACT-01 a ACT-06                        |
|    - Archivo: docs/05_GUIA_DE_DESARROLLO_Y_FASE_A_FASE.md                                      |
|    - Estado actual: LISTA PARA BANCO (Ninguna prueba ha sido ejecutada en silicio todavía).     |
|    - Requisito obligatorio: Ejecución supervisada en laboratorio antes de operar desatendido.  |
+-------------------------------------------------------------------------------------------------+
```

---

## 🔬 7. Estado del Protocolo Físico ACT-01 a ACT-06 (TEST-01)

| Código | Objetivo Técnico | Criterio de Aceptación Cuantitativo | Instrumento | Estado Actual |
| :---: | :--- | :--- | :--- | :---: |
| **ACT-01** | Estado seguro del driver en boot/reset | $V_{GS} \le 0.10\,\text{V}$ en reposo, boot y reset; $I_{leak} < 10\,\mu\text{A}$ | DMM + Osciloscopio | `LISTA PARA BANCO` |
| **ACT-02** | Excitación de compuerta a 3.3V | $V_{GS} \ge 4.50\,\text{V}$ bajo carga ($15\,\Omega$, $5\,\text{W}$); $T_{\text{res}} \le 60^\circ\text{C}$ sin contacto | DMM + Termocupla/IR | `LISTA PARA BANCO` |
| **ACT-03** | Conmutación real y estabilidad térmica | $V_{DS} \le 50\,\text{mV}$, $\Delta T \le 15^\circ\text{C}$ tras 20s en agua | DMM + Termómetro IR | `LISTA PARA BANCO` |
| **ACT-04** | Supresión de transitorios inductivos | $V_{\text{peak}} \le 6.0\,\text{V}$ en Drain con diodo 1N5819 (trigger $+4.50\,\text{V}$) | Osciloscopio $\ge 20\,\text{MHz}$ | `LISTA PARA BANCO` |
| **ACT-05** | Enclavamiento fail-safe de boya | Corte en $\le 50\,\text{ms}$, $V_{GS} \le 0.10\,\text{V}$ ante fallo/desconexión | DMM + Depósito | `LISTA PARA BANCO` |
| **ACT-06** | Límites 20s (A1), Watchdog 25s (A2) y HTTP (B) | A1: $20\pm 0.5\,\text{s}$ (`AUTO_MAX_DURATION_REACHED`); A2: $25\pm 0.5\,\text{s}$ (`SAFETY_TIMEOUT`); B: 7 HTTP y $\Delta Heap \le 200\,\text{B}$ | Cronómetro + cURL LAN | `LISTA PARA BANCO` |

> [!CAUTION]
> **Prohibición Estricta de Operación Desatendida:**
> El sistema **no debe energizarse de forma desatendida** bajo ninguna circunstancia hasta que los 6 ensayos ACT-01 a ACT-06 cuenten con registros numéricos firmados en el cuaderno de laboratorio.

---

## 📋 8. Guía de Onboarding para la Siguiente IA
Si eres un agente de IA que retoma este proyecto:
1. **Comprueba el estado de Git:** Verifica la rama activa (`main` o `audit/documentation-review`) y ejecuta `git status`.
2. **No confundas pruebas de software con validación física:** Las 76 pruebas automatizadas demuestran robustez lógica y sintáctica, pero **las pruebas físicas ACT-01 a ACT-06 siguen pendientes de ejecución instrumental**.
3. **Ten en cuenta las restricciones de diseño:**
   - La bomba es de 5 V (JT-DC3W). No hay electroválvulas de 12 V.
   - El sensor capacitivo en ESP32 DEBE ir a 3.3V (pin 34).
   - El driver ESP32 utiliza la etapa de 2 transistores NPN+PNP para excitar el IRLZ44N a 4.85V.
4. **Siguiente paso de ingeniería:** Asistir al usuario en la ejecución paso a paso de la prueba **ACT-01** en el banco de prototipado sin carga.
