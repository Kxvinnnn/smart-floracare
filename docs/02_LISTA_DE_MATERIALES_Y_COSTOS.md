# 🛒 02. Lista de Materiales (BOM) y Desglose Financiero Auditado

Esta lista detalla los componentes requeridos para la construcción del sistema **Smart FloraCare v1.2.1** en sus dos variantes (**Arduino Nano V3** y **ESP32 DevKit V1**), con especificaciones eléctricas verificadas contra hojas de datos oficiales y cotizaciones del mercado electrónico colombiano (COP / USD a TRM de referencia $1\,\text{USD} \approx 4,000\,\text{COP}$).

---

## ⚠️ Aclaración Crítica de Alcance de Actuadores (Resolución H-14)

> [!IMPORTANT]
> **Ausencia de Electroválvula de 12 V:**
> El diseño oficial de **Smart FloraCare v1.2.1** utiliza **exclusivamente una microbomba sumergible de 5 V DC (JT-DC3W)** para el riego de una sola planta en maceta desde un depósito compartido.
> **No existe ni está prevista una electroválvula de 12 V en esta versión**. Todo el sistema eléctrico opera sobre un único riel de alimentación regulada de 5 V DC.

---

## 📊 1. Resumen de Costos y Opciones de Construcción

El proyecto contempla el riego de una **única maceta de interior** desde un **mismo depósito compartido**. Los componentes de control pueden evaluarse como proyectos independientes (Opción A u Opción B) o como una suite de desarrollo dual donde ambas placas quedan armadas y comparten la infraestructura hidráulica:

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

## 🔍 2. Tabla Maestra de Correspondencia Técnica de Componentes (H-09)

La siguiente tabla enumera con precisión cada componente del circuito, distinguiendo entre control, potencia, protección y accesorios, para evitar cualquier ambigüedad en el aprovisionamiento de taller:

| Identificador | Componente | Valor o referencia | Cantidad por unidad | Función | Ubicación documental | Estado |
| :--- | :--- | :--- | :---: | :--- | :--- | :--- |
| **MCU-01** | Microcontrolador | Arduino Nano V3 (ATmega328P 16MHz, 5V) | 1 (Solo Nano) | Unidad de procesamiento local | `docs/01_ARQUITECTURA_DEL_SISTEMA.md` | CONFIRMADO POR CÓDIGO |
| **MCU-02** | Microcontrolador IoT | ESP32 DevKit V1 (ESP-WROOM-32 30 pines, 3.3V) | 1 (Solo ESP32) | Procesamiento local y servidor Web HTTP | `docs/01_ARQUITECTURA_DEL_SISTEMA.md` | CONFIRMADO POR CÓDIGO |
| **DISP-01** | Pantalla OLED | SSD1306 0.96" I2C (128x64, dirección 0x3C) | 1 por placa | Interfaz visual de telemetría y alarmas | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **SENS-01** | Sensor de humedad | Sensor Capacitivo de Suelo v1.2 | 1 por placa | Lectura analógica de humedad libre de electrólisis | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **SW-01** | Sensor de nivel | Boya magnética reed switch CS-CO058 | 1 por placa | Enclavamiento fail-safe por depósito vacío | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR CÓDIGO |
| **BUZ-01** | Transductor acústico | Zumbador piezoeléctrico pasivo 5V/3.3V | 1 por placa | Señalización audible suave de alarmas (850 Hz) | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR CÓDIGO |
| **BTN-01** | Pulsador manual | Tact switch 6x6 mm 4 pines | 1 por placa | Disparo de ciclo manual supervisado (3 s) | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR CÓDIGO |
| **Q1** | Transistor NPN | 2N2222A / PN2222A / P2N2222A (TO-92, selección provisional) | 1 (Solo ESP32) | Traslador de nivel de entrada del pre-driver (pines funcionales E, B, C sujetos a hoja técnica) | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | SELECCIÓN PROVISIONAL (PINOUT PENDIENTE DE DATASHEET) |
| **Q2** | Transistor PNP | 2N3906 (TO-92, selección provisional) | 1 (Solo ESP32) | Conmutador de nivel alto a riel de 5V (pines funcionales E, B, C sujetos a hoja técnica) | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | SELECCIÓN PROVISIONAL (PINOUT PENDIENTE DE DATASHEET) |
| **Q3** | N-MOSFET Logic-Level | Vishay IRLZ44N (TO-220) | 1 por placa | Interruptor de potencia de bajo lado (Low-Side) | `docs/adr/ADR-001-MOSFET-SELECTION.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **D1** | Diodo Schottky | 1N5819 ($40\,\text{V}, 1\,\text{A}, V_F \le 0.45\,\text{V}$) | 1 por placa | Supresión rápida de fuerza contraelectromotriz | `docs/adr/ADR-001-MOSFET-SELECTION.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **R_G_NANO** | Resistencia de compuerta | $1\,\text{k}\Omega$ 1/4W 5% | 1 (Solo Nano) | Limitación de corriente de pico en pin D3 | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **R_PD_NANO**| Resistencia pull-down | $10\,\text{k}\Omega$ 1/4W 5% | 1 (Solo Nano) | Retención de compuerta a GND en boot/reset | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **R1_ESP** | Resistencia base Q1 | $2.2\,\text{k}\Omega$ 1/4W 5% | 1 (Solo ESP32) | Polarización de base de Q1 desde GPIO 18 | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **R2_ESP** | Resistencia colector Q1 | $2.2\,\text{k}\Omega$ 1/4W 5% | 1 (Solo ESP32) | Limitación de corriente de base de Q2 | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **R_PD1_ESP**| Resistencia pull-down | $10\,\text{k}\Omega$ 1/4W 5% | 1 (Solo ESP32) | Retención de base Q1 a GND en boot/reset | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **R_PU_ESP** | Resistencia pull-up | $10\,\text{k}\Omega$ 1/4W 5% | 1 (Solo ESP32) | Retención de base Q2 a +5V en corte | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **R_PD2_ESP**| Resistencia pull-down | $10\,\text{k}\Omega$ 1/4W 5% | 1 (Solo ESP32) | Descarga y retención de compuerta Q3 a GND | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **R_BUZ** | Resistencia zumbador | $220\,\Omega$ 1/4W 5% | 1 por placa | Limitación de corriente y atenuación acústica | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **PMP-01** | Microbomba sumergible | JT-DC3W (3.5V - 5V DC, 200-350 mA) | 1 (Compartida) | Impulsión hidráulica de agua hacia la planta | `docs/01_ARQUITECTURA_DEL_SISTEMA.md` | CONFIRMADO POR DOCUMENTACIÓN |
| **PWR-SRC** | Fuente regulada | 5V DC 2A a 3A estabilizada | 1 (Compartida) | Alimentación unificada de potencia y lógica | `docs/03_DIAGRAMA_Y_CONEXIONES.md` | CONFIRMADO POR DOCUMENTACIÓN |

> [!CAUTION] Alerta Crítica sobre Pinout de Q1 y Q2 en Encapsulado TO-92 (CRIT-01 / MEN-05):
> La selección de los transistores bipolares Q1 (2N2222A) y Q2 (2N3906) es de naturaleza funcional y **provisional en cuanto al orden numérico de pines**.
> Existen variantes de mercado con orden invertido: por ejemplo, la serie `P2N2222A` (onsemi) presenta patillaje **1: Colector, 2: Base, 3: Emisor (C-B-E)**, mientras que la serie `PN2222A` u otros fabricantes utilizan **1: Emisor, 2: Base, 3: Colector (E-B-C)**.
> **Secuencia Obligatoria de Identificación (6 Pasos):** (1) Registrar el marcaje exacto del componente físico; (2) Identificar fabricante y referencia comercial; (3) Consultar la hoja de datos oficial correspondiente al componente y encapsulado exactos; (4) Confirmar la orientación física de Emisor, Base y Colector según dicho fabricante; (5) Usar el multímetro en modo diodo como comprobación complementaria de las uniones PN (sin asumir una desigualdad universal entre $V_{BE}$ y $V_{BC}$ para distinguir Emisor de Colector); y (6) Registrar el resultado antes de montar o energizar el circuito. Los terminales se conectan estrictamente por su función (Emisor a GND para Q1, Emisor a +5V para Q2).

---

## 🔌 3. Grupo A: Electrónica de Control (Desglose Itemizado por Componente)

| ID | Componente | Especificación Técnica Crítica | Cant. Dual | Asignación | Proveedor Sugerido | Precio Unit. (COP) | Subtotal Dual (COP) |
| :--- | :--- | :--- | :---: | :---: | :--- | :---: | :---: |
| **MCU-01** | **Arduino Nano V3** | ATmega328P, 16 MHz, lógica 5.0V, conector Mini-USB / USB-C | 1 | Solo Nano | Mecabot / Electronilab | \$18,000 | \$18,000 |
| **MCU-02** | **ESP32 DevKit V1** | ESP-WROOM-32 (30 pines), Dual-Core, 3.3V, Wi-Fi 802.11 b/g/n | 1 | Solo ESP32 | Electronilab / Dinastía | \$28,000 | \$28,000 |
| **DISP-01** | **Pantalla OLED 0.96"** | 128×64 monocromo, controlador SSD1306, bus I2C (0x3C) | 2 | 1 c/u | Electronilab / RoosTech | \$13,500 | \$27,000 |
| **SENS-01** | **Sensor de Suelo Capacitivo v1.2** | Salida analógica proporcional. **Nano a 5V; ESP32 obligatoriamente a 3.3V** | 2 | 1 c/u | Mecabot / RoosTech | \$6,500 | \$13,000 |
| **PWR-01A**| **Transistor NPN Pre-Driver** | **2N2222A / PN2222A / P2N2222A** (TO-92, selección provisional - pinout variable por fabricante) | 1 | Solo ESP32 | Almacén de electrónica | \$800 | \$800 |
| **PWR-01B**| **Transistor PNP Pre-Driver** | **2N3906** (TO-92, selección provisional - pinout variable por fabricante) | 1 | Solo ESP32 | Almacén de electrónica | \$700 | \$700 |
| **PWR-02** | **MOSFET de Potencia IRLZ44N** | **Vishay Siliconix IRLZ44N** ($R_{DS(on)} \le 0.035\,\Omega$ a $V_{GS}=5.0\,\text{V}$) | 2 | 1 c/u | Almacén de electrónica | \$3,000 | \$6,000 |
| **DIO-01** | **Diodo Flyback Schottky** | **1N5819** ($40\,\text{V}, 1\,\text{A}, V_F \le 0.45\,\text{V}$) en antiparalelo con motor | 2 | 1 c/u | Almacén de electrónica | \$600 | \$1,200 |
| **SW-01** | **Sensor de Flotador de Nivel** | Boya magnética con interruptor reed hermético (CS-CO058 / Horizontal) | 2 | 1 c/u | [Mecabot](https://www.mecabot.com/products/sensor-de-nivel-de-agua-horizontal-cs-co058) / Electronilab | \$12,900 | \$25,800 |
| **BUZ-01** | **Zumbador Pasivo (Buzzer)** | Transductor piezoeléctrico pasivo 3.3V/5V. Tono suave 850 Hz vía PWM | 2 | 1 c/u | Mecabot / RoosTech | \$1,200 | \$2,400 |
| **BTN-01** | **Pulsador Táctil (Push Button)** | Tact switch 6×6 mm de 4 pines para inserción en protoboard | 2 | 1 c/u | Almacén local | \$400 | \$800 |
| **RES-SPEC**| **Resistencias de Precisión/Carbón** | Enumeradas individualmente: $2.2\,\text{k}\Omega$ (x2), $10\,\text{k}\Omega$ (x4), $1\,\text{k}\Omega$ (x1), $220\,\Omega$ (x2), 1/4W 5% | 1 kit | Compartido | Almacén local | \$2,000 | \$2,000 |
| **BRD-01** | **Protoboard 400 Puntos** | Matriz de conexión para prototipado rápido en banco de pruebas | 2 | 1 c/u | Mecabot / RoosTech | \$6,500 | \$13,000 |
| **CAB-01** | **Cables Dupont** | Kit surtido de 40 jumpers (20 cm, M-M y M-H) | 1 kit | Compartido | Electronilab / Mecabot | \$7,000 | \$7,000 |
| **CAB-02** | **Cable Mini-USB (Nano)** | Cable de datos y programación para Arduino Nano | 1 | Solo Nano | Informática / Hogar | \$4,000 | \$4,000 |
| **CAB-03** | **Cable Micro-USB (ESP32)** | Cable de datos y programación para ESP32 | 1 | Solo ESP32 | Informática / Hogar | \$4,000 | \$4,000 |
| **SUBTOTAL GRUPO A (COMBINADO)** | | | | | | | **\$153,700 COP** |

*(Subtotal individual asignable: Nano = \$71,100 COP; ESP32 = \$82,600 COP).*

---

## 💧 4. Grupo B: Hidráulica y Alimentación Compartida (1 Sola Planta)

Estos componentes conforman la planta física de riego e impulsión, compartida entre ambos controladores:

| ID | Componente | Especificación Técnica Verificada | Cant. | Proveedor Sugerido | Precio Est. (COP) |
| :--- | :--- | :--- | :---: | :--- | :---: |
| **PMP-01** | **Microbomba Sumergible DC** | Modelo JT-DC3W (3.5V - 5V DC), caudal 80-120 L/h, nominal 200-350 mA en agua. ($I_{stall}$ estimada en 0.8–1.2 A; prohibido frenar rotor en banco) | 1 | Robótica / Acuarios | \$8,500 |
| **HOS-01** | **Manguera Flexible Transparente** | Grado alimenticio (silicona o PVC atóxico), diámetro interno 6 mm o 7 mm | 1 m | Ferreterías / Acuarios | \$3,000 |
| **PWR-SRC** | **Fuente Regulada 5V DC (2A a 3A)** | Adaptador de pared 5V DC estabilizado con conector Jack DC o terminales | 1 | Electrónica / Hogar | \$16,000 |
| **RES-TNK** | **Depósito de Agua Estable** | Contenedor plástico hermético reciclado con tapa (1.5 a 3 Litros) | 1 | Hogar / Bazar | \$0 |
| **TRP-01** | **Bandeja Plástica Antiderrames** | Bandeja rectangular para protección del banco durante pruebas hidráulicas | 1 | Hogar / Ferretería | \$4,000 |
| **SUBTOTAL GRUPO B** | | | | | **\$31,500 COP** |

---

## 🛠️ 5. Grupo C: Gabinetes, Protecciones y Consumibles de Taller

| ID | Componente | Especificación Técnica | Cant. Dual | Función de Seguridad | Precio Est. (COP) |
| :--- | :--- | :--- | :---: | :--- | :---: |
| **ENC-01** | **Caja Estanca PVC 100×100×50 mm** | Gabinete plástico hermético para alojamiento de placa definitiva | 2 | Protección IP54 contra salpicaduras | \$18,000 (\$9,000 c/u) |
| **INS-01** | **Barniz Aislante / Esmalte de Uñas** | Barniz acrílico o poliuretano de alta rigidez dieléctrica | 1 frasco | Sellado de componentes y pistas superiores del sensor | \$3,500 |
| **SHR-01** | **Tubo Termorretráctil Surtido** | Tramos de 10 mm y 15 mm para empalmes y protección del sensor | 1 m | Aislamiento dieléctrico y refuerzo mecánico | \$2,500 |
| **SUBTOTAL GRUPO C (COMBINADO)** | | | | | **\$24,000 COP** |

*(Subtotal individual para 1 sola variante: 1 caja + barniz + termorretráctil = \$15,000 COP).*

---

## 🔬 6. Instrumentación Requerida para Validación en Banco (ACT-01 a ACT-06)

Estos equipos son obligatorios para ejecutar el protocolo físico de aceptación de ingeniería:

1. **Multímetro Digital (DMM) Calibrado:** Con resolución mínima de $0.1\,\text{mV}$, prueba de diodos y escala de corriente hasta $10\,\text{A}$.
2. **Osciloscopio Digital (DSO):** Con ancho de banda $\ge 20\,\text{MHz}$ y memoria de disparo único (*single-shot*) para capturar el transitorio inductivo de desconexión en Drain.
3. **Termómetro Infrarrojo o Termocupla Tipo K:** Con precisión de $\pm 1^\circ\text{C}$ para registrar $T_{\text{case}}$ del MOSFET y diodo.
4. **Fuente de Alimentación Regulada con Límite de Corriente:** Capaz de suministrar $5.0\,\text{V} \pm 2\%$ con ajuste de protección de sobrecorriente fijado inicialmente en $1.0\,\text{A}$.

---

## ⚠️ 7. Puntos Eléctricos Críticos y Criterios Definitivos de Selección

1. **Selección del Driver de Bomba para ESP32:**
   - **FQP30N06L:** Ha sido **rechazado oficialmente** debido a su umbral de compuerta ($V_{GS(th)} = 1.0\,\text{V}-2.5\,\text{V}$), que en condiciones límite no garantiza conducción a 3.3V.
   - **IRLZ44N Directo:** No posee especificación garantizada de $R_{DS(on)}$ para $V_{GS} \le 3.3\,\text{V}$ en la tabla oficial de Vishay/Infineon.
   - **Pre-Driver NPN Simple Inversor:** **PROHIBIDO** porque encendería la bomba en el arranque cuando el GPIO está en LOW o High-Z.
   - **Opción Oficial Adoptada:** **Driver No Inversor Bipolar de 2 Etapas (NPN 2N2222A + PNP 2N3906 + IRLZ44N) (Decisión de diseño sujeta a validación en banco)**, calculada para conmutar la compuerta a nivel óptimo (~4.85V teóricos) preservando lógica activa en alto (`HIGH = ON`, `LOW = OFF`) y asegurando corte estricto en arranque y reset.
   - **Alternativa Comercial Pendiente de Evaluación:** Módulo MOSFET HW-517 (Dual AOD4184A) sujeto a inspección previa de esquemático interno, tensión de activación a 3.3V, estado en boot/reset y ensayo con carga real de bomba ($V_{DS} \le 50\,\text{mV}$).
   - **Módulos de Relé Electromecánicos: DESCARTADOS OFICIALMENTE.** Los relés Low-Trigger quedan prohibidos por violar el principio fail-safe (arranque accidental durante el reset). Los módulos High-Trigger presentan problemas frecuentes de conmutación marginal con los 3.3V del ESP32 debido a la caída de tensión del optoacoplador interno de 5V, y no garantizan estabilidad transitoria en el encendido.
2. **Alimentación del Sensor Capacitivo de Suelo:**
   - En **Arduino Nano**: Alimentado a 5.0V. Su salida entrega entre ~2.4V (agua) y ~4.8V (aire), operando dentro del rango del ADC de 10 bits.
   - En **ESP32**: **Obligatoriamente alimentado a 3.3V**. Conectarlo a 5V generaría tensiones superiores a los 3.3V tolerados por el `GPIO 34`, dañando de forma permanente el conversor ADC del ESP32.
3. **Diodo Flyback de Conmutación:**
   - Se selecciona un **1N5819 Schottky** ($1\,\text{A}$ continuo, $40\,\text{V}$, $V_F \le 0.45\,\text{V}$) en antiparalelo directo con los terminales del motor. Su capacidad de absorber transitorios inductivos con el consumo nominal (200–350 mA) y picos transitorios de arranque ($I_{stall}$ estimada en 0.8–1.2 A) debe confirmarse en el montaje real con la bomba y fuente definitivas mediante osciloscopio en ACT-04. Queda prohibido bloquear deliberadamente el rotor para forzar mediciones de sobrecorriente.
4. **Distribución de Potencia:**
   - La bomba toma su corriente en paralelo directamente de la fuente externa de 5V (2A–3A). **Bajo ninguna circunstancia debe alimentarse el motor desde los pines 5V o 3V3 de la placa de desarrollo**.
