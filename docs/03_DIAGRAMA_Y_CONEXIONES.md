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
| **Driver de Bomba (Opción Oficial ESP32)** | `IN / Trigger` | `GPIO 18` (vía R 1 kΩ) | Módulo MOSFET LR7843 / AOD4184 con optoacoplador o Módulo Relé 5V (disparo garantizado a 3.3V) |
| **Driver de Bomba (Alternativa Experimental)** | `Gate` | `GPIO 18` (vía R 1 kΩ) | IRLZ44N discreto (sujeto a validación en banco de $V_{GS} \ge 3.1\,\text{V}$ y $V_{DS} \le 0.05\,\text{V}$). Pull-down 10 kΩ |
| | `Source` | `GND` | Tierra común |
| | `Drain` | Bomba (-) | Retorno conmutado a tierra |
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
         +--------+-----------------------+-------------------------+
         |                                |                         |
      [VIN Pin]                           | (Riel +5V)        Bomba 5V (+)
    +-----------------+                   |                         |
    |  ESP32 DEVKIT   |                   +---[R_pu: 10 kΩ]---+   [ M ] (Microbomba DC)
    |  (30 Pines)     |                   |                   |     |
    |                 |                   |  Emisor (Pin 1)   |     |
    |  GPIO 18        |                   |  +-------------+  |     |
    |  (Disparo 3.3V) |                   +--| Q2: 2N3906  |  |     |
    |        |        |                      | (PNP TO-92) |  |     |
    |    [R1: 2.2 kΩ] |                      +-------------+  |     |
    |        |        |                   Base (Pin 2) |      |     |
    |        +-------+------> Base (Pin 2)     |       |      |     |
    |                |        +-------------+  |       |      |     |
    |         [R_pd1: 10 kΩ]  | Q1: 2N2222A |  |       |      |     |
    |                |        | (NPN TO-92) |  |       |      |     |
    |               GND       +-------------+  |       |      |     |
    |                         Emisor (Pin 1)|  |       |      |     |
    |                               |          |       |      |     |
    |                              GND         |       |      |     |
    |                                          |       |      |     |
    |    Colector Q1 (Pin 3) ------------------+       |      |     |
    |                                                  |      |     |
    |    Colector Q2 (Pin 3: Salida +4.85V) -----------+      |     |
    |                                                  |      |     |
    |                                    +-------------+      |     |
    |                                    |                    |     |
    |                               Pin 1: Gate               |     |
    |                                    |                    |     |
    |                              [R_pd2: 10 kΩ]             |     |
    |                                    |                    |     |
    |                                   GND                   |     |
    |                                                         |     |
    |                                                 Pin 2: Drain  |
    |                                                 +----+  |     |
    |                                                 |    |  +-----+-----+
    |                                                 +----+  |     |     |
    |                                                 Pin 3:  | [1N5819]  |
    |                                                 Source  | (Schottky)|
    |                                                    |    +-----+-----+
    |                                                    |          |
    |  GPIO 23 (Buzzer)-----[220 Ω]-----> [BUZZER]-----> GND        |
    |  GPIO 19 (Flotador)---[BOYA CS-CO058]------------> GND        |
    |  GPIO 4  (Pulsador)---[TACT SWITCH]--------------> GND        |
    |  GPIO 21 (SDA)--------> SDA OLED SSD1306 (0x3C)               |
    |  GPIO 22 (SCL)--------> SCL OLED SSD1306 (0x3C)               |
    |  3V3 Pin -------------> VCC Sensor Capacitivo v1.2 (¡3.3V!)   |
    |  GPIO 34 (Sensor)-----> Salida Analógica Sensor Capacitivo    |
    |                                                    |          |
    |       GND                                          |          |
    +--------+-------------------------------------------+----------+
             |
            GND Fuente Externa (Tierra común unificada)
```

#### Pines y Encapsulados de los Semiconductores (Vista Frontal Cara Plana):
- **Q1 (2N2222A - NPN TO-92):** Pin 1 = Emisor (a GND), Pin 2 = Base (desde GPIO 18 vía $R_1$), Pin 3 = Colector (a Base de Q2 vía $R_2$).
- **Q2 (2N3906 - PNP TO-92):** Pin 1 = Emisor (a +5.0V), Pin 2 = Base (con $R_{pu} = 10\,\text{k}\Omega$ a +5V y $R_2 = 2.2\,\text{k}\Omega$ a Colector Q1), Pin 3 = Colector (a Gate de M1).
- **M1 (IRLZ44N - N-MOSFET TO-220):** Pin 1 = Gate (desde Colector Q2 con $R_{pd2} = 10\,\text{k}\Omega$ a GND), Pin 2 = Drain (a retorno de bomba), Pin 3 = Source (a GND).
- **D1 (1N5819 Schottky):** Cátodo (franja plateada) al borne positivo (+5V) del motor; Ánodo al borne negativo (Drain) del motor.

---

## 4. Análisis de Decisiones Eléctricas Críticas

### 4.1 Comportamiento del MOSFET a 3.3V (ESP32) vs 5V (Nano) y Diseño de Etapa de Potencia

- **Arduino Nano V3 (Accionamiento Directo Garantizado a 5.0V):**
  - El pin `D3` entrega $V_{OH} \ge 4.2\,\text{V}$ (típico $4.8\,\text{V}-4.9\,\text{V}$). A este voltaje, el **IRLZ44N** (Vishay Siliconix / Infineon) tiene su resistencia de conducción $R_{DS(on)} \le 0.035\,\Omega$ **100% garantizada por la tabla de especificaciones eléctricas de la hoja de datos**. Disipa menos de $5\,\text{mW}$ y opera totalmente saturado a temperatura ambiente con lógica directa activa en alto (`HIGH = ON`, `LOW = OFF`).
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
                     |                 [R_pu: 10 kΩ]
                     |                   |
                     |         +---------+--------- Emisor Q2 (PNP: 2N3906 / 2N2907)
                     |         |                   |
                     |       [R2: 2.2 kΩ]         Base Q2
                     |         |                   |
                     |         +-- Colector Q1     Colector Q2
                     |                (NPN)        |
                     |                             +-----> Gate MOSFET (IRLZ44N)
                     |                                       |
GPIO 18 ---[R1: 2.2 kΩ]--- Base Q1                         [R_pd2: 10 kΩ]
               |              |                              |
             [R_pd1: 10 kΩ]  Emisor Q1                     GND
               |              |
              GND            GND
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

##### 4. Criterio de Selección para Módulos Comerciales:
- **Módulo Relé 5V (SRD-05VDC-SL-C):** Si se utiliza un módulo relé, el jumper de configuración debe situarse obligatoriamente en **HIGH Level Trigger**, y los cables del motor deben conectarse entre `COM` (Común) y `NO` (Normalmente Abierto). **Bajo ninguna circunstancia configurar en Low Level Trigger**, ya que encendería la bomba durante el reset del ESP32. Si el módulo relé no conmuta con 3.3V en High Level Trigger debido a la caída de tensión del LED del optoacoplador, debe descartarse el módulo relé en favor del driver discreto NPN+PNP.
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
