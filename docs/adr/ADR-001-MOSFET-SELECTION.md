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
   - **Justificación:** El pin digital del ATmega328P entrega $V_{OH} \ge 4.2\,\text{V}$ (típicamente $4.8\,\text{V}-4.9\,\text{V}$). A este nivel, el $R_{DS(on)} \le 0.035\,\Omega$ está **100% garantizado por hoja de datos oficial**.
   - Con la bomba consumiendo $0.35\,\text{A}$ nominales y $0.8\,\text{A}$ de arranque:
     $$\Delta V = 0.35\,\text{A} \times 0.035\,\Omega = 0.012\,\text{V} \implies P_{dis} = (0.35)^2 \times 0.035 = 4.3\,\text{mW}$$
   - El componente opera totalmente frío sin necesidad de disipador.

2. **Plataforma ESP32 DevKit V1 (Controlador 3.3V nativo):**
   - **Opción Principal Recomendada:** Utilizar un **Módulo MOSFET con Optoacoplador / Pre-Driver (LR7843, AOD4184 o FR120N)** o un **Módulo Relé 5V de 1 canal con transistor excitador**.
     - Estos módulos reciben la señal lógica de 3.3V del GPIO 18 y conmutan la compuerta del transistor de potencia usando los 5.0V de la fuente principal, garantizando saturación completa de fábrica.
   - **Opción Secundaria (Circuito Discreto en Protoboard):** Emplear un pre-driver de bajo costo con un transistor NPN de señal (2N2222, 2N3904 o BC547) para elevar la tensión de compuerta del IRLZ44N de 3.3V a 5.0V.
   - **Opción Alternativa (Accionamiento Directo de IRLZ44N condicionado a Banco):** Si se conecta directamente al GPIO 18, debe someterse al **Protocolo Obligatorio de Aceptación Eléctrica** (medición de $V_{GS} \ge 3.1\,\text{V}$, $V_{DS} \le 0.05\,\text{V}$ y temperatura $< 40^\circ\text{C}$ tras 30s de bombeo).

## Consecuencias y Reglas de Protección
- **Resistencia Pull-Down Obligatoria:** Resistencia de $10\,\text{k}\Omega$ entre Gate y Source (GND) para mantener el transistor sólidamente apagado durante el arranque o reset del microcontrolador.
- **Resistencia Serie de Compuerta:** $1\,\text{k}\Omega$ (o $220\,\Omega$) en serie con el GPIO para limitar los transitorios de carga capacitiva de compuerta ($C_{iss} \approx 1.7\,\text{nF}$).
- **Supresión Inductiva:** Diodo Schottky **1N5819** ($40\,\text{V}, 1\,\text{A}, V_F \approx 0.35\,\text{V}$) conectado en antiparalelo directo en bornes de la bomba. Prohibido terminantemente el uso de 1N4148 por corriente insuficiente.
