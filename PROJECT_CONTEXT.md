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
- **Driver de bomba:** MOSFET Logic-Level **Infineon / IR `IRLZ44N`** (`PD-91338`, $R_{DS(on)} \le 0.025\,\Omega$ a $5.0\,\text{V}$ y $\le 0.035\,\Omega$ a $4.0\,\text{V}$ a $T_J=25^\circ\text{C}$) excitado desde `D3` vía $R_{\text{gate}} = 1\,\text{k}\Omega$ y $R_{pd} = 10\,\text{k}\Omega$ a GND (conmutación estática ON/OFF, sin PWM).
- **Diodo Flyback:** **1N5819 Schottky** ($40\,\text{V}, 1\,\text{A}$) en antiparalelo directo en bornes de la bomba.
- **Buzzer y Botón:** Buzzer pasivo en `D5` (vía $220\,\Omega$); pulsador en `D2` (`INPUT_PULLUP`); boya en `D4` (`INPUT_PULLUP`).

### 3.2 ESP32 DevKit V1 (3.3V Nativo)
- **Microcontrolador:** ESP-WROOM-32 (30 pines, lógica 3.3 V).
- **Sensor de suelo capacitivo v1.2:** **Obligatoriamente alimentado a 3.3V** (conectado a `GPIO 34` / ADC1_CH6).
  - *Criterio de Recepción e Inspección:* Selección de lote en recepción sujeta a $\bar{V}_{\text{aire}} \le 2{,}900\,\text{V}$ (según protocolo instrumental en `docs/04_CALIBRACION_Y_CONFIGURACION.md`).
  - *Aclaración Metrológica:* El valor de $2{,}900\,\text{V}$ representa un umbral interno adoptado como margen de diseño respecto a la alimentación nominal de $3{,}3\,\text{V}$ para asegurar holgura en el control de calidad, sin constituir una garantía de linealidad absoluta del conversor ADC del SoC.
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
  - **Q3 (N-MOSFET Infineon / IR `IRLZ44N`, Doc. `PD-91338`, TO-220):**
    * Gate: Conectado a Colector de Q2; pull-down de descarga $R_{pd2} = 10\,\text{k}\Omega$ a GND (uso exclusivo en conmutación estática ON/OFF; prohibido PWM).
    * Drain: Conectado al polo negativo de la microbomba.
    * Source: A GND común.
  - **Comportamiento Pasivo en Arranque:** Durante boot, reset o desconexión de `GPIO 18`, $R_{pd1}$, $R_{pu}$ y $R_{pd2}$ referencian la compuerta de Q3 a $0\,\text{V}$ (verificación física pendiente en `ACT-01.1` con DMM y `ACT-01.2` con osciloscopio).
  - **Excitación en Activo (Modelo Analítico Condicional):** Con `GPIO 18` en alto (`onsemi PN2222A/D` + `onsemi 2N3906/D`, resistencias $\pm 5\%$), el modelo arroja $\beta_{f,Q1,\text{máx}} = 3.05 < 10$ y $\beta_{f,Q2,\text{máx}} = 0.372 \ll 10$, proyectando $V_{GS,\text{nom}} \approx 4.90\,\text{V}$ y $V_{GS,\text{mín(modelo)}} = 4.50\,\text{V}$ bajo las hipótesis adoptadas ($V_{CC,\text{mín}} = 4.75\,\text{V}$, $|V_{EC2(sat)}| = 0.25\,\text{V}$ y $V_{CE1(sat)} \le 0.30\,\text{V}$). Se distingue estrictamente entre: (1) *resultado matemático del modelo* ($4.90\,\text{V}$ nom. / $4.50\,\text{V}$ peor caso condicional), (2) *límite garantizado por fabricante* (exclusivo para los puntos discretos tabulados en `PN2222A/D`, `2N3906/D` y `PD-91338`, sin interpolación contractual de $R_{DS(on)}$ entre $4.0\,\text{V}$ y $5.0\,\text{V}$) y (3) *resultado experimental* medido en el montaje físico (`ACT-02` y `ACT-03.1`).
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

## 📊 5. Estado Actual de Git, CI, Presupuesto y Validación Física (OBL-03 / CI-01)

- **Rama Principal Consolidada:** `main`
  * **Paquete Base de Auditoría Técnica Integrado:** `a6af86a` (`docs(audit): update electrical models, metrology criteria, and bench protocols (ACT-01..06)`).
  * **Comprobación Dinámica del HEAD:** El estado actual del árbol de trabajo debe consultarse dinámicamente con `git rev-parse HEAD` y `git status` sobre la rama `main` para considerar cualquier commit de mantenimiento documental posterior sin recurrir a referencias estáticas obsoletas.
  * **Estado de CI en GitHub Actions (`main`):** Verificado en **`SUCCESS`** (ejecuta los 2 binarios C++ nativos de `tests/test_native_controller.cpp`, las 78 pruebas en Python y compila los 4 entornos PlatformIO: `nanoatmega328`, `esp32dev`, `nanoatmega328_bench_wdt25s` y `esp32dev_bench_wdt25s`).
  * **Distinción crítica de alcance:** El éxito de GitHub Actions certifica únicamente la compilación cruzada y las pruebas de software/mocks; **no certifica ni sustituye la validación eléctrica ni térmica del montaje físico**.
- **Presupuesto Reconciliado por Escenario (`docs/02_LISTA_DE_MATERIALES_Y_COSTOS.md`):**
  * **Compra Autónoma Solo Nano:** $\$122,100$ producción $+\,\$2,500$ carga banco $= \mathbf{\$124,600\,\text{COP}}$ *(subtotal contable prorrateado al 50% de kits: $\$117,600\,\text{COP}$)*.
  * **Compra Autónoma Solo ESP32 (Driver BJT):** $\$133,600$ producción $+\,\$2,500$ carga banco $= \mathbf{\$136,100\,\text{COP}}$ *(subtotal contable prorrateado: $\$129,100\,\text{COP}$)*.
  * **Suite Dual Completa (2 placas armadas completas):** $\$209,200$ producción $+\,\$2,500$ carga banco $= \mathbf{\$211,700\,\text{COP}}$ *(no incluye adquisición de instrumentos de medición ni envíos)*.
- **Estado Físico y de Aprovisionamiento:**
  - **Componentes:** No adquiridos todavía ($0$ ensayos físicos ejecutados).
  - **Circuito físico:** Desenergizado ($0\,\text{V}$); microbomba desconectada.

---

## 🧪 6. Jerarquía y Taxonomía Formal de Pruebas (SW-01 / OBL-04 / NUEVO-01)

La suite del repositorio ejecuta **2 binarios C++ nativos** y **78 pruebas automatizadas en Python**, además de **4 compilaciones cruzadas PlatformIO**, clasificadas formalmente en:

```text
+-------------------------------------------------------------------------------------------------+
|                        TAXONOMÍA DE PRUEBAS DE SMART FLORACARE v1.2.1                           |
+-------------------------------------------------------------------------------------------------+
| 1A. Ejecución Nativa del Controlador C++ Real (tests/test_native_controller.cpp + test_32):     |
|    - Compila con g++ -std=c++17 y ejecuta directamente include/IrrigationController.h           |
|      e include/SensorValidator.h en dos binarios nativos independientes:                        |
|      * Modo Producción: verifica corte automático a 20.000 ms (AUTO_MAX_DURATION_REACHED).      |
|      * Modo Aislado (-DFLORACARE_BENCH_TEST_WATCHDOG_25S): verifica permanencia en WATERING     |
|        a 20.000 ms / 24.999 ms y corte de watchdog a 25.000 ms (SAFETY_TIMEOUT).                |
|    - Limitación: Ejecuta en CPU host de CI; NO sustituye el ensayo eléctrico en la placa real.  |
+-------------------------------------------------------------------------------------------------+
| 1B. Pruebas de Dominio Lógico Simulado en Python (Nivel A): 32 pruebas (31 mocks + 1 bridge C++)|
|    - Archivo: tests/test_irrigation_suite.py                                                    |
|    - Valida: Equivalencia lógica en MockIrrigationController (test_01..test_31, incluyendo      |
|      test_16 para 20s y test_31 para 25s simulado) y puente a C++ nativo (test_32).             |
|    - Naturaleza: test_01..test_31 usan un mock Python; NO deben presentarse por sí solos        |
|      como ejecución directa del firmware C++ (NUEVO-01).                                        |
+-------------------------------------------------------------------------------------------------+
| 2. Pruebas de Análisis Estático de Firmware (Nivel B1): 13 pruebas                              |
|    - Archivo: tests/test_firmware_security.py                                                   |
|    - Valida: Cero credenciales hardcodeadas, recolección de cabeceras HTTP, polaridad           |
|      INPUT_PULLUP de boya, presencia de códigos 4xx/5xx, StopReasons diferenciados,             |
|      entornos de banco en platformio.ini y ausencia de bucles infinitos.                        |
|    - Naturaleza: Expresiones regulares e inspección de texto sobre archivos C++ y config.       |
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
| 4. Compilaciones Reproducibles en CI: 4 Entornos PlatformIO (Producción + Banco Aislado)        |
|    - Producción: nanoatmega328 y esp32dev.                                                      |
|    - Banco Aislado (ACT-06.A2): nanoatmega328_bench_wdt25s y esp32dev_bench_wdt25s              |
|      (compilados explícitamente con -D FLORACARE_BENCH_TEST_WATCHDOG_25S).                      |
|    - Limitación: La compilación exitosa no es una prueba funcional ni certifica el circuito.    |
+-------------------------------------------------------------------------------------------------+
| 5. Protocolo de Pruebas Físicas en Banco Instrumentado: ACT-01 a ACT-06                        |
|    - Archivo: docs/05_GUIA_DE_DESARROLLO_Y_FASE_A_FASE.md                                      |
|    - Estado actual: PENDIENTE DE BANCO / LAB EXTERNO (0 pruebas físicas ejecutadas todavía).    |
|    - Requisito obligatorio: Ejecución supervisada en laboratorio antes de operar desatendido.  |
+-------------------------------------------------------------------------------------------------+
```

---

## 🔬 7. Estado del Protocolo Físico ACT-01 a ACT-06 y Requisitos de Evidencia (TEST-01)

| Código | Objetivo Técnico | Criterio de Aceptación Cuantitativo | Instrumento Obligatorio | Estado Actual |
| :---: | :--- | :--- | :--- | :---: |
| **`ACT-01.1`** | Corte estático preliminar en boot/reset/hilo suelto | $V_{GS} \le 0.10\,\text{V}$ en DMM y LED testigo OFF (no cierra `ACT-01` por sí sola) | DMM + LED ($330\,\Omega$) | `PENDIENTE DE MEDICIÓN EN BANCO` |
| **`ACT-01.2`** | Captura determinista Modo 1 (11 trazas) + Vigilancia Modo 2 | Modo 1: $V_{GS(\text{peak})} \le 0.50\,\text{V}$ ($<0.80\,\text{V}$); Modo 2: `0` disparos $\ge +0.50\,\text{V}$ | Osciloscopio $\ge 20\,\text{MHz}$ (2CH) | `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` |
| **`ACT-02`** | Excitación estática ($15\,\Omega, 5\,\text{W}$, pulso manual $3\,\text{s}$) | $V_{GS} \ge 4.50\,\text{V}$ y criterio experimental $V_{DS} \le 15.0\,\text{mV}$ ($>15\,\text{mV}$ incumple; $>30\,\text{mV}$ aborta) | DMM (exactitud registrada) + Termómetro | `PENDIENTE DE MEDICIÓN EN BANCO` |
| **`ACT-03.1`** | Conmutación estacionaria con bomba sumergida ($20\,\text{s}$) | $I_{\text{est}} \le 400\,\text{mA}$, $V_{DS} \le 50\,\text{mV}$, $\Delta T \le 15^\circ\text{C}$, $T_{\text{case}} \le 45^\circ\text{C}$ | DMM + Termómetro + Agua | `PENDIENTE DE MEDICIÓN EN BANCO` |
| **`ACT-03.2`** | Arranque dinámico de bomba (shunt bajo lado en `Source` a `GND_REF`) | $V_{CC,\text{mín(din)}} \ge 4.65\,\text{V}$ e $I_{\text{arranque}} = V_{\text{shunt,peak}}/R_{\text{shunt,real}} \le 1.20\,\text{A}$ | Osciloscopio 2CH + Shunt $0.10\,\Omega$ / Sonda Hall | `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` |
| **`ACT-04`** | Recorte inductivo Flyback y descarga de compuerta | $V_{DS(\text{peak})} \le V_{CC}+0.60\,\text{V}$ ($\le 6\,\text{V}$), rebote Gate $<0.80\,\text{V}$, registro $t_{f(G)}$ | Osciloscopio $\ge 20\,\text{MHz}$ (2 sondas 10X) | `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` |
| **`ACT-05.1`** | Bloqueo estacionario `FAULT` ante boya baja o hilo abierto | Pin `D4`/`GPIO 19` pasa a `HIGH`, corte y rechazo de riego manual/web | Boya + DMM + OLED/Serie | `PENDIENTE DE ENSAYO FÍSICO` |
| **`ACT-05.2`** | Tensión estacionaria de corte en compuerta en fallo | $V_{GS} \le 0.10\,\text{V}$ en estado `FAULT` | DMM en Nodo N5 | `PENDIENTE DE MEDICIÓN EN BANCO` |
| **`ACT-05.3a`** | Latencia lógica (`D4`/`GPIO 19` $\to$ `D3`/`GPIO 18`) | Orden digital de apagado $\le 60\,\text{ms}$ (no demuestra apagado eléctrico) | Analizador lógico 2CH u Osciloscopio 2CH | `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` |
| **`ACT-05.3b`** | Latencia eléctrica en 2 capturas independientes (`Drain` y `Gate`) | `ACT-05.3b-1` ($\Delta t_{\text{Drain}} \le 80\,\text{ms}$) y `ACT-05.3b-2` ($\Delta t_{\text{Gate}} \le 80\,\text{ms}$) | **Osciloscopio 2CH** (2 capturas obligatorias) | `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` |
| **`ACT-06`** | Límites $20\,\text{s}$ (`A1`), Watchdog $25\,\text{s}$ (`A2`) y HTTP (`B`) | A1: $20\pm 0.5\,\text{s}$; A2: $25\pm 0.5\,\text{s}$ + reflasheo prod; B: 7 HTTP y $\Delta Heap \le 200\,\text{B}$ | Cronómetro + DMM + cURL LAN | `PENDIENTE DE MEDICIÓN EN BANCO` |

### Requisitos Obligatorios para Cambiar el Estado de una Prueba Física
Ninguna prueba podrá marcarse como aprobada sin registrar: (1) fecha, variante (`Nano` o `ESP32`) y commit SHA cargado; (2) marca, modelo, rango, resolución, **exactitud especificada en ese rango** y estado de verificación/calibración de cada instrumento; (3) auditoría óhmica en frío ($\pm 5\%$) y pinout verificado por hoja técnica; y (4) valores numéricos completos y capturas `.PNG`/`.CSV` de osciloscopio para `ACT-01.2`, `ACT-03.2`, `ACT-04` y `ACT-05.3b`.

> [!CAUTION]
> **Prohibición Estricta de Operación Desatendida:**
> El sistema **no debe energizarse de forma desatendida** bajo ninguna circunstancia hasta que todas las sub-verificaciones de `ACT-01` a `ACT-06` cuenten con registros numéricos y capturas firmadas en el cuaderno de laboratorio.

---

## 📋 8. Guía de Onboarding para la Siguiente IA
Si eres un agente de IA que retoma este proyecto:
1. **Comprueba el estado de Git:** Consulta dinámicamente el HEAD de `main` con `git rev-parse HEAD` y confirma la limpieza del árbol con `git status`. El paquete base de auditoría técnica v1.2.1 parte de `a6af86a`.
2. **No confundas pruebas de software con validación física:** Los 2 ejecutables C++ nativos (`tests/test_native_controller.cpp`) y las 78 pruebas automatizadas en Python demuestran robustez lógica y sintáctica, pero **todas las pruebas físicas `ACT-01` a `ACT-06` siguen pendientes**.
3. **Ten en cuenta las restricciones de diseño y pines vigentes:**
   - La bomba es de 5 V (`JT-DC3W`); no hay electroválvulas de 12 V. Conmutación estrictamente ON/OFF (sin PWM).
   - Pines ESP32: Bomba = `GPIO 18`, Flotador = `GPIO 19`, Botón = `GPIO 4`, Buzzer = `GPIO 23`, Sensor (a `3.3V`!) = `GPIO 34`, OLED = `GPIO 21/22`.
   - Pines Nano: Bomba = `D3`, Flotador = `D4`, Botón = `D2`, Buzzer = `D5`, Sensor = `A0`, OLED = `A4/A5`.
4. **Siguiente paso de ingeniería:** Tras adquirir componentes, ejecutar primero la auditoría óhmica y de pinout en frío ($0\,\text{V}$) antes de `ACT-01.1`.
