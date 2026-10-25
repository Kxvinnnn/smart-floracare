# 🛒 02. Lista de Materiales (BOM) y Comparativa de Costos

Esta lista detalla cada componente exacto del diseño del video original de Wirecora, junto con las alternativas optimizadas de mejor rendimiento por el mismo presupuesto.

---

## 1. Lista de Componentes Exactos (Diseño Original del Video)

| Componente | Referencia / Modelo | Función | Precio Est. (USD) |
| :--- | :--- | :--- | :--- |
| **Microcontrolador** | Arduino Nano V3.0 (ATmega328P, con conector USB Mini o Type-C) | Cerebro y lógica de control | \$3.00 - \$4.50 |
| **Sensor de Humedad** | Sensor Capacitivo de Humedad del Suelo v1.2 (3 pines: VCC, GND, AOUT) | Medición sin corrosión en la tierra | \$1.20 - \$2.00 |
| **Pantalla** | Pantalla OLED 0.96" I2C monocromática (SSD1306, 128x64 píxeles) | Visualización de porcentaje y estado | \$2.50 - \$3.50 |
| **Conmutador/Driver** | Transistor Darlington TIP120 (TO-220) | Encendido de la microbomba | \$0.40 - \$0.70 |
| **Resistencias** | 1x 1 kΩ (1/4W) y 1x 10 kΩ (1/4W) | Resistencia de base y pull-down | \$0.10 |
| **Diodo Flyback** | 1N4007 (o 1N4148 / 1N5819 Schottky) | Protección contra picos inductivos del motor | \$0.10 |
| **Actuador de Riego** | Microbomba de agua sumergible 5V DC (3V-6V horizontal o vertical) | Bombeo de agua al tiesto | \$1.50 - \$2.50 |
| **Manguera** | Tubo transparente de vinilo/silicona 6mm/7mm ID (1 metro) | Conducción del agua | \$0.80 - \$1.20 |
| **Alimentación** | Fuente o cargador USB 5V 2A + cable USB a pin 5V/GND | Suministro de potencia estable | \$3.00 - \$5.00 |
| **Varios** | Protoboard pequeña o placa perforada + cables Dupont | Interconexión de componentes | \$1.50 - \$2.50 |
| **TOTAL APROXIMADO** | | | **\$14.10 - \$22.10 USD** |

---

## 2. Lista de Componentes Optimizados (Versión Recomendada "Mismo Precio, Mejor Sistema")

Manteniendo el mismo rango de presupuesto (\$15 a \$20 USD / ~\$60,000 a \$80,000 COP), estas sustituciones eliminan las debilidades del circuito original:

| Componente | Opción Recomendada | Ventaja frente al Original | Referencia / Proveedor Local (Colombia) | Precio Est. (COP / USD) |
| :--- | :--- | :--- | :--- | :--- |
| **Driver de Bomba** | **IRLZ44N** o **FQP30N06L** (MOSFET N-Channel Logic-Level) | Se satura y abre al 100% con 3.3V y 5V. Cero caída de tensión, no calienta nada. | Tiendas de robótica / electrónica local | \$2,500 - \$4,000 COP (~$0.70 - $1.00 USD) |
| **Sensor de Flotador (Opción 1: Horizontal)** | **Sensor Horizontal de Nivel de Agua Plástico** (CS-CO058) | Ideal para perforar pared lateral de balde/tanque; incluye empaque tórico y rosca antifugas. | [Electronilab](https://electronilab.co/tienda/sensor-horizontal-de-nivel-de-agua-plastico-flotador-interruptor/) / [Mecabot](https://www.mecabot.com/products/sensor-de-nivel-de-agua-horizontal-cs-co058) | \$11,900 - \$16,900 COP (~$3.00 - $4.00 USD) |
| **Sensor de Flotador (Opción 2: Vertical)** | **Sensor Vertical de Nivel de Agua Plástico** | Ideal para colgar del borde o taladrar la tapa superior del frasco sin tocar los laterales. | [Electronilab](https://electronilab.co/tienda/sensor-vertical-de-nivel-de-agua-plastico-flotador-interruptor/) / [RoosTech](https://roostech.co/sensores/agua-y-liquidos/flotador-interruptor-vertical-de-nivel-de-agua/) | \$12,600 - \$12,900 COP (~$3.00 - $3.20 USD) |
| **Alarma Sonora Suave** | **Zumbador Pasivo Piezoeléctrico (TMB12A05)** + Resistencia 100-220 Ω | Sonido agradable personalizable mediante `tone()` (tono grave y suave). No aturde ni asusta a tu abuela. | Genérico en cualquier tienda de electrónica | \$800 - \$1,500 COP (~$0.20 - $0.35 USD) |
| **Botón de Riego Manual** | **Pulsador Tact Switch 6x6 mm (4 pines)** | Riego instantáneo de 3 segundos al presionar. Usa `INPUT_PULLUP` interno (sin resistencias extra). | Genérico en cualquier tienda de electrónica | \$300 - \$500 COP (~$0.10 USD) |
| **Protección contra Humedad** | **Esmalte de uñas transparente o barniz acrílico** | Sellar los componentes SMD del sensor capacitivo para que dure años. | Disponible en casa | \$0 COP |

---

## 3. Variante Opcional: Cerebro con ESP32 (Monitoreo por Teléfono)

Si prefieres que el sistema envíe notificaciones a tu teléfono o al de tu abuela por Telegram / WhatsApp o una página web local:
- **Placa alternativa:** ESP32 DevKit V1 (30 o 38 pines).
- **Costo:** \$3.80 - \$5.00 USD (prácticamente igual al Arduino Nano).
- **Ventajas:** Conectividad WiFi integrada, servidor web para ver la gráfica de humedad en el móvil, calibración remota vía navegador.

---

## 4. Consejos Críticos al Comprar

> [!WARNING] Cuidado con los Sensores Capacitivos Falsificados
> En el mercado existen clones de bajo costo del "Capacitive Soil Moisture Sensor v1.2" que vienen de fábrica con un regulador defectuoso o sin el temporizador 555 real, arrojando valores analógicos fijos o ruidosos. Asegúrate de adquirir sensores que tengan integrado el chip oscilador (habitualmente marcado como **NE555**, **TLC555** o circuito RC genuino).

> [!TIP] Aislamiento del Sensor
> Aunque el sensor capacitivo no tiene pistas de cobre expuestas en la zona que se entierra, la parte superior donde están soldados los cables y componentes SMD **no es sumergible**. Aplica silicón caliente, laca protectora o tubo termorretráctil con adhesivo en la parte superior del sensor antes de enterrarlo en la maceta.
