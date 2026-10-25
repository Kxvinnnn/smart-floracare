# ⚡ 03. Diagrama Esquemático y Mapa de Conexiones

Este documento proporciona los esquemas detallados de cableado pin a pin para la construcción del circuito.

---

## 1. Mapa de Pines: Arduino Nano (Versión Original & Mejorada)

| Dispositivo / Módulo | Pin del Dispositivo | Pin en Arduino Nano | Descripción / Nota |
| :--- | :--- | :--- | :--- |
| **Sensor de Humedad** | `VCC` | `5V` | Alimentación del sensor |
| | `GND` | `GND` | Tierra común |
| | `AOUT` | `A0` | Lectura analógica de humedad (0 - 1023) |
| **Pantalla OLED SSD1306** | `VCC` | `5V` (o `3.3V` según pantalla) | Alimentación |
| | `GND` | `GND` | Tierra común |
| | `SCL` | `A5` | Reloj I2C (I2C SCL) |
| | `SDA` | `A4` | Datos I2C (I2C SDA) |
| **Driver de Bomba (MOSFET/TIP120)** | `Gate` (o Base) | `D3` (a través de R 1kΩ) | Señal digital de activación PWM/ON-OFF |
| | `Source` (o Emisor) | `GND` | Conexión a tierra común |
| | `Drain` (o Colector) | Bomba (-) | Retorno conmutado de la bomba |
| **Bomba Sumergible 5V** | Positivo (+) | `5V` (Fuente de poder) | Alimentación positiva continua |
| | Negativo (-) | `Drain` (MOSFET) / `Colector` (TIP120) | Conmutado a tierra |
| **Diodo de Protección (1N4007)** | Cátodo (Banda plateada) | Bomba (+) / `5V` | Protección flyback antiparalelo |
| | Ánodo | Bomba (-) / Drain | Conectado a la salida conmutada |
| **Sensor de Flotador (Horizontal o Vertical)** | Terminal 1 | `D4` | Entrada digital con `INPUT_PULLUP` interno (LOW = nivel de agua normal, HIGH = vacío) |
| | Terminal 2 | `GND` | Conexión a tierra común |
| **Buzzer Pasivo (TMB12A05)** | Positivo (+) | `D5` (vía Resistencia 100 Ω - 220 Ω) | Generación de tono suave con función `tone()` en código |
| | Negativo (-) | `GND` | Tierra común |
| **Pulsador Manual (Tact Switch 6x6mm)** | Terminal 1 (Pata 1) | `D2` | Entrada digital con `INPUT_PULLUP` interno (lee LOW al presionar) |
| | Terminal 2 (Pata diagonal) | `GND` | Conexión a tierra común |

---

## 2. Diagrama Eléctrico Conceptual (Circuito de Potencia y Señal)

```text
               +5V Fuente Externa (2A)
                  |
         +--------+------------------+-----------------+
         |                           |                 |
       [VIN/5V]                 Bomba 5V (+)           |
    +-----------+                    |                 |
    |  ARDUINO  |                  [ M ]            [ OLED / SENSOR ]
    |   NANO    |                    |                 |
    |           |               +----+----+            +5V / GND
    |      D3   |               |         |
    |      |    |            [1N4007]     |
    |   [1kΩ]   |           (Cátodo +)    |
    |      |    |               |         |
    |      +----+----> Gate     +----+----+
    |           |                    |
    |        [10kΩ]             Drain| (IRLZ44N o FQP30N06L)
    |           |               +----+
    |          GND              |  MOSFET N-Channel
    |                           +----+
    |      D5   |                    | Source
    |   [220Ω]  |                    |
    |      |    |                    |
    |   [BUZZER PASIVO]              |
    |      |    |                    |
    |      D4---+---> [FLOTADOR (Horizontal/Vertical)] ---> GND (INPUT_PULLUP)
    |      D2---+---> [BOTON MANUAL (Tact 6x6mm)]      ---> GND (INPUT_PULLUP)
    |           |                    |
    |  GND      |                    |
    +---+-------+--------------------+-----------------+
        |
       GND Fuente Externa (Tierra común)
```

---

## 3. Esquema Alternativo: ESP32 DevKit V1

Si decides armar la versión con conectividad WiFi:

| Dispositivo | Pin en ESP32 DevKit V1 | Nota |
| :--- | :--- | :--- |
| **Sensor Humedad (AOUT)** | `GPIO 34` (ADC1_CH6) | Los pines de solo entrada `GPIO 34, 35, 36, 39` son ideales para ADC |
| **OLED SCL** | `GPIO 22` | SCL por defecto en ESP32 |
| **OLED SDA** | `GPIO 21` | SDA por defecto en ESP32 |
| **Gate Driver Bomba** | `GPIO 18` | Salida digital 3.3V (apta para MOSFET nivel lógico IRLZ44N) |
| **Sensor Flotador** | `GPIO 19` | Configurado como `INPUT_PULLUP` |
| **Buzzer Activo** | `GPIO 23` | Salida digital 3.3V |
| **Botón Manual** | `GPIO 4` | Configurado como `INPUT_PULLUP` |

---

## 4. Reglas de Oro de Seguridad Eléctrica

> [!IMPORTANT]
> 1. **Diodo 1N4007 Obligatorio:** Los motores generan una fuerza contraelectromotriz (fem inversa) de hasta decenas de voltios cuando se apagan repentinamente. Sin el diodo antiparalelo, este pico destruirá el transistor y puede quemar el microcontrolador.
> 2. **Tierra Común (Common Ground):** Todos los terminales GND (fuente, Arduino, sensores, MOSFET) **deben estar unidos entre sí**.
> 3. **No alimentar la bomba desde el pin 5V del Arduino:** Si conectas la bomba directamente a la salida del regulador del Arduino, la sobrecarga reiniciará el microcontrolador o quemará el regulador AMS1117. La bomba debe tomar sus 5V directamente de la fuente de alimentación externa.
