# 🛒 02. Lista de Materiales (BOM) y Guía de Adquisición Auditada

Esta lista detalla los materiales necesarios para construir **ambas versiones físicas (Arduino Nano V3 y ESP32 DevKit V1)**, permitiendo probar cada sistema de forma independiente sobre la **misma planta de interior y el mismo depósito**, maximizando la eficiencia de costos.

---

## 📦 Clasificación de la Lista de Compra

1. **Grupo A (Electrónica Duplicada):** Componentes necesarios para ensamblar los dos controladores independientes.
2. **Grupo B (Hidráulica y Montaje Compartido):** Materiales compartidos (bomba, manguera, maceta, depósito) para probar un controlador a la vez.
3. **Grupo C (Herramientas y Consumibles de Taller):** Comprar únicamente lo que no tengas disponible en casa.

---

## 🔌 Grupo A: Electrónica (Controladores Nano y ESP32)

| Componente | Especificación Técnica Verificada | Cantidad | Referencia / Proveedor Local (Colombia) | Precio Est. Unitario (COP / USD) | Subtotal Est. (COP) |
| :--- | :--- | :---: | :--- | :--- | :--- |
| **Arduino Nano V3** | ATmega328P, 16 MHz, 5V, conector USB Mini o USB-C | 1 | Tiendas locales / Dinastía Tecnológica / Mecabot | \$16,000 - \$22,000 COP (~$4.50 USD) | \$18,000 COP |
| **ESP32 DevKit V1** | ESP32-WROOM-32 (30 o 38 pines), Dual-Core, 3.3V, WiFi | 1 | Electronilab / Mecabot / Sigma Electrónica | \$24,000 - \$32,000 COP (~$6.50 USD) | \$28,000 COP |
| **Sensor Capacitivo de Suelo v1.2** | Salida analógica proporcional a capacitancia (chip 555 real). **En ESP32 se alimenta a 3.3V; en Nano a 5V** | 2 | Electronilab / Dinastía / Mecabot | \$6,000 - \$9,000 COP (~$1.80 USD) | \$15,000 COP |
| **Pantalla OLED 0.96" I2C** | 128×64 píxeles, controlador SSD1306, interfaz I2C (dirección 0x3C) | 2 | Electronilab / RoosTech / MercadoLibre | \$12,000 - \$16,000 COP (~$3.50 USD) | \$28,000 COP |
| **MOSFET de Potencia N-Channel** | **IRLZ44N** o **FQP30N06L** (Compuerta de nivel lógico $V_{GS(th)} \le 2.0\text{V}$, $R_{DS(on)} < 0.035\,\Omega$ a 3.3V) | 2 | Tiendas de componentes electrónicos | \$2,500 - \$4,000 COP (~$0.80 USD) | \$6,000 COP |
| **Diodo Flyback de Potencia** | **1N5819 (Schottky $1\text{A}, 40\text{V}$, $V_F \approx 0.35\text{V}$)** o **1N4007**. *(¡PROHIBIDO usar 1N4148 por baja corriente!)* | 2 | Tiendas de componentes electrónicos | \$500 - \$1,000 COP (~$0.20 USD) | \$1,500 COP |
| **Resistencias de Compuerta** | $1\,\text{k}\Omega$, ¼ W, 5% tolerancia (limitadora de corriente del pin GPIO) | 2 | Tiendas de componentes electrónicos | \$100 - \$200 COP | \$300 COP |
| **Resistencias Pull-Down** | $10\,\text{k}\Omega$, ¼ W, 5% (fijación de masa en Gate para evitar flotación) | 2 | Tiendas de componentes electrónicos | \$100 - \$200 COP | \$300 COP |
| **Sensor de Flotador de Nivel** | Tipo Boya magnética con contacto seco hermético (Horizontal CS-CO058 o Vertical) | 2 | [Electronilab](https://electronilab.co/tienda/sensor-horizontal-de-nivel-de-agua-plastico-flotador-interruptor/) / [Mecabot](https://www.mecabot.com/products/sensor-de-nivel-de-agua-horizontal-cs-co058) | \$11,900 - \$16,900 COP (~$3.50 USD) | \$28,000 COP |
| **Zumbador Pasivo (Buzzer)** | Piezoeléctrico pasivo 5V/3.3V (Tipo TMB12A05). Generación de tono suave 850 Hz con `tone()` | 2 | Tiendas de componentes electrónicos | \$1,000 - \$1,800 COP (~$0.30 USD) | \$2,500 COP |
| **Resistencias para Buzzer** | $220\,\Omega$, ¼ W (Atenuación acústica de confort y protección del pin) | 2 | Tiendas de componentes electrónicos | \$100 - \$200 COP | \$300 COP |
| **Pulsador Manual de Riego** | Tact Switch 6×6 mm de 4 patas para protoboard | 2 | Tiendas de componentes electrónicos | \$300 - \$500 COP | \$800 COP |
| **Protoboard o Placa Perforada** | Protoboard mini de 400 puntos (o placa PCB perforada para montaje final) | 2 | Tiendas de robótica local | \$6,000 - \$9,000 COP (~$1.80 USD) | \$15,000 COP |
| **Cables Dupont** | Kit surtido de 40 cables (Macho-Macho y Macho-Hembra de 20 cm) | 1 kit | Tiendas de robótica local | \$6,000 - \$8,000 COP (~$1.80 USD) | \$7,000 COP |
| **Cables USB de Programación** | 1x Mini-USB (Nano) + 1x Micro-USB o Tipo-C (según ESP32 adquirido) | 2 | Disponibles en casa o tiendas de informática | \$4,000 - \$8,000 COP | \$8,000 COP |
| **Fuentes Reguladas 5V (2A - 3A)** | Adaptador de pared 5V DC regulado con conector USB o Jack DC | 2 | Tiendas de cargadores o electrónica | \$14,000 - \$20,000 COP (~$4.50 USD) | \$32,000 COP |
| **Gabinete / Caja de Proyecto** | Caja plástica para proyectos con tapa transparente o estanca | 2 | Ferreterías / Tiendas de electrónica | \$8,000 - \$14,000 COP (~$2.50 USD) | \$20,000 COP |
| **SUBTOTAL GRUPO A** | | | | | **~\$210,700 COP (~$52 USD)** |

---

## 💧 Grupo B: Hidráulica y Montaje Compartido (1 sola Planta)

Como ambas versiones se probarán secuencialmente sobre la **misma maceta y el mismo depósito**, solo necesitas comprar una unidad de cada elemento hidráulico:

| Componente | Especificación Técnica | Cantidad | Dónde Comprar | Precio Est. (COP / USD) |
| :--- | :--- | :---: | :--- | :--- |
| **Microbomba Sumergible 5V DC** | Modelo horizontal JT-DC3W (3V - 6V DC), caudal 80-120 L/h, consumo 150-300 mA | 1 | Tiendas de robótica / acuarios | \$8,000 - \$12,000 COP (~$2.50 USD) |
| **Manguera Flexible Transparente** | Silicona o vinilo grado alimenticio, diámetro interno 6 mm o 7 mm | 1 metro | Ferreterías / Acuarios | \$3,000 - \$5,000 COP (~$1.00 USD) |
| **Depósito de Agua Estable** | Frasco o contenedor plástico de boca ancha con tapa (capacidad 1.5 a 3 Litros) | 1 | Hogar / Almacenes de cadena | \$0 - \$8,000 COP |
| **Maceta y Planta de Interior** | Lirio de la paz (*Spathiphyllum*), Pothos o Helecho de tu abuela | 1 | Vivero local / Ya disponible | \$0 COP |
| **Bandeja Plástica Antiderrames** | Bandeja rectangular para contener posibles reboses durante pruebas de banco | 1 | Hogar / Ferretería | \$4,000 - \$8,000 COP (~$1.50 USD) |
| **Prensaestopas / Pasacables** | Diámetro PG7 o goma pasamuros para sellar entradas de cable en la caja | 4 uds | Ferreterías eléctricas | \$4,000 - \$6,000 COP (~$1.20 USD) |
| **Bridas y Estacas Plásticas** | Bridas de nylon de 10 cm y clip para fijar manguera sobre la tierra | 1 paquete | Ferretería | \$3,000 - \$4,000 COP (~$0.80 USD) |
| **SUBTOTAL GRUPO B** | | | | **~\$25,000 - \$35,000 COP (~$7.50 USD)** |

---

## 🛠️ Grupo C: Herramientas y Consumibles de Taller

*Revisa tu caja de herramientas antes de comprar; la mayoría de estos elementos suelen estar disponibles en casa:*

- **Multímetro digital:** Para verificar continuidad, 5V de alimentación y 3.3V en el sensor.
- **Pelacables y alicate de corte pequeño:** Para conexionado limpio.
- **Soldador de estaño y alambre de soldar:** Solo si se ensambla en placa perforada definitiva.
- **Esmalte de uñas transparente o laca acrílica:** Para impermeabilizar la cabecera del sensor capacitivo.
- **Tubo termorretráctil (Heat-shrink):** Para aislar empalmes del motor y sensores.
- **Destornillador perillero plano/estrella:** Para ajustar bornes y prensacables.

---

## ⚠️ Puntos Eléctricos Críticos que NO Deben Cambiarse

1. **El Sensor Capacitivo en ESP32 va a 3.3V:** Jamás conectes el cable rojo del sensor a 5V en el ESP32. Debe conectarse al pin `3V3` de la placa.
2. **Diodo Flyback 1N5819 Schottky:** No permitas que el vendedor te entregue un diodo 1N4148 en sustitución. El 1N4148 no tiene capacidad para la corriente inductiva del motor. Si no tienen 1N5819, pide un **1N4007** o **1N5822**.
3. **MOSFET Logic-Level:** Verifica en el encapsulado que la serigrafía diga **IRLZ44N** (la letra 'L' significa Logic-Level). El modelo IRFZ44N (con 'F') **NO** se satura con 3.3V ni con 5V y recalentará.
4. **Fuente de 5V 2A a 3A:** La bomba toma su corriente en paralelo directamente de la fuente de 5V, **NUNCA** a través del pin 5V del microcontrolador.
