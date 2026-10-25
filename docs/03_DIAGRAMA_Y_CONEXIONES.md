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
| **Driver de Bomba (MOSFET)** | `Gate` | `GPIO 18` (vía R 1 kΩ) | Señal de disparo 3.3V (apta para IRLZ44N/FQP30N06L). Pull-down 10 kΩ a GND |
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

### 4.1 Comportamiento del MOSFET a 3.3V (ESP32) vs 5V (Nano)
- **IRLZ44N:** Es un MOSFET de canal N diseñado específicamente con compuerta de nivel lógico (*Logic-Level*). Su tensión de umbral es $V_{GS(th)} = 1.0\,\text{V} - 2.0\,\text{V}$.
  - A $V_{GS} = 3.3\,\text{V}$ (ESP32): La resistencia de conducción es $R_{DS(on)} \approx 0.035\,\Omega$. Para la microbomba que demanda entre $0.2\,\text{A}$ y $0.5\,\text{A}$, la caída de tensión en el MOSFET es menor a $0.018\,\text{V}$ y disipa $< 0.01\,\text{W}$. Funciona 100% frío y saturado.
  - A $V_{GS} = 5.0\,\text{V}$ (Nano): $R_{DS(on)} \le 0.028\,\Omega$. Totalmente saturado.
- **FQP30N06L:** Alternativa excelente equivalente con $V_{GS(th)} = 1.0\,\text{V} - 2.5\,\text{V}$, capaz de manejar hasta $15\,\text{A}$ a $3.3\,\text{V}$.
- *Conclusión:* Ambos modelos son 100% seguros y aptos para ambas placas.

### 4.2 Selección del Diodo Flyback: ¿Por qué NO usar 1N4148?
- **1N4148:** Es un diodo de señal pequeña de conmutación rápida para señales de alta frecuencia. Su corriente directa continua máxima es de solo $150\,\text{mA}-200\,\text{mA}$. La microbomba puede generar picos de corriente transitoria inductiva de más de $500\,\text{mA}$ al apagarse. **El 1N4148 puede quemarse en cortocircuito y destruir la compuerta del MOSFET.**
- **1N5819 (Diodo Schottky - Elección Principal):** Soporta $1.0\,\text{A}$ continuo y $25\,\text{A}$ de sobretensión. Tiene un tiempo de recuperación ultra rápido ($< 10\,\text{ns}$) y una caída directa mínima ($V_F \approx 0.35\,\text{V}$). Es la opción perfecta para motores de 5V.
- **1N4007 (Diodo Rectificador - Alternativa Aceptable):** Soporta $1.0\,\text{A}$ continuo y $30\,\text{A}$ de pico transitorio. Su recuperación es más lenta (~$2\,\mu\text{s}$), pero amortigua eficazmente el pico inductivo en motores de corriente continua pequeños.

### 4.3 Dimensionamiento de la Fuente Regulada de 5V (2A a 3A)
- Consumo pico simultáneo en el peor escenario:
  - Microbomba 5V DC: $250\,\text{mA}$ nominal, pico de arranque transitorio de $600\,\text{mA}-800\,\text{mA}$.
  - ESP32 transmitiendo WiFi a máxima potencia: $240\,\text{mA}-350\,\text{mA}$.
  - Pantalla OLED SSD1306 blanca con todos los píxeles encendidos: $30\,\text{mA}-40\,\text{mA}$.
  - Microcontrolador Nano / periféricos pasivos: $\approx 50\,\text{mA}$.
- **Consumo pico máximo total:** $\approx 1.25\,\text{A}$.
- Con una fuente de **5V y 2A a 3A**, el sistema opera con un margen de holgura superior al 60%, evitando caídas de tensión bruscas (*brownouts*) que reiniciarían el ESP32 o el Nano cuando la bomba arranque.
