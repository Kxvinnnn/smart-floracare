# 🛒 02. Lista de Materiales (BOM) y Desglose Financiero Auditado

Esta lista detalla los componentes requeridos para la construcción del sistema **Smart FloraCare** en sus dos variantes (**Arduino Nano V3** y **ESP32 DevKit V1**), con especificaciones eléctricas verificadas contra hojas de datos oficiales y cotizaciones del mercado electrónico colombiano (COP / USD).

---

## 📊 1. Resumen de Costos y Opciones de Construcción

Como ambas versiones se probarán sobre la **misma planta de interior y el mismo depósito**, el sistema hidráulico se comparte de forma alternada:

| Opción de Construcción | Electrónica y Control | Hidráulica Compartida | Gabinete y Consumibles | Inversión Total Estimada (COP) | Equivalente USD (1 USD $\approx$ 4,000 COP) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Solo Versión Arduino Nano V3** | \$68,000 COP | \$25,000 COP | \$12,000 COP | **\$105,000 COP** | **~$26.25 USD** |
| **Solo Versión ESP32 DevKit V1** | \$78,000 COP | \$25,000 COP | \$12,000 COP | **\$115,000 COP** | **~$28.75 USD** |
| **Ambas Versiones (Duplicadas + Compartidas)** | \$146,000 COP | \$25,000 COP | \$24,000 COP | **\$195,000 - \$210,000 COP** | **~$48.75 - \$52.50 USD** |
| **Costo Incremental 2ª Versión** | + \$78,000 COP | \$0 COP (Reutilizada) | + \$12,000 COP | **+ \$90,000 COP** | **~$22.50 USD** |

---

## 🔌 2. Grupo A: Electrónica de Control (Desglose por Variante)

### A.1 Componentes Exclusivos / Duplicados

| Identificador | Componente | Especificación Técnica Verificada | Cant. | Compatibilidad | Proveedor Local Sugerido | Precio Unit. (COP) | Subtotal (COP) |
| :--- | :--- | :--- | :---: | :---: | :--- | :---: | :---: |
| **MCU-01** | **Arduino Nano V3** | ATmega328P, 16 MHz, 5V, conector Mini-USB / USB-C | 1 | Variante A | Mecabot / Electronilab | \$18,000 | \$18,000 |
| **MCU-02** | **ESP32 DevKit V1** | ESP-WROOM-32 (30 pines), Dual-Core, 3.3V, Wi-Fi / BLE | 1 | Variante B | Electronilab / Dinastía | \$28,000 | \$28,000 |
| **DISP-01** | **Pantalla OLED 0.96"** | 128×64 monocromo, controlador SSD1306, interfaz I2C (0x3C) | 2 | Ambas (3.3V / 5V) | Electronilab / RoosTech | \$13,500 | \$27,000 |
| **SENS-01** | **Sensor de Suelo Capacitivo v1.2** | Salida analógica proporcional. **Nano a 5V; ESP32 obligatoriamente a 3.3V** | 2 | Ambas | Mecabot / RoosTech | \$6,500 | \$13,000 |
| **PWR-01** | **Driver de Bomba (Opción 1: Módulo con Driver/Opto)** | **Módulo MOSFET LR7843 / AOD4184 o Módulo Relé 5V** con pre-driver de disparo | 1 | **Recomendado ESP32** | Mecabot / Electronilab | \$4,500 | \$4,500 |
| **PWR-02** | **Driver de Bomba (Opción 2: Transistor Discreto)** | **MOSFET IRLZ44N Logic-Level** (Vishay, $V_{GS(th)} \le 2.0\text{V}$, $R_{DS(on)} \le 0.035\,\Omega$ a 5V) | 2 | **Nativo Nano** / ESP32 condicionado | Almacén de electrónica | \$3,000 | \$6,000 |
| **DIO-01** | **Diodo Flyback de Potencia** | **1N5819 Schottky** ($40\,\text{V}, 1\,\text{A}, V_F \approx 0.35\,\text{V}$) o 1N4007. *(¡No 1N4148!)* | 2 | Ambas | Almacén de electrónica | \$600 | \$1,200 |
| **SW-01** | **Sensor de Flotador de Nivel** | Tipo Boya magnética con contacto seco hermético (CS-CO058) | 2 | Ambas (`INPUT_PULLUP`) | [Mecabot](https://www.mecabot.com/products/sensor-de-nivel-de-agua-horizontal-cs-co058) / Electronilab | \$12,900 | \$25,800 |
| **BUZ-01** | **Zumbador Pasivo (Buzzer)** | Piezoeléctrico pasivo 5V/3.3V (TMB12A05). Tono suave 850 Hz vía `tone()` | 2 | Ambas | Mecabot / RoosTech | \$1,200 | \$2,400 |
| **BTN-01** | **Pulsador Táctil (Push Button)** | Tact switch 6×6 mm de 4 pines para protoboard | 2 | Ambas (`INPUT_PULLUP`) | Almacén local | \$400 | \$800 |
| **RES-01** | **Kit de Resistencias** | 1 kΩ (x2), 10 kΩ (x2), 220 Ω (x2), 1/4W 5% | 1 kit | Ambas | Almacén local | \$2,000 | \$2,000 |
| **BRD-01** | **Protoboard 400 Puntos** | Mini protoboard para ensamble y pruebas de banco | 2 | Ambas | Mecabot / RoosTech | \$6,500 | \$13,000 |
| **CAB-01** | **Cables Dupont** | Kit surtido de 40 jumpers (M-M y M-H de 20 cm) | 1 kit | Ambas | Electronilab / Mecabot | \$7,000 | \$7,000 |
| **CAB-02** | **Cables USB** | 1x Mini-USB (Nano) y 1x Micro-USB (ESP32) | 2 | Exclusivo por placa | Hogar / Informática | \$4,000 | \$8,000 |

---

## 💧 3. Grupo B: Hidráulica y Mecánica Compartida (1 Sola Planta)

Estos componentes se conectan al controlador activo de forma intercambiable:

| Identificador | Componente | Especificación Técnica | Cant. | Proveedor Sugerido | Precio Est. (COP) |
| :--- | :--- | :--- | :---: | :--- | :---: |
| **PMP-01** | **Microbomba Sumergible DC** | Modelo JT-DC3W (3V - 5V DC), caudal 80-120 L/h, consumo 200-350 mA | 1 | Tiendas de robótica / Acuarios | \$8,500 |
| **HOS-01** | **Manguera Flexible Transparente** | Silicona o vinilo grado alimenticio, diámetro interno 6 mm o 7 mm | 1 m | Ferreterías / Acuarios | \$3,000 |
| **PWR-SRC** | **Fuente Regulada 5V (2A a 3A)** | Adaptador de pared 5V DC regulado con conector Jack DC o USB | 1 | Tiendas de electrónica / Hogar | \$16,000 |
| **RES-TNK** | **Depósito de Agua Estable** | Frasco o contenedor plástico hermético con tapa (1.5 a 3 Litros) | 1 | Hogar / Bazar | \$0 - \$5,000 |
| **PLT-01** | **Maceta y Planta de Interior** | Lirio de la paz (*Spathiphyllum*), Pothos o Helecho de tu abuela | 1 | Hogar / Ya disponible | \$0 |
| **TRP-01** | **Bandeja Plástica Antiderrames** | Bandeja rectangular para banco de pruebas | 1 | Hogar / Ferretería | \$4,000 |
| **SUBTOTAL GRUPO B** | | | | | **~\$31,500 - \$36,500 COP** |

---

## 🛠️ 4. Grupo C: Gabinetes, Protecciones y Consumibles de Taller

| Identificador | Componente | Especificación Técnica | Cant. | Función | Precio Est. (COP) |
| :--- | :--- | :--- | :---: | :--- | :---: |
| **ENC-01** | **Caja Estanca Plástica (Gabinete)** | Caja de conexiones PVC 100×100×50 mm (1 por controlador definitivo) | 2 | Alojamiento y protección contra salpicaduras | \$18,000 (\$9,000 c/u) |
| **INS-01** | **Barniz Aislante / Esmalte** | Esmalte transparente de poliuretano | 1 | Impermeabilización de cabecera del sensor capacitivo | \$3,500 |
| **SHR-01** | **Tubo Termorretráctil** | Variados (diámetro 15 mm para sensor y empalmes) | 1 m | Aislamiento dieléctrico y sujeción | \$2,500 |
| **SUBTOTAL GRUPO C** | | | | | **~\$24,000 COP** |

---

## ⚠️ 5. Puntos Eléctricos y Bloqueos de Compra Resueltos

1. **Alimentación del Sensor Capacitivo:**
   - En Arduino Nano: Alimentado a 5.0V. Su salida entrega entre ~2.4V (agua) y ~4.8V (aire), aprovechando el rango completo del ADC de 10 bits.
   - En ESP32: **Obligatoriamente alimentado a 3.3V**. Jamás alimentar a 5V en el ESP32, ya que la salida superaría los 3.3V tolerados por el `GPIO 34`, destruyendo el silicio del conversor analógico-digital.
2. **Accionamiento del MOSFET de Potencia:**
   - Para Arduino Nano: El **IRLZ44N** es óptimo y está 100% garantizado por hoja de datos a 5V ($R_{DS(on)} \le 0.035\,\Omega$).
   - Para ESP32: Se recomienda adquirir un **Módulo MOSFET con Optoacoplador / Driver (LR7843 o AOD4184)** o intercalar un pequeño transistor NPN (2N2222 / 2N3904) para conmutar la compuerta a 5.0V. Si se utiliza el IRLZ44N directo a 3.3V, debe validarse en banco que $V_{GS} \ge 3.1\,\text{V}$ y que la caída $V_{DS} \le 0.05\,\text{V}$.
3. **Diodo Flyback:**
   - Debe adquirirse un diodo **1N5819 Schottky** ($1\text{A}, 40\text{V}$, $<10\text{ns}$) o en su defecto un **1N4007**. No aceptar 1N4148 por insuficiencia de corriente de avalancha inversa.
4. **Alimentación General:**
   - La bomba toma su corriente en paralelo directamente de la fuente de 5V (2A–3A), **NUNCA a través del pin 5V o 3V3 del microcontrolador**.
