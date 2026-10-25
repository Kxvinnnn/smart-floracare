# ADR-001: Selección de MOSFET de Nivel Lógico (IRLZ44N / FQP30N06L)

## Estado
Aceptado

## Contexto
En el diseño de referencia original de Wirecora, se utilizó un transistor bipolar de tipo Darlington TIP120 para conmutar la alimentación de una microbomba de agua sumergible de 5V DC.

Durante la revisión de ingeniería se identificaron los siguientes problemas con el TIP120:
1. **Caída de Tensión Colector-Emisor ($V_{CE(sat)}$):** El TIP120 presenta una caída típica entre 0.8V y 1.2V cuando está saturado. En un riel de alimentación de 5V, esto reduce el voltaje efectivo en la bomba a solo ~3.8V, disminuyendo significativamente el caudal hidráulico y la altura de bombeo (head lift).
2. **Disipación Térmica:** La potencia disipada ($P = V_{CE(sat)} \times I_{load}$) en el transistor genera calentamiento innecesario dentro de un gabinete cerrado.
3. **Consumo de Corriente de Base:** Siendo un dispositivo accionado por corriente, requiere un flujo constante de 15 mA a 20 mA proveniente del pin del microcontrolador.

## Decisión
Se reemplaza el transistor BJT Darlington TIP120 por un MOSFET canal N de nivel lógico (*Logic-Level Gate*): **IRLZ44N** (o alternativamente **FQP30N06L**).

## Consecuencias
### Positivas:
- **Tensión de umbral baja ($V_{GS(th)}$):** El IRLZ44N y el FQP30N06L alcanzan una saturación casi completa con solo 3.3V a 5.0V en la compuerta (Gate), lo que los hace 100% compatibles tanto con Arduino Nano (5V) como con ESP32 (3.3V).
- **Resistencia en estado activo ultrabaja ($R_{DS(on)} < 0.03\,\Omega$):** La caída de tensión es despreciable (< 0.05V), entregando prácticamente los 5.0V completos al motor de la bomba.
- **Eficiencia y Confort Térmico:** El MOSFET permanece frío a temperatura ambiente con corrientes de hasta 1A sin necesidad de disipador.
- **Accionamiento por Voltaje:** La compuerta aislada por óxido no drena corriente estática del microcontrolador.

### Negativas / Mitigaciones:
- Requiere una resistencia pull-down de 10 kΩ entre Gate y GND para evitar encendidos erráticos de la bomba por compuerta flotante durante el arranque o reset del microcontrolador.
