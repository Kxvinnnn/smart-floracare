# 🛒 02. Lista de Materiales (BOM) y Desglose Financiero Auditado

Esta lista detalla los componentes requeridos para la construcción del sistema **Smart FloraCare** en sus dos variantes (**Arduino Nano V3** y **ESP32 DevKit V1**), con especificaciones eléctricas verificadas contra hojas de datos oficiales y cotizaciones del mercado electrónico colombiano (COP / USD a TRM de referencia $1\,\text{USD} \approx 4,000\,\text{COP}$).

---

## 📊 1. Resumen de Costos y Opciones de Construcción

El proyecto contempla el riego de una **única maceta de interior** desde un **mismo depósito compartido**. Por lo tanto, los componentes de control pueden evaluarse como proyectos independientes (Opción A u Opción B) o como una suite de desarrollo dual donde ambas placas quedan armadas y comparten la infraestructura hidráulica:

| Opción de Construcción | Grupo A: Electrónica de Control | Grupo B: Hidráulica y Potencia | Grupo C: Gabinete y Consumibles | Inversión Total Auditada (COP) | Equivalente USD ($1\,\text{USD} \approx 4,000\,\text{COP}$) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Opción A: Solo Arduino Nano V3** | \$71,100 COP | \$31,500 COP | \$15,000 COP | **\$117,600 COP** | **~$29.40 USD** |
| **Opción B: Solo ESP32 DevKit V1** | \$82,600 COP | \$31,500 COP | \$15,000 COP | **\$129,100 COP** | **~$32.28 USD** |
| **Opción Combinada (Ambos Controladores)** | \$153,700 COP | \$31,500 COP | \$24,000 COP | **\$209,200 COP** | **~$52.30 USD** |

> [!NOTE] Reconciliación Aritmética de Costos
> - Si se construye solo la **Opción A (Nano)**: $\$71,100 + \$31,500 + \$15,000 = \$117,600\,\text{COP}$.
> - Si se construye solo la **Opción B (ESP32)**: $\$82,600 + \$31,500 + \$15,000 = \$129,100\,\text{COP}$.
> - Si se compran componentes para montar **ambas placas (Opción Combinada)**:
>   - La electrónica se suma de forma completa: $\$71,100 + \$82,600 = \$153,700\,\text{COP}$.
>   - La hidráulica y fuente ($\$31,500\,\text{COP}$) **no se duplica**, ya que se riega una sola planta con el mismo depósito y bomba.
>   - En consumibles y gabinetes, se añade únicamente una segunda caja plástica ($\$9,000\,\text{COP}$), reutilizando el mismo frasco de barniz dieléctrico y el tubo termorretráctil: $\$15,000 + \$9,000 = \$24,000\,\text{COP}$.
>   - Total Combinado: $\$153,700 + \$31,500 + \$24,000 = \mathbf{\$209,200\,\text{COP}}$ (Ahorro por sinergia: $\$37,500\,\text{COP}$).

---

## 🔌 2. Grupo A: Electrónica de Control (Desglose Itemizado por Componente)

La siguiente tabla presenta el listado completo para el montaje dual (o los ítems específicos para cada placa individual):

| ID | Componente | Especificación Técnica Crítica | Cant. Dual | Asignación | Proveedor Sugerido | Precio Unit. (COP) | Subtotal Dual (COP) |
| :--- | :--- | :--- | :---: | :---: | :--- | :---: | :---: |
| **MCU-01** | **Arduino Nano V3** | ATmega328P, 16 MHz, lógica 5.0V, conector Mini-USB / USB-C | 1 | Solo Nano | Mecabot / Electronilab | \$18,000 | \$18,000 |
| **MCU-02** | **ESP32 DevKit V1** | ESP-WROOM-32 (30 pines), Dual-Core, 3.3V, Wi-Fi 802.11 b/g/n | 1 | Solo ESP32 | Electronilab / Dinastía | \$28,000 | \$28,000 |
| **DISP-01** | **Pantalla OLED 0.96"** | 128×64 monocromo, controlador SSD1306, bus I2C (0x3C) | 2 | 1 c/u | Electronilab / RoosTech | \$13,500 | \$27,000 |
| **SENS-01** | **Sensor de Suelo Capacitivo v1.2** | Salida analógica proporcional. **Nano a 5V; ESP32 obligatoriamente a 3.3V** | 2 | 1 c/u | Mecabot / RoosTech | \$6,500 | \$13,000 |
| **PWR-01** | **Driver de Bomba (Etapa Oficial ESP32)** | **Driver No Inversor NPN+PNP (2N2222A + 2N3906 + IRLZ44N) o Módulo HW-517** | 1 | Solo ESP32 | Electrónica local / Mecabot | \$4,500 | \$4,500 |
| **PWR-02** | **Driver de Bomba (Etapa Oficial Nano)** | **MOSFET IRLZ44N Logic-Level** (Vishay/Infineon, $R_{DS(on)} \le 0.035\,\Omega$ a 5V) | 1 | Solo Nano | Almacén de electrónica | \$3,000 | \$3,000 |
| **DIO-01** | **Diodo Flyback de Potencia** | **1N5819 Schottky** ($40\,\text{V}, 1\,\text{A}, V_F \approx 0.35\,\text{V}$, $<10\,\text{ns}$). Alternativa: 1N4007 | 2 | 1 c/u | Almacén de electrónica | \$600 | \$1,200 |
| **SW-01** | **Sensor de Flotador de Nivel** | Boya magnética con interruptor reed hermético (CS-CO058 / Horizontal) | 2 | 1 c/u | [Mecabot](https://www.mecabot.com/products/sensor-de-nivel-de-agua-horizontal-cs-co058) / Electronilab | \$12,900 | \$25,800 |
| **BUZ-01** | **Zumbador Pasivo (Buzzer)** | Transductor piezoeléctrico pasivo 3.3V/5V. Tono suave 850 Hz vía PWM | 2 | 1 c/u | Mecabot / RoosTech | \$1,200 | \$2,400 |
| **BTN-01** | **Pulsador Táctil (Push Button)** | Tact switch 6×6 mm de 4 pines para inserción en protoboard | 2 | 1 c/u | Almacén local | \$400 | \$800 |
| **RES-01** | **Kit de Resistencias de Carbón** | 1 kΩ (x2), 10 kΩ (x2), 220 Ω (x2), 1/4W 5% | 1 kit | Compartido | Almacén local | \$2,000 | \$2,000 |
| **BRD-01** | **Protoboard 400 Puntos** | Matriz de conexión para prototipado rápido en banco de pruebas | 2 | 1 c/u | Mecabot / RoosTech | \$6,500 | \$13,000 |
| **CAB-01** | **Cables Dupont** | Kit surtido de 40 jumpers (20 cm, M-M y M-H) | 1 kit | Compartido | Electronilab / Mecabot | \$7,000 | \$7,000 |
| **CAB-02** | **Cable Mini-USB (Nano)** | Cable de datos y programación para Arduino Nano | 1 | Solo Nano | Informática / Hogar | \$4,000 | \$4,000 |
| **CAB-03** | **Cable Micro-USB (ESP32)** | Cable de datos y programación para ESP32 | 1 | Solo ESP32 | Informática / Hogar | \$4,000 | \$4,000 |
| **SUBTOTAL GRUPO A (COMBINADO)** | | | | | | | **\$153,700 COP** |

*(Subtotal individual asignable: Nano = \$71,100 COP; ESP32 = \$82,600 COP).*

---

## 💧 3. Grupo B: Hidráulica y Alimentación Compartida (1 Sola Planta)

Estos componentes conforman la planta física de riego e impulsión, compartida entre ambos controladores:

| ID | Componente | Especificación Técnica Verificada | Cant. | Proveedor Sugerido | Precio Est. (COP) |
| :--- | :--- | :--- | :---: | :--- | :---: |
| **PMP-01** | **Microbomba Sumergible DC** | Modelo JT-DC3W (3.5V - 5V DC), caudal 80-120 L/h, consumo 200-350 mA | 1 | Robótica / Acuarios | \$8,500 |
| **HOS-01** | **Manguera Flexible Transparente** | Grado alimenticio (silicona o PVC atóxico), diámetro interno 6 mm o 7 mm | 1 m | Ferreterías / Acuarios | \$3,000 |
| **PWR-SRC** | **Fuente Regulada 5V DC (2A a 3A)** | Adaptador de pared 5V DC estabilizado con conector Jack DC o terminales | 1 | Electrónica / Hogar | \$16,000 |
| **RES-TNK** | **Depósito de Agua Estable** | Contenedor plástico hermético reciclado con tapa (1.5 a 3 Litros) | 1 | Hogar / Bazar | \$0 |
| **TRP-01** | **Bandeja Plástica Antiderrames** | Bandeja rectangular para protección del banco durante pruebas hidráulicas | 1 | Hogar / Ferretería | \$4,000 |
| **SUBTOTAL GRUPO B** | | | | | **\$31,500 COP** |

---

## 🛠️ 4. Grupo C: Gabinetes, Protecciones y Consumibles de Taller

| ID | Componente | Especificación Técnica | Cant. Dual | Función de Seguridad | Precio Est. (COP) |
| :--- | :--- | :--- | :---: | :--- | :---: |
| **ENC-01** | **Caja Estanca PVC 100×100×50 mm** | Gabinete plástico hermético para alojamiento de placa definitiva | 2 | Protección IP54 contra salpicaduras | \$18,000 (\$9,000 c/u) |
| **INS-01** | **Barniz Aislante / Esmalte de Uñas** | Barniz acrílico o poliuretano de alta rigidez dieléctrica | 1 frasco | Sellado de componentes y pistas superiores del sensor | \$3,500 |
| **SHR-01** | **Tubo Termorretráctil Surtido** | Tramos de 10 mm y 15 mm para empalmes y protección del sensor | 1 m | Aislamiento dieléctrico y refuerzo mecánico | \$2,500 |
| **SUBTOTAL GRUPO C (COMBINADO)** | | | | | **\$24,000 COP** |

*(Subtotal individual para 1 sola variante: 1 caja + barniz + termorretráctil = \$15,000 COP).*

---

## ⚠️ 5. Puntos Eléctricos Críticos y Criterios Definitivos de Selección

1. **Selección del Driver de Bomba para ESP32:**
   - **FQP30N06L:** Ha sido **rechazado oficialmente** debido a su umbral de compuerta ($V_{GS(th)} = 1.0\,\text{V}-2.5\,\text{V}$), que en condiciones límite no garantiza conducción a 3.3V.
   - **IRLZ44N Directo:** No posee especificación garantizada de $R_{DS(on)}$ para $V_{GS} \le 3.3\,\text{V}$ en la tabla oficial de Vishay/Infineon.
   - **Pre-Driver NPN Simple Inversor:** **PROHIBIDO** porque encendería la bomba en el arranque cuando el GPIO está en LOW o High-Z.
   - **Opción Oficial y Recomendada en Taller:** **Driver No Inversor Bipolar de 2 Etapas (NPN 2N2222A + PNP 2N3906)** excitando la compuerta del IRLZ44N a +4.85V plenos, o **Buffer Lógico TTL 74AHCT125 / 74HCT14**, garantizando lógica activa en alto (`HIGH = ON`, `LOW = OFF`) y apagado estricto durante el arranque y reinicio.
   - **Alternativa Comercial:** Módulo HW-517 (Dual AOD4184A) con verificación previa en banco de $V_{DS} \le 50\,\text{mV}$ bajo carga real. Si se opta por un Módulo Relé 5V, este debe configurarse obligatoriamente con disparo en alto (**High Level Trigger**) y comprobarse que conmute de forma fiable con 3.3V.
2. **Alimentación del Sensor Capacitivo de Suelo:**
   - En **Arduino Nano**: Alimentado a 5.0V. Su salida entrega entre ~2.4V (agua) y ~4.8V (aire), operando dentro del rango del ADC de 10 bits.
   - En **ESP32**: **Obligatoriamente alimentado a 3.3V**. Conectarlo a 5V generaría tensiones superiores a los 3.3V tolerados por el `GPIO 34`, dañando de forma permanente el conversor ADC del ESP32.
3. **Diodo Flyback de Conmutación:**
   - Debe instalarse un **1N5819 Schottky** ($1\,\text{A}, 40\,\text{V}$, conmutación $<10\,\text{ns}$) o alternativamente un **1N4007** en antiparalelo directo con los terminales del motor. Proporciona una ruta de recirculación para la corriente inductiva transitoria; su eficacia contra reinicios y ruidos electromagnéticos debe confirmarse en el montaje real.
4. **Distribución de Potencia:**
   - La bomba toma su corriente en paralelo directamente de la fuente externa de 5V (2A–3A). **Bajo ninguna circunstancia debe alimentarse el motor desde los pines 5V o 3V3 de la placa de desarrollo**.
