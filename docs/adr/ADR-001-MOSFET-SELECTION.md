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

### 2.2 Análisis de Estados de Alimentación, Desacoplo de Señal y Fugas

La interacción entre el dominio de control de 3.3V (ESP32) y la etapa de potencia de 5.0V se analiza formalmente bajo los siguientes estados operativos de alimentación:

1. **Estado 1 (Sistema Completamente Desenergizado):**
   - Rieles de 3.3V y 5.0V a 0V.
   - Como objetivo de diseño, $Q_1$ y $Q_2$ permanecen en reposo referenciados a tierra a través de $R_{pd1} = 10\,\text{k}\Omega$ y $R_{pd2} = 10\,\text{k}\Omega$, proyectando teóricamente $V_{GS} \approx 0\,\text{V}$.

2. **Estado 2 (Régimen Nominal — Ambos Rieles Energizados):**
   - Riel de 3.3V activo y riel de 5.0V activo.
   - En reposo (`GPIO 18` en LOW): Se busca mantener $Q_1$ en corte. La base de $Q_2$ es referenciada a $+5\,\text{V}$ mediante $R_{pu} = 10\,\text{k}\Omega$ para favorecer el corte de $Q_2$. La resistencia $R_{pd2} = 10\,\text{k}\Omega$ establece una vía de descarga pasiva de compuerta hacia GND. El cumplimiento del criterio de diseño $V_{GS} \le 0{,}100\,\text{V}$ constituye un requisito que solo puede confirmarse experimentalmente en ACT-01.1.
   - En activación (`GPIO 18` en HIGH): $Q_1$ conduce, estableciendo una corriente de base en $Q_2$ a través de $R_2 = 2{,}2\,\text{k}\Omega$. Esto satura $Q_2$, con el objetivo de elevar la compuerta a $V_{GS} \ge 4{,}50\,\text{V}$ según el modelo analítico nominal. El valor efectivo alcanzado dependerá de la carga real y las caídas de saturación que se registren en banco en ACT-02.

3. **Estado 3 — ESP32 energizado y riel de 5.0V desenergizado:**
   - Si `GPIO 18` conmuta a HIGH, $Q_1$ conduce y su colector queda próximo al potencial de GND, con una caída determinada por sus condiciones de conducción ($V_{CE1(sat)}$). El emisor de $Q_2$ permanece conectado al riel de 5V, cuyo potencial depende del estado de la fuente y de sus circuitos internos.
   - La existencia de una trayectoria de retorno hacia GND no demuestra por sí sola que la unión emisor-base de $Q_2$ quede polarizada en directa. Su estado de conducción depende de la diferencia instantánea de potencial entre emisor y base, así como de las trayectorias resistivas, capacitivas y semiconductoras presentes.
   - Por tanto, no se establece una corriente universal ni una resistencia serie equivalente fija para esta condición. El comportamiento del circuito debe evaluarse a partir de la topología real y, si se requiere caracterizar este estado, mediante un procedimiento experimental específico y seguro. Esta condición se considera anómala y debe evitarse durante la secuencia operativa normal.

4. **Estado 4A (Riel de 5.0V Energizado, ESP32 Desenergizado con Señal Conectada):**
   - *Condición teórica suspendida preventivamente de ejecución directa.*
   - Si el pin `GPIO 18` permanece cableado a $R_1$ mientras el microcontrolador carece de alimentación en su riel de 3.3V, la resistencia $R_{pd1} = 10\,\text{k}\Omega$ busca mantener la base de $Q_1$ referenciada a GND.
   - En la hoja técnica de onsemi (`PN2222A/D`), la corriente de corte $I_{CEX}$ está especificada como $\le 10\,\text{nA}$ a $T_A = 25^\circ\text{C}$ bajo la condición de base polarizada en inversa ($V_{CE} = 60\,\text{V}, V_{EB(\text{off})} = 3{,}0\,\text{V}$). En el circuito real, con la base conectada pasivamente a tierra y sujeta a variaciones de temperatura y corrientes de fuga superficiales de placa, las corrientes de fuga no están garantizadas contractualmente en dicho valor. Su absorción por $R_{pu}$ para mantener $Q_2$ en corte pasivo constituye una hipótesis de diseño no validada experimentalmente en esta fase, quedando formalmente excluida de la secuencia inicial de ACT-01.1 para proteger las estructuras de protección del SoC.

5. **Estado 4B (Riel de 5.0V Energizado, ESP32 Energizado en Reset):**
   - *Condición física evaluada activamente en ACT-01.1 (Escenario C).*
   - Con ambos rieles energizados ($3{,}3\,\text{V}$ y $+5\,\text{V}$) y el pulsador físico EN presionado hacia GND, el SoC se encuentra en reinicio por hardware con sus pines en alta impedancia (High-Z). La resistencia $R_{pd1} = 10\,\text{k}\Omega$ retiene la base de $Q_1$ en $0\,\text{V}$, manteniendo el driver en corte pasivo, condición que se valida instrumentalmente en el Escenario C de ACT-01.1.

> **Delimitación de la Medición Instrumental:**
> - El protocolo mandatorio **ACT-01.1** se restringe a medir de forma no invasiva la tensión $V_{GS}$ en la compuerta de $Q_3$ con multímetro de alta resolución y verificar visualmente el estado del LED testigo.
> - La estimación de corriente en la interfaz de entrada mediante la caída diferencial en bornes de $R_1$ ($I_{R1} = \Delta V_{R1} / R_1$) se clasifica exclusivamente como una técnica auxiliar exploratoria. Dicha corriente representa el flujo neto por esa resistencia y no equivale directamente a la corriente inyectada en `GPIO 18` debido a la presencia en paralelo de $R_{pd1}$ y posibles fugas del nodo. Su empleo exige evaluar formalmente los efectos de carga y la incertidumbre del instrumento.

## Consecuencias y Reglas de Protección
- **Uso Exclusivo en Conmutación Estática ON/OFF (Sin PWM):** En ESP32 la descarga pasiva de compuerta por $R_{pd2} = 10\,\text{k}\Omega$ arroja una estimación teórica orientativa de apagado de $\approx 68\,\mu\text{s}$ (RC lineal) a $\approx 100\text{--}220\,\mu\text{s}$ (proyectando $Q_g \le 48\,\text{nC}$ a $44\,\text{V}/25\,\text{A}$). Queda prohibido emplear PWM sin rediseñar la descarga de compuerta y caracterizar la forma de onda en `ACT-04`.
- **Resistencia Pull-Down Obligatoria ($10\,\text{k}\Omega$) y Serie en Nano ($1\,\text{k}\Omega$):** Mantiene Gate referenciado a Source en boot/reset y limita el pico capacitivo sobre `D3` en Nano.
- **Supresión Inductiva:** Diodo Schottky **1N5819** ($40\,\text{V}, 1\,\text{A}, V_F \le 0.45\,\text{V}$) en antiparalelo directo en bornes de la bomba, sujeto a verificación con osciloscopio en `ACT-04` ($V_{DS(\text{peak})} \le V_{CC} + 0.60\,\text{V}$). Prohibido el uso de 1N4148.
