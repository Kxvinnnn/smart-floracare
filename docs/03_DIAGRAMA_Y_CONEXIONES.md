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

## 3. Diagrama Esquemático de Potencia y Señal Unificado

```text
               +5V Fuente Externa Regulada (2A - 3A)
                  |
         +--------+--------------------------+
         |                                   |
       [5V / VIN Micro]                 Bomba 5V (+)
    +-------------------+                    |
    | MICROCONTROLADOR  |                  [ M ]
    | (Nano o ESP32)    |                    |
    |                   |               +----+----+
    |   PIN GATE BOMBA  |               |         |
    |   (D3 o GPIO18)   |          [DIODO FLYBACK]
    |         |         |          (1N5819 / 1N4007)
    |       [1 kΩ]      |          (Cátodo al +)  |
    |         |         |               |         |
    |         +------------> Gate       +----+----+
    |                   |                    |
    |        [10 kΩ]    |               Drain| (IRLZ44N o FQP30N06L)
    |      (Pull-down)  |               +----+
    |         |         |               |  MOSFET N-Channel
    |        GND        |               +----+
    |                   |                    | Source
    |   PIN BUZZER      |                    |
    |   (D5 o GPIO23)   |                    |
    |         |         |                    |
    |       [220 Ω]     |                    |
    |         |         |                    |
    |   [BUZZER PASIVO] |                    |
    |         |         |                    |
    |        GND        |                    |
    |                   |                    |
    |   FLOTADOR (D4/19)---------[BOYA MECÁNICA]--------> GND (INPUT_PULLUP)
    |   BOTÓN (D2/4)------------[TACT SWITCH]-----------> GND (INPUT_PULLUP)
    |                   |                    |
    |        GND        |                    |
    +---------+---------+--------------------+
              |
             GND Fuente Externa (Tierra común unificada)
```

---

## 4. Análisis de Decisiones Eléctricas Críticas

### 4.1 Comportamiento del MOSFET a 3.3V (ESP32) vs 5V (Nano) y Criterio de Selección

- **Arduino Nano V3 (Accionamiento Directo Garantizado a 5.0V):**
  - El pin `D3` entrega $V_{OH} \ge 4.2\,\text{V}$ (típico $4.8\,\text{V}-4.9\,\text{V}$). A este voltaje, el **IRLZ44N** (Vishay Siliconix / Infineon) tiene su resistencia de conducción $R_{DS(on)} \le 0.035\,\Omega$ **100% garantizada por la tabla de especificaciones eléctricas de la hoja de datos**. Disipa menos de $5\,\text{mW}$ y opera totalmente saturado a temperatura ambiente.
- **ESP32 DevKit V1 (Accionamiento a 3.3V):**
  - **Hoja de Datos Oficial del IRLZ44N:** No contiene especificación garantizada de $R_{DS(on)}$ para tensiones de compuerta $\le 3.3\,\text{V}$ (solo especifica hasta $4.0\,\text{V}$ en tabla). Aunque las curvas típicas de laboratorio a $25^\circ\text{C}$ muestran conducción suficiente para cargas de $300\,\text{mA}$, una unidad con tolerancia desfavorable ($V_{GS(th)} \to 2.0\,\text{V}$) reduce el sobrevoltaje efectivo a $\approx 1.1\,\text{V}$.
  - **Decisión de Ingeniería para ESP32:** Se establece como **etapa de potencia oficial de producción un Módulo MOSFET con Optoacoplador / Pre-Driver (LR7843, AOD4184 o Módulo Relé 5V)**. Estos módulos aceptan la señal lógica de 3.3V en su entrada y conmutan la compuerta usando los 5.0V de la fuente principal, garantizando saturación plena de fábrica.
  - **Accionamiento Directo de IRLZ44N en ESP32:** Se clasifica estrictamente como **alternativa experimental de banco**, no apta para compra definitiva sin antes verificar con multímetro que $V_{GS} \ge 3.1\,\text{V}$, $V_{DS} \le 0.05\,\text{V}$ y temperatura $< 40^\circ\text{C}$ bajo carga real.
  - **FQP30N06L:** **DESCARTADO FORMALMENTE** para el ESP32. Su $V_{GS(th)}$ alcanza hasta $2.5\,\text{V}$, dejando un margen de sobretensión inaceptable de apenas $0.8\,\text{V}$ a 3.3V.

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
