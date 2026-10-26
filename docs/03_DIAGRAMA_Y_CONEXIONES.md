# ⚡ 03. Diagrama Esquemático y Mapa de Conexiones Eléctricas

Este documento proporciona los esquemas detallados de cableado pin a pin para la construcción de ambas versiones (**Arduino Nano** y **ESP32 DevKit V1**), con especial atención a la compatibilidad de niveles de tensión de 3.3V y 5V.

---

## 1. Mapa de Pines: Arduino Nano V3 (5V Lógica)

| Dispositivo / Módulo | Pin del Dispositivo | Pin en Arduino Nano | Descripción / Nota Eléctrica |
| :--- | :--- | :--- | :--- |
| **Sensor Capacitivo v1.2** | `VCC` | `5V` (o `3.3V`) | Alimentación del sensor |
| | `GND` | `GND` | Tierra común |
| | `AOUT` | `A0` | Lectura analógica ADC (0 - 1023) |
| **Pantalla OLED SSD1306** | `VCC` | `5V` (o `3.3V`) | Alimentación I2C |
| | `GND` | `GND` | Tierra común |
| | `SCL` | `A5` | Reloj I2C (Hardware SCL) |
| | `SDA` | `A4` | Datos I2C (Hardware SDA) |
| **Driver de Bomba (MOSFET)** | `Gate` | `D3` (vía R 1 kΩ) | Señal PWM / Digital. Resistencia pull-down de 10 kΩ a GND obligatoria |
| | `Source` | `GND` | Conexión a tierra común |
| | `Drain` | Bomba (-) | Retorno conmutado a tierra de la bomba |
| **Bomba Sumergible 5V** | Positivo (+) | `+5V` (Fuente externa) | Conexión directa al riel de potencia de la fuente (NUNCA al pin de la placa) |
| | Negativo (-) | `Drain` (MOSFET) | Conmutado a masa por el MOSFET |
| **Diodo Flyback (1N5819 / 1N4007)** | Cátodo (Banda) | Bomba (+) / `+5V` | En antiparalelo directo a bornes del motor (supresión de Back-EMF) |
| | Ánodo | Bomba (-) / Drain | Conexión al colector/drain |
| **Sensor de Flotador** | Terminal 1 | `D4` | Entrada digital con `INPUT_PULLUP` interno (LOW = Agua OK, HIGH = Vacío) |
| | Terminal 2 | `GND` | Retorno a masa |
| **Buzzer Pasivo (TMB12A05)** | Positivo (+) | `D5` (vía R 220 Ω) | Tono suave (850 Hz) generado con `tone()`. Resistencia de atenuación |
| | Negativo (-) | `GND` | Tierra común |
| **Pulsador Manual (Tact 6x6)** | Pata 1 | `D2` | Entrada digital con `INPUT_PULLUP` interno (LOW al pulsar) |
| | Pata diagonal | `GND` | Retorno a masa |

---

## 2. Mapa de Pines: ESP32 DevKit V1 (3.3V Lógica)

> [!CAUTION] ¡CUIDADO CRÍTICO CON LA TENSIÓN ANALÓGICA EN ESP32!
> Los pines del ESP32 operan a **3.3V como máximo absoluto** ($V_{in} \le 3.6V$). El pin `GPIO 34` **SE DESTRUIRÁ** si recibe tensiones de 5V.  
> Por tanto, el Sensor Capacitivo de Suelo v1.2 en el ESP32 **DEBE alimentarse obligatoriamente desde el pin 3.3V del ESP32**, jamás desde los 5V. Al alimentarlo con 3.3V, su salida analógica entrega entre ~1.2V (agua) y ~2.9V (aire seco), 100% segura para el conversor ADC1 del ESP32.

| Dispositivo / Módulo | Pin del Dispositivo | Pin en ESP32 DevKit V1 | Descripción / Nota Eléctrica |
| :--- | :--- | :--- | :--- |
| **Sensor Capacitivo v1.2** | `VCC` | **`3.3V`** (¡NO 5V!) | **Obligatorio a 3.3V para proteger el ADC del ESP32** |
| | `GND` | `GND` | Tierra común |
| | `AOUT` | `GPIO 34` | Entrada analógica ADC1_CH6 (rango 0 a 3.1V aprox.) |
| **Pantalla OLED SSD1306** | `VCC` | `3.3V` | Alimentación del módulo display |
| | `GND` | `GND` | Tierra común |
| | `SCL` | `GPIO 22` | Reloj I2C por defecto en ESP32 |
| | `SDA` | `GPIO 21` | Datos I2C por defecto en ESP32 |
| **Driver de Bomba (Opción Oficial ESP32: Driver No Inversor)** | Control Input | GPIO 18 (vía R1 2.2 kΩ) | Base de Q1 (2N2222A) con pull-down 10 kΩ a GND. Activo en HIGH |
| | Etapa Alta | Emisor Q2 a +5V | Q2 (2N3906 PNP) con pull-up 10 kΩ a +5V y base a colector Q1 vía R2 2.2 kΩ |
| | Salida a Gate| Colector Q2 | Conectado a Gate del IRLZ44N con pull-down 10 kΩ a GND (conmuta a ~4.85V) |
| | Source | GND | Tierra común unificada |
| | Drain | Bomba (-) | Retorno conmutado del motor a tierra |
| **Driver de Bomba (Alternativa Comercial Verificada)** | TRIG / IN | GPIO 18 | Módulo MOSFET HW-517 (Dual AOD4184A con ensayo previo en banco de $V_{DS} \le 50\,\text{mV}$). Relés descartados. |
| **Bomba Sumergible 5V** | Positivo (+) | `+5V` (Fuente externa) | Conexión directa a fuente 5V 2A/3A |
| | Negativo (-) | `Drain` (MOSFET) | Conmutado |
| **Diodo Flyback (1N5819 / 1N4007)** | Cátodo (Banda) | Bomba (+) / `+5V` | En antiparalelo directo a bornes del motor |
| | Ánodo | Bomba (-) / Drain | Conexión al drain del MOSFET |
| **Sensor de Flotador** | Terminal 1 | `GPIO 19` | Configurado con `INPUT_PULLUP` interno |
| | Terminal 2 | `GND` | Retorno a masa |
| **Buzzer Pasivo** | Positivo (+) | `GPIO 23` (vía R 220 Ω) | Control de tono por software. Resistencia de 220 Ω de protección |
| | Negativo (-) | `GND` | Tierra común |
| **Pulsador Manual** | Pata 1 | `GPIO 4` | Configurado con `INPUT_PULLUP` interno |
| | Pata diagonal | `GND` | Retorno a masa |

---

## 3. Diagramas Esquemáticos Completos por Plataforma

Para evitar cualquier ambigüedad de cableado entre los sistemas de 5.0V (Nano) y 3.3V (ESP32), se presentan los esquemas eléctricos completos e independientes para cada controlador:

### 3.1 Esquema Eléctrico Completo: Arduino Nano V3 (Lógica Nativa 5.0V)

En el Arduino Nano, el pin digital entrega entre $4.8\,\text{V}$ y $4.9\,\text{V}$, permitiendo el accionamiento directo de la compuerta del **MOSFET IRLZ44N** con lógica directa (`HIGH = ON`, `LOW = OFF`):

```text
               +5V Fuente Externa Regulada (2A - 3A)
                  |
         +--------+------------------------------------+
         |                                             |
       [5V Pin]                                  Bomba 5V (+)
    +-----------------+                                |
    |  ARDUINO NANO   |                              [ M ] (Microbomba DC)
    |  (ATmega328P)   |                                |
    |                 |                           +----+----+
    |  D3 (Disparo)   |                           |    |    |
    |        |        |                           |  [ 1N5819 ] Diodo Flyback
    |     [1 kΩ]      |                           |  (Cátodo con franja al +)
    |        |        |                           |    |    |
    |        +-------------------> Pin 1: Gate    +----+----+
    |                 |                    |           |
    |     [10 kΩ]     |                    |      Pin 2: Drain
    |   (Pull-down)   |                    |      +----+
    |        |        |                    |      |    | IRLZ44N (TO-220)
    |       GND       |                    |      +----+
    |                 |                    |           | Pin 3: Source
    |  D5 (Buzzer)    |                    |           |
    |        |        |                    |           |
    |     [220 Ω]     |                    |           |
    |        |        |                    |           |
    |  [BUZZER PASIVO]|                    |           |
    |        |        |                    |           |
    |       GND       |                    |           |
    |                 |                    |           |
    |  D4 (Flotador)---------[BOYA MAGNÉTICA CS-CO058]---------> GND (INPUT_PULLUP)
    |  D2 (Pulsador)---------[TACT SWITCH 6x6 mm]-------------> GND (INPUT_PULLUP)
    |  A4 (SDA)-------------> SDA Pantalla OLED SSD1306 (0x3C)
    |  A5 (SCL)-------------> SCL Pantalla OLED SSD1306 (0x3C)
    |  A0 (Sensor)----------> Señal Sensor Capacitivo v1.2 (Alimentado a 5V)
    |                 |                                |
    |       GND       |                                |
    +--------+--------+--------------------------------+
             |
            GND Fuente Externa (Tierra común unificada)
```

---

### 3.2 Esquema Eléctrico Completo: ESP32 DevKit V1 (Driver No Inversor NPN+PNP)

En el ESP32 (lógica de 3.3V), la compuerta del IRLZ44N **no debe conectarse directamente** debido a la falta de garantía de $R_{DS(on)}$ a $\le 3.3\,\text{V}$. Se utiliza un **Driver No Inversor Bipolar de 2 Etapas (Q1 NPN + Q2 PNP)** que eleva la señal de compuerta a **+4.85V plenos**, garantizando que la bomba permanezca **apagada durante el arranque, reset o desconexión del microcontrolador**:

```text
               +5V Fuente Externa Regulada (2A - 3A)
                  |
         +--------+------------------------+------------------------------+
         |                                 |                              |
      [VIN Pin]                            | (Riel +5V)             Bomba 5V (+)
    +-----------------+                    |                              |
    |  ESP32 DEVKIT   |                    +---------------+        [ M ] (Microbomba DC)
    |  (30 Pines)     |                    |               |              |
    |                 |              [R_pu: 10 kΩ]   Emisor (Pin 1)       |
    |  GPIO 18        |                    |         +-------------+      |
    |  (Disparo 3.3V) |                    +---------| Q2: 2N3906  |      |
    |        |        |                    | (Base)  | (PNP TO-92) |      |
    |   [R1: 2.2 kΩ]  |               [R2: 2.2 kΩ]   +-------------+      |
    |        |        |                    |         Colector (Pin 3)     |
    |        +--------+---> Base (Pin 2)   |               |              |
    |        |        |    +-------------+ |               |              |
    |  [R_pd1: 10 kΩ] |    | Q1: 2N2222A | |               |              |
    |        |        |    | (NPN TO-92) | |               |              |
    |       GND       |    +-------------+ |               |              |
    |                 |     Emisor (Pin 1) |               |              |
    |                 |            |       |               |              |
    |                 |           GND      |               |              |
    |                 |                    |               |              |
    |  Colector Q1 (Pin 3) ----------------+               |              |
    |                                                      |              |
    |  Salida de Compuerta (+4.85V teóricos) <-------------+              |
    |        |                                                            |
    |        +----------------------------------------> Pin 1: Gate       |
    |        |                                             |              |
    |  [R_pd2: 10 kΩ]                                      |        Pin 2: Drain
    |        |                                             |        +-----+  |
    |       GND                                            |        | IRLZ|  +----+
    |                                                      |        | 44N |  |    |
    |                                                      |        +-----+  [1N5819]
    |                                                      |           |     (Schottky)
    |                                                      |        Pin 3:   |    |
    |                                                      |        Source   +----+
    |                                                      |           |          |
    |  GPIO 23 (Buzzer)-----[220 Ω]-----> [BUZZER]-------->+          GND         |
    |  GPIO 19 (Flotador)---[BOYA CS-CO058]--------------->+                      |
    |  GPIO 4  (Pulsador)---[TACT SWITCH]----------------->+                      |
    |  GPIO 21 (SDA)--------> SDA OLED SSD1306 (0x3C)      |                      |
    |  GPIO 22 (SCL)--------> SCL OLED SSD1306 (0x3C)      |                      |
    |  3V3 Pin -------------> VCC Sensor Capacitivo v1.2   |                      |
    |  GPIO 34 (Sensor)-----> Señal Analógica Sensor       |                      |
    |                                                      |                      |
    |       GND                                            |                      |
    +--------+---------------------------------------------+----------------------+
             |
            GND Fuente Externa (Tierra común unificada)
```

#### 📌 Netlist y Tabla Maestra de Nodos Pin a Pin (Resolución Inequívoca):

| Nodo # | Identificador | Terminales Físicos Conectados | Nivel Teórico en Reposo / Boot | Nivel Teórico en Activo (GPIO=3.3V) | Criterio de Aceptación Cuantitativo | Condiciones de Prueba y Diagnóstico Instrumental |
| :---: | :--- | :--- | :---: | :---: | :---: | :--- |
| **N1** | `GPIO_CTRL` | ESP32 `GPIO 18` + Terminal 1 de $R_1$ ($2.2\,\text{k}\Omega$) | $0.0\,\text{V}$ (o High-Z) | $3.3\,\text{V}$ ($V_{OH}$) | Reposo: $\le 0.10\,\text{V}$<br/>Activo: $\ge 3.00\,\text{V}$ | Salida lógica directa del microcontrolador. |
| **N2** | `BASE_Q1` | Terminal 2 de $R_1$ ($2.2\,\text{k}\Omega$) + Terminal 1 de $R_{pd1}$ ($10\,\text{k}\Omega$) + Base (Pin 2) de Q1 (2N2222A) | $0.0\,\text{V}$ (Corte Q1) | $\approx 0.70\,\text{V}$ ($V_{BE1(on)}$) | Reposo: $\le 0.05\,\text{V}$<br/>Activo: $0.65\,\text{V} - 0.75\,\text{V}$ | **Condición:** $V_{BE1} \approx 0.70\,\text{V}$ solo indica polarización directa de la unión B-E. **No demuestra por sí solo saturación**; debe verificarse simultáneamente midiendo en N3 que $V_{CE1} \le 0.20\,\text{V}$ con $I_B \approx 1.1\,\text{mA}$. |
| **N3** | `COL_Q1` | Colector (Pin 3) de Q1 (2N2222A) + Terminal 1 de $R_2$ ($2.2\,\text{k}\Omega$) | $+5.0\,\text{V}$ (vía $R_2$ y $R_{pu}$) | $\approx 0.10\,\text{V}$ ($V_{CE1(sat)}$) | Reposo: $\ge 4.75\,\text{V}$<br/>Activo: $\le 0.20\,\text{V}$ | Si $V_{N3} > 0.30\,\text{V}$ en activo, Q1 no está en saturación profunda (revisar $R_1$, $R_{pd1}$ o ganancia del transistor). |
| **N4** | `BASE_Q2` | Terminal 2 de $R_2$ ($2.2\,\text{k}\Omega$) + Terminal 1 de $R_{pu}$ ($10\,\text{k}\Omega$) + Base (Pin 2) de Q2 (2N3906) | $+5.0\,\text{V}$ ($V_{EB2} = 0\,\text{V}$, Corte Q2) | **$\approx 4.30\,\text{V}$** (Conducción nominal Q2)<br/>*o bien*<br/>**$\approx 0.98\,\text{V}$** (Divisor pasivo sin carga de base) | Reposo: $\ge 4.75\,\text{V}$<br/>Activo nominal: $4.20\,\text{V} - 4.40\,\text{V}$ | **Interpretación de condiciones:**<br/>- **$4.30\,\text{V}$:** Juntura E-B polarizada en directa con emisor a +5V ($V_{N4} = 5.0 - V_{EB2}$).<br/>- **$0.98\,\text{V}$:** Divisor Thévenin formado por $R_{pu}$ y $R_2$ con $V_{C1} \approx 0.10\,\text{V}$ cuando Q2 no carga el nodo. **No sentencia falla por sí solo**: debe interpretarse en conjunto con N3, N5 y comprobación de pines (E-B-C). |
| **N5** | `GATE_Q3` | Colector (Pin 3) de Q2 (2N3906) + Terminal 1 de $R_{pd2}$ ($10\,\text{k}\Omega$) + Gate (Pin 1) de Q3 (IRLZ44N) | $0.0\,\text{V}$ (Corte Q3 / Bomba OFF) | $\approx 4.85\,\text{V} - 4.90\,\text{V}$ ($V_{CC} - V_{EC2(sat)}$) | Reposo: $\le 0.10\,\text{V}$ (ACT-01)<br/>Activo: $\ge 4.50\,\text{V}$ (ACT-02) | **Distinción crítica:** $V_{GS} \ge 4.50\,\text{V}$ garantiza sobrevoltaje de compuerta (*overdrive*), pero **no prueba por sí sola la conducción de carga**. La conducción efectiva se verifica midiendo $V_{DS}$ en N6 y el flujo de corriente en la carga. |
| **N6** | `DRAIN_Q3` | Drain (Pin 2) de Q3 (IRLZ44N) + Ánodo de D1 (1N5819) + Retorno (-) de la Carga Conectada | $+5.0\,\text{V}$ *(con carga conectada)*<br/>*Flotante (si no hay carga)* | Prueba LED: $< 1\,\text{mV}$<br/>Prueba Bomba: $12 - 15\,\text{mV}$ | Prueba LED: LED ON.<br/>Prueba Bomba (ACT-03): $V_{DS} \le 50\,\text{mV}$ a $350\,\text{mA}$ nominal en agua. | **Dependencia de la carga instalada:**<br/>- En reposo, N6 mide $+5.0\,\text{V}$ solo si la carga (LED+resistencia o motor) está físicamente conectada a +5V.<br/>- El criterio $V_{DS} \le 50\,\text{mV}$ aplica a la corriente nominal de la bomba en agua; no es un valor universal para cualquier carga o corriente. |
| **N7** | `VCC_5V` | Riel +5V Fuente + Pin VIN ESP32 + Emisor (Pin 1) Q2 + Terminal 2 de $R_{pu}$ + Cátodo D1 + Positivo Carga | $+5.0\,\text{V}$ | $+5.0\,\text{V}$ (con caída admisible por carga) | $4.75\,\text{V} - 5.25\,\text{V}$ | Riel común de potencia regulada. Verificar estabilidad bajo conmutación de carga. |
| **N8** | `GND_COMUN` | GND Fuente + GND ESP32 + Emisor (Pin 1) Q1 + Terminal 2 de $R_{pd1}$ + Terminal 2 de $R_{pd2}$ + Source (Pin 3) Q3 | $0.0\,\text{V}$ | $0.0\,\text{V}$ | $0.0\,\text{V}$ ($\Delta V < 10\,\text{mV}$ en pistas/jumpers) | Masa de referencia unificada para todas las medidas instrumentales. |

#### 🔬 Interpretación Rigurosa de Diagnóstico Nodal y Condicionamiento de Estados:

1. **Relación N2 (`BASE_Q1`) frente a N3 (`COL_Q1`):**
   - Una lectura de $\approx 0.70\,\text{V}$ en N2 solo confirma que la juntura base-emisor de Q1 está polarizada en directa. **No es suficiente para declarar que el transistor está en saturación profunda**.
   - Para que Q1 esté en saturación requerida, debe cumplirse la condición simultánea en N3 de que $V_{CE1} \le 0.20\,\text{V}$ (típicamente $0.08\text{--}0.15\,\text{V}$) con la corriente de base de diseño ($I_{B1} \approx 1.1\,\text{mA}$). Si N2 marca $0.70\,\text{V}$ pero N3 permanece en $\ge 0.50\,\text{V}$, Q1 se encuentra en región activa directa (inadecuada para conmutación).

2. **Doble Régimen Nodal en N4 (`BASE_Q2`):**
   - **Régimen Nominal ($\approx 4.30\,\text{V}$):** Se presenta cuando el transistor PNP Q2 está insertado correctamente y su juntura emisor-base conduce. Dado que su emisor está en el riel de $+5.0\,\text{V}$, la juntura fija el potencial de base en $V_B = V_E - V_{EB(on)} \approx 5.0\,\text{V} - 0.70\,\text{V} = 4.30\,\text{V}$. La corriente de base resultante es $I_{B2} \approx 1.84\,\text{mA}$, llevando a Q2 a saturación profunda ($\beta_{forced} \approx 0.26$).
   - **Régimen de Circuito Abierto / Divisor Thévenin Pasivo ($\approx 0.98\,\text{V}$):** Ocurre si la base de Q2 no drena corriente (transistor ausente, pata de base no conectada o juntura interna abierta). En ese escenario, el nodo N4 queda determinado por el divisor resistivo formado por $R_{pu} = 10\,\text{k}\Omega$ conectada a $+5.0\,\text{V}$ y $R_2 = 2.2\,\text{k}\Omega$ conectada al colector saturado de Q1 ($V_{C1} \approx 0.10\,\text{V}$):
     $$V_{N4(unloaded)} = 0.10\,\text{V} + (5.0\,\text{V} - 0.10\,\text{V}) \times \frac{2.2\,\text{k}\Omega}{10\,\text{k}\Omega + 2.2\,\text{k}\Omega} = 0.10 + 4.90 \times \frac{2.2}{12.2} \approx \mathbf{0.98\,\text{V}}$$
   - **Protocolo de Diagnóstico:** Obtener $\approx 0.98\,\text{V}$ en N4 no debe diagnosticarse apresuradamente como "avería" sin antes contrastar: (a) que N3 esté en $\le 0.20\,\text{V}$, (b) que la orientación física de las patas de Q2 corresponda a Pin 1=Emisor, Pin 2=Base, Pin 3=Colector, y (c) que en N5 no aparezca tensión alta.

3. **Distinción entre Excitación de Compuerta N5 ($V_{GS}$) y Conducción N6 ($V_{DS}$):**
   - Una tensión de compuerta de $4.85\text{--}4.90\,\text{V}$ en N5 ($V_{GS}$) certifica que el pre-driver bipolar funciona y entrega la sobretensión de compuerta requerida (*gate overdrive*) para llevar al MOSFET IRLZ44N a su región óhmica de mínima resistencia.
   - Sin embargo, **$V_{GS} \ge 4.50\,\text{V}$ no demuestra por sí sola que la corriente circule por la carga**. Para validar la conmutación efectiva del canal, es mandatorio medir la tensión de drenador $V_{DS}$ en el nodo N6:
     * Si el canal conduce normalmente, $V_{DS} = I_D \times R_{DS(on)}$ debe caer a valores mínimos.
     * Si N5 está en $4.90\,\text{V}$ pero N6 permanece en $+5.0\,\text{V}$, el MOSFET tiene el canal abierto, la fuente desconectada de GND o la pata de compuerta dañada.
   - El criterio de aceptación $V_{DS} \le 50\,\text{mV}$ está específicamente acotado a la corriente nominal de la microbomba JT-DC3W sumergida en agua ($I_D \approx 300\text{--}350\,\text{mA}$, disipación $\approx 4.3\,\text{mW}$); con la carga de prueba preliminar LED ($I_D \approx 9\,\text{mA}$), la caída es inferior a $1\,\text{mV}$. No debe extrapolarse como un valor absoluto para cargas superiores sin recalcular $I_D \times R_{DS(on)}$.

4. **Dependencia del Estado de Reposo en N6 (`DRAIN_Q3`) según la Carga Instalada:**
   - La indicación de que N6 mide $+5.0\,\text{V}$ en reposo asume que existe una carga eléctrica conectada entre el riel de $+5.0\,\text{V}$ y el terminal Drain (ya sea la rama de prueba LED + resistencia de $330\,\Omega$, o el bobinado de la bomba). La resistencia interna de dicha carga actúa como pull-up natural.
   - Si no hay ninguna carga conectada entre +5V y N6 (circuito abierto), el nodo N6 se encuentra en alta impedancia (flotante), y la lectura con un multímetro de alta impedancia de entrada no reflejará $+5.0\,\text{V}$ de manera determinista.

#### Identificación Funcional y Variantes de Encapsulado (CRIT-01 / MEN-05):
> [!CAUTION] Alerta Crítica de Fabricante: Ambigüedad en el Pinout de Q1 (2N2222A / PN2222A / P2N2222A)
> **No existe un orden de pines numérico universal para transistores marcados como 2N2222 en encapsulado plástico TO-92.**
> - **Variante P2N2222A (onsemi / Motorola):** Con la cara plana hacia el observador y las patillas hacia abajo, el orden de pines es **Pin 1 = Colector (C), Pin 2 = Base (B), Pin 3 = Emisor (E)**.
> - **Variante PN2222A / 2N2222A (Fairchild / Central Semiconductor / genéricos):** Con la cara plana hacia el observador y las patillas hacia abajo, el orden es **Pin 1 = Emisor (E), Pin 2 = Base (B), Pin 3 = Colector (C)**.
> - **Variante 2N2222 original (Cápsula metálica TO-18):** Pin 1 = Emisor, Pin 2 = Base, Pin 3 = Colector (unido eléctricamente a la carcasa).
> - **Transistor Q2 (2N3906 PNP TO-92):** Habitualmente Pin 1 = Emisor (E), Pin 2 = Base (B), Pin 3 = Colector (C), pero igualmente sujeto a variación según el fabricante.
> 
> **Procedimiento Obligatorio de Identificación Pre-Montaje (en este orden estricto):**
> 1. **Registrar el marcaje exacto:** Anotar la serigrafía completa impresa en el cuerpo del componente físico recibido.
> 2. **Determinar fabricante y referencia:** Identificar el fabricante y la variante comercial exacta (ej. `P2N2222A` frente a `PN2222A`).
> 3. **Consultar la hoja de datos oficial (*datasheet*):** Obtener el documento técnico correspondiente exactamente a esa referencia y encapsulado.
> 4. **Confirmar la orientación física:** Determinar la posición de Emisor, Base y Colector a partir del plano mecánico oficial de dicho fabricante.
> 5. **Comprobación complementaria con multímetro:** Utilizar el modo de prueba de diodos para verificar la integridad de las uniones PN y confirmar la terminal de Base ($V_{BE} \approx 0.60\text{--}0.75\,\text{V}$ y $V_{BC} \approx 0.60\text{--}0.75\,\text{V}$ en directa; circuito abierto `OL` en inversa y entre C-E). *Nota técnica (MEN-05): No debe utilizarse una desigualdad estricta entre $V_{BE}$ y $V_{BC}$ como criterio único para distinguir Emisor de Colector, dado que su diferencia depende del lote, corriente de prueba y temperatura; la referencia primaria para asignar E y C es siempre la hoja de datos del componente exacto.*
> 6. **Registro previo a energización:** Documentar el pinout verificado en la hoja de registro de laboratorio antes de insertar, soldar o alimentar el circuito.

- **Q1 (Transistor NPN, e.g. 2N2222A / PN2222A / P2N2222A - TO-92):**
  * `Emisor (E)`: Conectado firmemente a GND común.
  * `Base (B)`: Conectada al divisor de entrada desde GPIO 18 vía $R_1 = 2.2\,\text{k}\Omega$ y con $R_{pd1} = 10\,\text{k}\Omega$ a GND.
  * `Colector (C)`: Conectado a la Base de Q2 vía $R_2 = 2.2\,\text{k}\Omega$.
- **Q2 (Transistor PNP, e.g. 2N3906 - TO-92):**
  * `Emisor (E)`: Conectado directamente al riel de potencia de $+5.0\,\text{V}$ DC regulado.
  * `Base (B)`: Conectada al Colector de Q1 mediante $R_2 = 2.2\,\text{k}\Omega$ y polarizada a $+5.0\,\text{V}$ mediante $R_{pu} = 10\,\text{k}\Omega$.
  * `Colector (C)`: Conectado directamente a la Compuerta (Gate) de Q3 (M1), con resistencia pull-down de descarga $R_{pd2} = 10\,\text{k}\Omega$ a GND.
- **M1 / Q3 (N-MOSFET Logic-Level Vishay IRLZ44N - TO-220):**
  * Pin 1 (`Gate`): Desde Colector de Q2, retenido a GND mediante $R_{pd2} = 10\,\text{k}\Omega$.
  * Pin 2 / Tab (`Drain`): Conectado al borne negativo de la microbomba JT-DC3W (retorno conmutado).
  * Pin 3 (`Source`): Conectado directamente a la masa común (GND).
- **D1 (Diodo Schottky 1N5819):**
  * `Cátodo` (banda plateada): Conectado al borne positivo (+5V) del motor.
  * `Ánodo`: Conectado al borne negativo (Drain de Q3) del motor. Montaje en antiparalelo directo sobre los bornes de la bomba.

#### Cálculo Nodal Riguroso de Polarización y Margen de Saturación:
1. **Etapa de Entrada Q1 (NPN):**
   - El circuito equivalente de Thévenin en la base de Q1, formado por el GPIO a $3.3\,\text{V}$, $R_1 = 2.2\,\text{k}\Omega$ y $R_{pd1} = 10\,\text{k}\Omega$, entrega $V_{TH} = 3.3 \times \frac{10}{12.2} \approx 2.70\,\text{V}$ y $R_{TH} = 2.2 \parallel 10 \approx 1.80\,\text{k}\Omega$.
   - Con $V_{BE1(sat)} \approx 0.70\,\text{V}$, la corriente real de base es:
     $$I_{B1} = \frac{2.70\,\text{V} - 0.70\,\text{V}}{1800\,\Omega} \approx 1.11\,\text{mA}$$
2. **Etapa Intermedia Q2 (PNP):**
   - Con Q1 saturado ($V_{CE1(sat)} \approx 0.1\,\text{V}$), la corriente total de colector drenada a través de $R_2 = 2.2\,\text{k}\Omega$ es $I_{C1} = \frac{5.0 - 0.7 - 0.1}{2200} \approx 1.91\,\text{mA}$.
   - Descontando la corriente de fuga de $R_{pu}$ ($0.7\,\text{V} / 10\,\text{k}\Omega = 0.07\,\text{mA}$), la corriente neta de base de Q2 es $I_{B2} \approx 1.84\,\text{mA}$.
   - Ganancia forzada en Q1: $\beta_{forced, Q1} = \frac{1.91\,\text{mA}}{1.11\,\text{mA}} \approx 1.72 \ll 10$. **Saturación ultra profunda asegurada.**
3. **Etapa de Salida hacia Compuerta:**
   - En régimen continuo, la compuerta del MOSFET solo drena la corriente de la resistencia de descarga $R_{pd2} = 10\,\text{k}\Omega$:
     $$I_{C2} = \frac{V_{Gate}}{10000\,\Omega} \approx \frac{4.85\,\text{V}}{10000\,\Omega} \approx 0.485\,\text{mA}$$
   - Ganancia forzada en Q2: $\beta_{forced, Q2} = \frac{0.485\,\text{mA}}{1.84\,\text{mA}} \approx 0.26 \ll 10$. **Saturación ultra profunda asegurada.**
   - Tensión de compuerta resultante: $V_{Gate} = V_{fuente} - V_{EC2(sat)} \approx 5.0\,\text{V} - 0.10\,\text{V} \approx \mathbf{4.90\,\text{V}}$ (en banco real con caída de carga en la fuente: $4.70\,\text{V} - 4.90\,\text{V}$), garantizando holgadamente que el IRLZ44N opere dentro de su especificación garantizada de $R_{DS(on)} \le 0.035\,\Omega$.

---

## 4. Análisis de Decisiones Eléctricas Críticas

### 4.1 Comportamiento del MOSFET a 3.3V (ESP32) vs 5V (Nano) y Diseño de Etapa de Potencia

- **Arduino Nano V3 (Accionamiento Directo a 5.0V):**
  - El pin `D3` entrega $V_{OH} \ge 4.2\,\text{V}$ (típicamente $4.8\,\text{V}-4.9\,\text{V}$ con carga mínima de compuerta). Según la hoja de datos oficial de Vishay Siliconix (Doc #91328), el MOSFET **IRLZ44N** especifica una resistencia máxima garantizada de $R_{DS(on)} \le 0.035\,\Omega$ bajo la condición estandarizada $V_{GS} = 5.0\,\text{V}, I_D = 25\,\text{A}, T_J = 25^\circ\text{C}$ (y $\le 0.040\,\Omega$ a $V_{GS} = 4.0\,\text{V}, I_D = 21\,\text{A}$). Para niveles de excitación entre $4.2\,\text{V}$ y $4.9\,\text{V}$, la resistencia estática de catálogo queda acotada en ese rango a $25^\circ\text{C}$, con una disipación nominal calculada de $\approx 4.3\,\text{mW}$ a $350\,\text{mA}$, sujeta al coeficiente térmico positivo de la juntura y a la verificación experimental de la tensión real $V_{GS}$ y caída $V_{DS}$ en banco.
- **ESP32 DevKit V1 (Accionamiento a 3.3V) y Desafío de Nivel Lógico:**
  - **Limitación del IRLZ44N a 3.3V:** No posee especificación garantizada de $R_{DS(on)}$ para tensiones de compuerta $\le 3.3\,\text{V}$ (solo especifica hasta $4.0\,\text{V}$ en tabla). Aunque las curvas típicas de laboratorio a $25^\circ\text{C}$ muestran conducción suficiente para cargas de $300\,\text{mA}$, una unidad con tolerancia desfavorable ($V_{GS(th)} \to 2.0\,\text{V}$) reduce el sobrevoltaje efectivo a $\approx 1.1\,\text{V}$.
  - **FQP30N06L:** **DESCARTADO FORMALMENTE** para el ESP32. Su $V_{GS(th)}$ alcanza hasta $2.5\,\text{V}$, dejando un margen de sobretensión inaceptable de apenas $0.8\,\text{V}$ a 3.3V.

---

#### ⚠️ ANÁLISIS CRÍTICO DE POLARIDAD Y REGLA DE ARRANQUE SEGURO

> [!CAUTION] ¡Peligro de Inversión Lógica y Bombeo Espurio en Reset!
> El firmware de Smart FloraCare opera con **Lógica Activa en Alto**:
> `digitalWrite(PIN_DRIVER_BOMBA, pumpShouldRun ? HIGH : LOW);`
> Durante el encendido, reinicio o cuelgue del microcontrolador, los pines GPIO se configuran como entradas en alta impedancia (High-Z) o se fuerzan a `LOW` por inicialización.
> **Cualquier etapa de potencia donde un nivel `LOW` o flotante active la bomba es INACEPTABLE**, pues provocaría una inundación inmediata al enchufar el circuito.

##### 1. Por qué un Pre-Driver NPN Simple (Emisor Común) está PROHIBIDO:
Si se conecta `GPIO 18` a la base de un NPN con el colector a la compuerta del MOSFET y una resistencia pull-up a +5V:
- Cuando `GPIO 18 = LOW` (o el ESP32 está en reset), el NPN está apagado $\to$ la resistencia pull-up lleva la compuerta a **+5.0V**, **ENCENDIENDO LA BOMBA INMEDIATAMENTE AL CONECTAR LA ALIMENTACIÓN**.
- Cuando `GPIO 18 = HIGH`, el NPN conduce y tira la compuerta a 0V, apagando la bomba.
- **Conclusión:** Esta topología invierte la lógica y viola el principio de diseño a prueba de fallos (*fail-safe*). **NO debe montarse.**

##### 2. Solución Discreta Oficial: Driver No Inversor de 2 Etapas (NPN + PNP)
Para lograr traslación de nivel de 3.3V a 5.0V con lógica no inversora y apagado garantizado en el arranque:

```
                  +5V Fuente Externa (Riel de Potencia)
                     |                   |
                     |                   +------------------------+
                     |                   |                        |
                     |             [R_pu: 10 kΩ]            Emisor Q2 (Pin 1)
                     |                   |                  +-------------+
                     |                   +------------------| Q2: 2N3906  |
                     |                   |                  | (PNP TO-92) |
                     |             [R2: 2.2 kΩ]             +-------------+
                     |                   |                  Colector Q2 (Pin 3)
                     |             Colector Q1 (Pin 3)            |
                     |             +-------------+                +-----> Gate MOSFET (IRLZ44N)
                     |             | Q1: 2N2222A |                             |
GPIO 18 ---[R1: 2.2 kΩ]------------| (NPN TO-92) |                       [R_pd2: 10 kΩ]
               |                   +-------------+                             |
         [R_pd1: 10 kΩ]            Emisor Q1 (Pin 1)                          GND
               |                         |
              GND                       GND
```

**Tabla de Estados del Driver No Inversor NPN+PNP:**

| Estado de GPIO 18 | Transistor Q1 (NPN) | Transistor Q2 (PNP) | Tensión en Gate IRLZ44N | Estado de la Bomba | Evaluación de Seguridad |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Arranque / Reset / High-Z** | Corte ($I_B = 0$) | Corte ($V_{EB} = 0$) | $0.00\,\text{V}$ (vía $R_{pd2}$) | **APAGADA** | **Fail-Safe Garantizado** |
| **Reposo Normal (`LOW = 0V`)** | Corte | Corte | $0.00\,\text{V}$ | **APAGADA** | Seguro |
| **Riego Activo (`HIGH = 3.3V`)** | Saturado ($I_B \approx 1.2\,\text{mA}$) | Saturado ($I_B \approx 1.9\,\text{mA}$) | **$\approx 4.85\,\text{V}$** | **ENCENDIDA** | **Saturación Plena ($R_{DS(on)} \le 0.035\,\Omega$)** |
| **Fallo / Cable Desconectado** | Corte (vía $R_{pd1}$) | Corte | $0.00\,\text{V}$ | **APAGADA** | Seguro ante desconexión |

##### 3. Opción de Circuito Integrado: Buffer TTL CMOS 74AHCT125 / 74HCT14
En lugar de transistores discretos, puede emplearse una compuerta buffer no inversora compatible TTL alimentada a 5.0V (como el **74AHCT125** con $\overline{\text{OE}}$ a GND, o dos compuertas inversoras en cascada de un **74HCT14**):
- Tensión de entrada alta admisible: $V_{IH} \ge 2.0\,\text{V}$ (reconoce limpiamente los 3.3V del ESP32).
- Salida CMOS rail-to-rail a 5.0V directos hacia la compuerta del IRLZ44N.
- Con pull-down de 10 kΩ en la entrada, la compuerta permanece rígidamente en 0.0V durante el boot.

##### 4. Criterio de Selección y Descarte de Módulos Comerciales:
- **Módulos de Relé Electromecánicos (DESCARTADOS OFICIALMENTE):** Quedan descartados como opción recomendada para el ESP32. Un módulo con lógica LOW-trigger encendería la bomba involuntariamente durante el reinicio del ESP32 (estado High-Z/LOW). Aunque un módulo cuente con jumper HIGH-trigger, la caída de tensión en el optoacoplador y LED interno diseñado para 5V suele provocar conmutaciones erráticas con señales lógicas de 3.3V, y no ofrece garantía fail-safe frente a transitorios de alimentación.
- **Módulo MOSFET HW-517 / XY-MOS (Dual AOD4184A):** Este módulo conecta la señal de entrada directamente a las compuertas de dos MOSFETs en paralelo con resistencia pull-down a GND (activo en alto). Aunque comercialmente se especifica desde 3.3V, debido a la dispersión de $V_{GS(th)}$ entre lotes, el reparto de corriente no es perfectamente simétrico. Por tanto, antes de validar su uso definitivo en el montaje, debe verificarse en banco que a 3.3V la caída total $V_{DS} \le 50\,\text{mV}$ bajo el consumo real de la bomba.

### 4.2 Selección del Diodo Flyback: Recirculación Inductiva
- **1N4148:** Es un diodo de señal pequeña de conmutación rápida. Su corriente continua máxima es de solo $150\,\text{mA}-200\,\text{mA}$. La microbomba genera corrientes de arranque y picos de desconexión inductiva de más de $500\,\text{mA}-800\,\text{mA}$. **El 1N4148 puede destruirse en cortocircuito y dañar la etapa de conmutación.**
- **1N5819 (Diodo Schottky - Elección Principal):** Soporta $1.0\,\text{A}$ continuo y $25\,\text{A}$ de sobretensión transitoria. Proporciona una ruta de recirculación de muy baja impedancia y baja caída directa ($V_F \approx 0.35\,\text{V}$) con recuperación ultra rápida ($< 10\,\text{ns}$), reduciendo la magnitud del transitorio inductivo a bornes del motor. Su efectividad para preservar la estabilidad del microcontrolador debe comprobarse con la carga real en el montaje definitivo.
- **1N4007 (Diodo Rectificador de Silicio - Alternativa Aceptable):** Soporta $1.0\,\text{A}$ continuo y $30\,\text{A}$ de pico. Aunque su tiempo de recuperación es mayor (~$2\,\mu\text{s}$) y su caída es de $\sim 1.0\,\text{V}$, ofrece una ruta de disipación inductiva robusta y probada para motores pequeños de continua.

### 4.4 Tabla de Verdad del Flotador y Comportamiento Fail-Safe

El interruptor de flotador opera como un contacto seco normalmente abierto/cerrado con imán permanente en la boya. Se conecta entre el pin digital (`D4` en Nano / `GPIO 19` en ESP32) y `GND`, utilizando la resistencia interna `INPUT_PULLUP` (~30–50 kΩ en AVR, ~45 kΩ en ESP32).

| Condición Física del Depósito | Estado Mecánico del Flotador | Estado del Contacto Reed | Tensión en Pin | Lectura Digital (`digitalRead`) | `waterLevelOk` | Acción del Sistema |
| :--- | :--- | :---: | :---: | :---: | :---: | :--- |
| **Depósito Lleno / Suficiente** | Boya elevada por flotación | **CERRADO a GND** | $\approx 0.0\,\text{V}$ | `LOW` | **`true`** | Riego permitido si otras condiciones se cumplen |
| **Depósito Vacío / Nivel Bajo** | Boya caída por gravedad | **ABIERTO** | $3.3\,\text{V} / 5.0\,\text{V}$ (Pull-up) | `HIGH` | **`false`** | Bomba bloqueada incondicionalmente (`FAULT`) |
| **Cable Cortado / Desconectado** | Circuito abierto accidental | **ABIERTO** | $3.3\,\text{V} / 5.0\,\text{V}$ (Pull-up) | `HIGH` | **`false`** | **Fail-Safe:** Bloqueo inmediato (no riega a ciegas) |
| **Cortocircuito Accidental a Chasis** | Pata puenteada a tierra | CERRADO permanente | $0.0\,\text{V}$ | `LOW` | `true` | Depende del watchdog de 25s si el tanque se vacía |

> [!IMPORTANT] Orientación Mecánica Obligatoria de la Boya
> Para asegurar el comportamiento **Fail-Safe** (que un cable suelto simule tanque vacío y bloquee el bombeo), el sensor de flotador se debe montar de modo que cuando el agua empuje la boya hacia arriba, el contacto magnético interno se **CIERRE** (conduzca a GND). Si tu modelo de boya viene invertido de fábrica, extrae el anillo de seguridad tipo circlip en la punta, dale la vuelta 180° a la boya cilíndrica y vuelve a colocar el seguro.

### 4.5 Resistencia de Compuerta ($R_G$) y Tiempo de Conmutación
- **Resistencia en serie ($1\,\text{k}\Omega$):** Con una capacidad de entrada típica $C_{iss} \approx 1700\,\text{pF}$ en el IRLZ44N, la constante de tiempo es $\tau = R \cdot C = 1000 \cdot 1.7\times 10^{-9} \approx 1.7\,\mu\text{s}$. El tiempo de conmutación total ($3\tau$) es $\approx 5.1\,\mu\text{s}$.
- Como la bomba conmuta en corriente continua estacionaria (encendido por segundos) y **no mediante modulación PWM a alta frecuencia**, un tiempo de subida de $5\,\mu\text{s}$ es perfectamente seguro, no produce pérdidas térmicas apreciables y protege el pin del microcontrolador limitando el pico de corriente de carga de compuerta a solo $I_{peak} \le 3.3\,\text{mA}$ en ESP32 y $5\,\text{mA}$ en Nano.
- **Resistencia Pull-Down ($10\,\text{k}\Omega$):** Conectada directamente entre Gate y Source (GND). Garantiza que durante el encendido del microcontrolador (cuando los pines GPIO se encuentran en alta impedancia o modo flotante antes de ejecutar `setup()`), la compuerta se mantenga sólidamente a $0\,\text{V}$, impidiendo cualquier encendido espurio o pulso transitorio de la bomba.

### 4.6 Intercambio Seguro de Controladores (Uso Alternado sin Retroalimentación)
- Como ambas variantes comparten el mismo montaje hidráulico y fuente de 5V, **NUNCA** se deben conectar los dos microcontroladores al mismo tiempo a la bomba.
- El circuito cuenta con una bornera o conectores Dupont macho/hembra de 3 vías (Alimentación +5V, Señal Gate, Tierra GND). Para cambiar de Nano a ESP32:
  1. Desconecta la fuente de pared de 5V.
  2. Desconecta el conector de 3 vías del controlador actual.
  3. Conecta el conector de 3 vías en el controlador alternativo.
  4. Reconecta la fuente de pared de 5V.
- Esto elimina cualquier riesgo de corriente circulante o retroalimentación parásita entre placas.
