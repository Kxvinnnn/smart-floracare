# ADR-001: Selección y Especificación de la Etapa de Conmutación de la Bomba (MOSFET / Módulo de Potencia)

## Estado
Aceptado con Condiciones de Validación Eléctrica

## Contexto
En el diseño de referencia inicial (video de Wirecora), se empleó un transistor Darlington BJT TIP120 para conmutar la alimentación de una microbomba sumergible de 5V DC (modelo JT-DC3W).

La auditoría técnica identificó deficiencias severas en el TIP120:
1. **Caída de Tensión Colector-Emisor ($V_{CE(sat)}$):** El TIP120 presenta una saturación típica entre 0.8V y 1.2V. En un riel de 5.0V, esto reduce la tensión efectiva en bornes de la bomba a ~3.8V–4.2V, disminuyendo drásticamente el caudal hidráulico y la altura de elevación (*head lift*).
2. **Disipación Térmica:** La potencia disipada ($P = V_{CE(sat)} \times I_{load}$) en el transistor genera calentamiento parásito innecesario dentro de un gabinete cerrado.
3. **Consumo de Corriente de Base:** Siendo accionado por corriente, drena entre 15 mA y 20 mA continuos del pin del microcontrolador.

## Auditoría Crítica de Hojas de Datos (Datasheets Oficiales)

Para reemplazar el BJT se evaluaron MOSFETs de nivel lógico, distinguiendo estrictamente entre:
- **Tensión de Umbral ($V_{GS(th)}$):** Comienzo de la conducción con microamperios ($I_D = 250\,\mu\text{A}$). **No garantiza conducción plena ni baja resistencia.**
- **Tensión con $R_{DS(on)}$ Máxima Garantizada:** Especificada por el fabricante en las tablas de características eléctricas bajo límites de temperatura y tolerancia de fabricación.
- **Curvas Típicas de Salida ($I_D$ vs $V_{DS}$):** Promedios de laboratorio que **no constituyen garantía contractual ni cubren desviaciones de lote**.

### 1. IRLZ44N (Vishay Siliconix, Doc #91328)
- $V_{GS(th)}$: $1.0\,\text{V}$ mín, $2.0\,\text{V}$ máx (a $I_D = 250\,\mu\text{A}$).
- $R_{DS(on)}$ garantizado en tabla de especificaciones:
  - $V_{GS} = 10\,\text{V}, I_D = 25\,\text{A} \implies R_{DS(on)} \le 0.028\,\Omega$
  - $V_{GS} = 5.0\,\text{V}, I_D = 25\,\text{A} \implies R_{DS(on)} \le 0.035\,\Omega$
  - $V_{GS} = 4.0\,\text{V}, I_D = 21\,\text{A} \implies R_{DS(on)} \le 0.040\,\Omega$
  - **A $V_{GS} = 3.3\,\text{V}$ o $3.0\,\text{V}$:** **NO existe valor de $R_{DS(on)}$ garantizado en la tabla eléctrica.**
  - Las curvas típicas a $25^\circ\text{C}$ indican capacidad de conducir $> 10\,\text{A}$ a $V_{GS} = 3.0\,\text{V}$, pero bajo tolerancia extrema ($V_{GS(th)} \to 2.0\,\text{V}$), el sobrevoltaje de compuerta $V_{GS} - V_{GS(th)}$ cae a solo $1.0\,\text{V}-1.3\,\text{V}$.

### 2. FQP30N06L (Fairchild / onsemi)
- $V_{GS(th)}$: $1.0\,\text{V}$ mín, $2.5\,\text{V}$ máx.
- $R_{DS(on)}$ garantizado: solo a $V_{GS} = 5.0\,\text{V}$ ($0.045\,\Omega$) y $10\,\text{V}$ ($0.035\,\Omega$).
- **A $V_{GS} = 3.3\,\text{V}$:** En el peor caso ($V_{GS(th)} = 2.5\,\text{V}$), el margen de sobretensión es de apenas $0.8\,\text{V}$. **Se descarta para accionamiento directo desde GPIO de 3.3V.**

## Decisión de Arquitectura y Selección por Plataforma

1. **Plataforma Arduino Nano V3 (Controlador 5.0V nativo):**
   - **Solución Adoptada:** Accionamiento directo con **IRLZ44N**.
   - **Justificación:** El pin digital del ATmega328P entrega $V_{OH} \ge 4.2\,\text{V}$ (típicamente $4.8\,\text{V}-4.9\,\text{V}$ con carga mínima de compuerta). Según la hoja de datos oficial de Vishay Siliconix (Doc #91328), el fabricante garantiza $R_{DS(on)} \le 0.035\,\Omega$ bajo la condición estandarizada de ensayo $V_{GS} = 5.0\,\text{V}, I_D = 25\,\text{A}, T_J = 25^\circ\text{C}$ (y $\le 0.040\,\Omega$ a $V_{GS} = 4.0\,\text{V}$). Para excitaciones entre $4.2\,\text{V}$ y $4.9\,\text{V}$, el límite máximo de catálogo a $25^\circ\text{C}$ queda acotado entre $0.035\,\Omega$ y $0.040\,\Omega$, sujeto a variación por temperatura y tolerancias de lote.
   - Con la bomba en régimen nominal ($I_D \approx 0.35\,\text{A}$ sumergida en agua):
     $$\Delta V_{DS, \text{nominal}} = 0.35\,\text{A} \times 0.035\,\Omega = 0.012\,\text{V} \implies P_{dis, \text{nominal}} = (0.35)^2 \times 0.035 \approx 4.3\,\text{mW}$$
   - Bajo transitorio estimado de arranque o rotor bloqueado ($I_{stall} \approx 0.8\,\text{A}-1.2\,\text{A}$), la disipación calculada ascendería a $\approx 50\,\text{mW}-60\,\text{mW}$. Ambos valores operan muy por debajo del límite de disipación libre del encapsulado TO-220 ($> 1.5\,\text{W}$), manteniendo al componente frío en laboratorio (pendiente de medición instrumental en banco).

2. **Plataforma ESP32 DevKit V1 (Controlador 3.3V nativo):**
   - **Opción Oficial Adoptada:** **Driver No Inversor Bipolar de 2 Etapas (Q1 NPN 2N2222A + Q2 PNP 2N3906 + IRLZ44N) (Decisión de diseño sujeta a validación en banco)**.
     - Diseñado para elevar la tensión de compuerta a nivel óptimo ($\approx 4.85\,\text{V}$ teóricos) preservando la lógica activa en alto (`HIGH = ON`, `LOW = OFF`).
     - Mediante pull-down en base de Q1 ($10\,\text{k}\Omega$), pull-up en base de Q2 ($10\,\text{k}\Omega$) y pull-down en compuerta ($10\,\text{k}\Omega$), tiene por objetivo garantizar que la bomba permanezca estrictamente apagada durante el arranque, reset o estado flotante (High-Z) del microcontrolador.
     - Los valores calculados ($V_{GS} \le 0.1\,\text{V}$ en reset/corte y $V_{GS} \ge 4.5\,\text{V}$ en activo) son predicciones de diseño que constituyen criterios propuestos de aceptación, a medirse directamente entre pines Gate y Source.
   - **Alternativa Comercial Pendiente de Evaluación:** Módulo MOSFET **HW-517 / XY-MOS (Dual AOD4184A)**. Su eventual adopción requiere verificar el circuito esquemático real del módulo, su tensión de umbral/excitación a 3.3V, el comportamiento seguro durante el arranque o reset del ESP32, y la medición en banco de caída $V_{DS} \le 50\,\text{mV}$ bajo la carga de la bomba.
   - **Opciones Descartadas Formalmente:**
     - *Módulos de relé electromecánicos:* Descartados por riesgo fail-unsafe en reset (Low-Trigger) y conmutación errática a 3.3V (High-Trigger).
     - *Accionamiento directo de IRLZ44N a 3.3V:* Descartado al no existir resistencia de conducción garantizada en la hoja de datos oficial de Vishay/Infineon para $V_{GS} \le 3.3\,\text{V}$.
     - *Pre-driver inversor de 1 solo transistor NPN:* Prohibido terminantemente porque invertiría la lógica, encendiendo la bomba cuando el microcontrolador entra en reset.

## Consecuencias y Reglas de Protección
- **Resistencia Pull-Down Obligatoria:** Resistencia de $10\,\text{k}\Omega$ entre Gate y Source (GND) para mantener el transistor sólidamente apagado durante el arranque o reset del microcontrolador.
- **Resistencia Serie de Compuerta:** $1\,\text{k}\Omega$ (o $220\,\Omega$) en serie con el GPIO para limitar los transitorios de carga capacitiva de compuerta ($C_{iss} \approx 1.7\,\text{nF}$).
- **Supresión Inductiva:** Diodo Schottky **1N5819** ($40\,\text{V}, 1\,\text{A}, V_F \le 0.35\,\text{V}$) adoptado como selección provisional conectada en antiparalelo directo en bornes de la bomba, sujeta a confirmación en banco de supresión de sobretensiones. Prohibido terminantemente el uso de 1N4148 por corriente insuficiente.
