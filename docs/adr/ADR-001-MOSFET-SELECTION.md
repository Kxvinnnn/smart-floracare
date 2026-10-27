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

### 1. IRLZ44N (Infineon / International Rectifier, Doc. `PD-91338`)
- $V_{GS(th)}$: $1.0\,\text{V}$ mín, $2.0\,\text{V}$ máx (a $I_D = 250\,\mu\text{A}$).
- $R_{DS(on)}$ garantizado en tabla de especificaciones a $T_J = 25^\circ\text{C}$:
  - $V_{GS} = 10\,\text{V}, I_D = 25\,\text{A} \implies R_{DS(on)} \le 0.022\,\Omega$ ($22\,\text{m}\Omega$)
  - $V_{GS} = 5.0\,\text{V}, I_D = 25\,\text{A} \implies R_{DS(on)} \le 0.025\,\Omega$ ($25\,\text{m}\Omega$)
  - $V_{GS} = 4.0\,\text{V}, I_D = 21\,\text{A} \implies R_{DS(on)} \le 0.035\,\Omega$ ($35\,\text{m}\Omega$)
  - *(Nota de trazabilidad H-01: el documento Vishay Siliconix `#91328` corresponde al `IRLZ44` sin sufijo `N`, cuyos límites tabulados son $0.028\,\Omega$, $0.035\,\Omega$ y $0.040\,\Omega$. La referencia oficial seleccionada es `IRLZ44N` / `IRLZ44NPbF`, `PD-91338`).*
  - **A $V_{GS} = 3.3\,\text{V}$ o $3.0\,\text{V}$:** **NO existe valor de $R_{DS(on)}$ garantizado en la tabla eléctrica.**
  - Las curvas típicas a $25^\circ\text{C}$ indican capacidad de conducir $> 10\,\text{A}$ a $V_{GS} = 3.0\,\text{V}$, pero bajo tolerancia extrema ($V_{GS(th)} \to 2.0\,\text{V}$), el sobrevoltaje de compuerta $V_{GS} - V_{GS(th)}$ cae a solo $1.0\,\text{V}-1.3\,\text{V}$.

### 2. FQP30N06L (Fairchild / onsemi)
- $V_{GS(th)}$: $1.0\,\text{V}$ mín, $2.5\,\text{V}$ máx.
- $R_{DS(on)}$ garantizado: solo a $V_{GS} = 5.0\,\text{V}$ ($0.045\,\Omega$) y $10\,\text{V}$ ($0.035\,\Omega$).
- **A $V_{GS} = 3.3\,\text{V}$:** En el peor caso ($V_{GS(th)} = 2.5\,\text{V}$), el margen de sobretensión es de apenas $0.8\,\text{V}$. **Se descarta para accionamiento directo desde GPIO de 3.3V.**

## Decisión de Arquitectura y Selección por Plataforma

1. **Plataforma Arduino Nano V3 (Controlador 5.0V nativo, pin `D3`):**
   - **Solución Adoptada:** Accionamiento directo con **`IRLZ44N`** (`Infineon PD-91338`).
   - **Justificación y alcance de la garantía:** El pin `D3` del ATmega328P entrega $V_{OH} \ge 4.2\,\text{V}$ (típicamente $4.8\,\text{V}-4.9\,\text{V}$ con carga estática mínima). Según `PD-91338`, a $T_J = 25^\circ\text{C}$ el fabricante garantiza límites únicamente en los puntos discretos $V_{GS} = 10\,\text{V}$ ($\le 0.022\,\Omega$), $V_{GS} = 5.0\,\text{V}, I_D = 25\,\text{A}$ ($\le 0.025\,\Omega$) y $V_{GS} = 4.0\,\text{V}, I_D = 21\,\text{A}$ ($\le 0.035\,\Omega$). Ninguna interpolación entre $4.0\,\text{V}$ y $5.0\,\text{V}$ constituye garantía contractual; para $4.0\,\text{V} \le V_{GS} < 5.0\,\text{V}$ el único límite discreto tabulado aplicable a $25^\circ\text{C}$ es $\le 0.035\,\Omega$ (a $4.0\,\text{V}$), y la aceptación práctica del montaje depende de las mediciones en `ACT-02` y `ACT-03.1`.
   - Con la bomba en régimen estacionario ($I_D \approx 0.35\,\text{A}$ sumergida en agua) y evaluando incluso el escalón conservador de $0.035\,\Omega$:
     $$\Delta V_{DS, \text{nominal}} = 0.35\,\text{A} \times 0.035\,\Omega = 0.012\,\text{V} \implies P_{dis, \text{nominal}} = (0.35)^2 \times 0.035 \approx 4.3\,\text{mW}$$
   - Bajo transitorio estimado de arranque ($I_{\text{arranque}} \approx 0.8\,\text{A}-1.2\,\text{A}$), la disipación instantánea calculada se mantiene $\le 60\,\text{mW}$ (pendiente de verificación instrumental en `ACT-03.1` y `ACT-03.2`).

2. **Plataforma ESP32 DevKit V1 (Controlador 3.3V nativo, pin `GPIO 18`):**
   - **Opción Oficial Adoptada:** **Driver No Inversor Bipolar de 2 Etapas (`Q1` NPN `2N2222A`, ref. `onsemi PN2222A/D` + `Q2` PNP `2N3906`, ref. `onsemi 2N3906/D` + `IRLZ44N`) — Decisión de diseño sujeta a validación en banco**.
     - **Modelo analítico condicional:** Con resistencias $\pm 5\%$ y $V_{CC} \in [4.75\,\text{V}; 5.25\,\text{V}]$, la relación de corriente forzada en peor caso es $\beta_{f,Q1,\text{máx}} = \frac{2.177\,\text{mA}}{0.713\,\text{mA}} = 3.05 < 10$ ($2.19$ a $4.75\,\text{V}$) y $\beta_{f,Q2,\text{máx}} = \frac{0.547\,\text{mA}}{1.469\,\text{mA}} = 0.372 \ll 10$ ($0.337$ a $4.75\,\text{V}$). Dado que en `PN2222A/D` y `2N3906/D` los límites máximos tabulados de saturación ($0.30\,\text{V}$ y $0.25\,\text{V}$) están especificados a corrientes mayores ($150\,\text{mA}$ y $-10\,\text{mA}$), tanto $V_{CE1(sat)} \le 0.30\,\text{V}$ como $|V_{EC2(sat)}| = 0.25\,\text{V}$ se tratan como **hipótesis conservadoras del modelo**, de las cuales resultan **$V_{GS,\text{nom}} \approx 4.90\,\text{V}$** y **$V_{GS,\text{mín(modelo)}} = 4.50\,\text{V}$**. Se distingue estrictamente entre: (1) *resultado matemático del modelo* ($4.90\,\text{V}$ nom. / $4.50\,\text{V}$ peor caso condicional), (2) *límite garantizado por fabricante* (exclusivo para las condiciones tabuladas en `PN2222A/D`, `2N3906/D` y `PD-91338`) y (3) *resultado experimental* medido en banco (`ACT-02` y `ACT-03.1`).
     - Mediante $R_{pd1} = 10\,\text{k}\Omega$, $R_{pu} = 10\,\text{k}\Omega$ y $R_{pd2} = 10\,\text{k}\Omega$, el diseño preserva lógica activa en alto (`GPIO 18`: `HIGH = ON`, `LOW = OFF`) y retención pasiva en arranque/reset, cuya validación completa exige tanto `ACT-01.1` (DMM) como la captura temporal `ACT-01.2` (osciloscopio).
   - **Alternativa Comercial Provisional No Verificada (`HW-517` / `XY-MOS`):** No autorizada para compra ni montaje directo; cualquier uso eventual exige levantar antes el esquemático del módulo físico recibido, verificar polaridad no inversora en boot/reset y medir $V_{DS} \le 50\,\text{mV}$ bajo carga real.
   - **Opciones Descartadas Formalmente:**
     - *Módulos de relé electromecánicos:* Descartados por riesgo fail-unsafe en reset (Low-Trigger) y conmutación errática a 3.3V (High-Trigger).
     - *Accionamiento directo de IRLZ44N a 3.3V:* Descartado al no existir $R_{DS(on)}$ garantizada en `PD-91338` para $V_{GS} \le 3.3\,\text{V}$.
     - *Pre-driver inversor de 1 solo transistor NPN:* Prohibido terminantemente porque invertiría la lógica, encendiendo la bomba cuando el microcontrolador entra en reset.

## Consecuencias y Reglas de Protección
- **Uso Exclusivo en Conmutación Estática ON/OFF (Sin PWM):** En ESP32 la descarga pasiva de compuerta por $R_{pd2} = 10\,\text{k}\Omega$ arroja una estimación teórica orientativa de apagado de $\approx 68\,\mu\text{s}$ (RC lineal) a $\approx 100\text{--}220\,\mu\text{s}$ (proyectando $Q_g \le 48\,\text{nC}$ a $44\,\text{V}/25\,\text{A}$). Queda prohibido emplear PWM sin rediseñar la descarga de compuerta y caracterizar la forma de onda en `ACT-04`.
- **Resistencia Pull-Down Obligatoria ($10\,\text{k}\Omega$) y Serie en Nano ($1\,\text{k}\Omega$):** Mantiene Gate referenciado a Source en boot/reset y limita el pico capacitivo sobre `D3` en Nano.
- **Supresión Inductiva:** Diodo Schottky **1N5819** ($40\,\text{V}, 1\,\text{A}, V_F \le 0.45\,\text{V}$) en antiparalelo directo en bornes de la bomba, sujeto a verificación con osciloscopio en `ACT-04` ($V_{DS(\text{peak})} \le V_{CC} + 0.60\,\text{V}$). Prohibido el uso de 1N4148.
