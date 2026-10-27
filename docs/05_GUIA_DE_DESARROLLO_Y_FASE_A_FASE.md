# 🛠️ 05. Guía de Desarrollo, Montaje y Comisionamiento Fase a Fase
## Protocolo Oficial de Validación Instrumental en Banco (ACT-01 a ACT-06)

Esta guía describe el procedimiento riguroso para construir, probar en frío, verificar eléctricamente, comisionar y desplegar el sistema de autorriego **Smart FloraCare v1.2.1** en sus dos variantes de hardware (**Arduino Nano V3** y **ESP32 DevKit V1**).

---

## 📋 Resumen del Procedimiento de Puesta en Marcha

El protocolo de comisionamiento sigue 6 fases secuenciales e independientes. **No se debe avanzar a la siguiente fase sin haber completado y registrado como APROBADA la anterior:**

```text
+-------------------+      +--------------------+      +--------------------+
|  FASE 1: BANCO    | ---> |  FASE 2: CHEQUEOS  | ---> |  FASE 3: ENERGIZADO|
|  Pre-montaje y    |      |  EN FRÍO (Cold)    |      |  PROGRESIVO        |
|  acondicionamiento|      |  Continuidad/Aisla |      |  Rieles y MCU      |
+-------------------+      +--------------------+      +--------------------+
                                                              │
                                                              ▼
+-------------------+      +--------------------+      +--------------------+
|  FASE 6: DESPLIEGUE| <--- |  FASE 5: PRUEBAS   | <--- |  FASE 4: HIDRÁULICA|
|  EN MACETA        |      |  DE ESTRÉS Y FALLO |      |  Y ANTI-SIFÓN      |
|  Aceptación final |      |  FSM e Interlocks  |      |  Air Gap / Caudal  |
+-------------------+      +--------------------+      +--------------------+
```

---

## 📌 Fase 1: Preparación y Acondicionamiento de Componentes

### 1.1 Sellado e Impermeabilización del Sensor Capacitivo
Los sensores capacitivos v1.2 poseen componentes electrónicos expuestos en el tercio superior de la placa que sufren deterioro si entran en contacto con salpicaduras de agua o vapor de sustrato continuo:
1. Identifica la línea serigrafiada blanca en el cuerpo de la placa. La inserción en el sustrato **nunca debe superar dicha línea**.
2. Aplica 2 capas uniformes de barniz protector acrílico (o esmalte de uñas transparente de poliuretano) sobre el circuito integrado NE555 / TLC555, resistencias SMD y pistas de la cabeza del sensor. Deja secar 30 minutos entre capas.
3. Coloca un tramo de tubo termorretráctil de 15 a 20 mm de diámetro (preferiblemente con adhesivo termofusible interno) cubriendo el cabezal del conector y los primeros centímetros del cableado. Aplica calor suave con pistola térmica o mechero.

**Criterio de Aceptación Fase 1:** El sensor presenta recubrimiento hermético en la electrónica superior sin residuos sobre la superficie sensora inferior.

---

## 📌 Fase 2: Montaje y Chequeos en Frío (Cold Checks)

Antes de suministrar energía eléctrica a la protoboard, realiza los siguientes ensayos con un Multímetro Digital (DMM) en modo continuidad (bip) y resistencia ($\Omega$). **Auditoría óhmica previa fuera del circuito ($\pm 5\%$):** medir cada resistor ($2.2\,\text{k}\Omega \in [2.09; 2.31]\,\text{k}\Omega$; $10\,\text{k}\Omega \in [9.50; 10.50]\,\text{k}\Omega$; $1\,\text{k}\Omega \in [0.95; 1.05]\,\text{k}\Omega$; $220\,\Omega \in [209; 231]\,\Omega$; carga ficticia $15\,\Omega \in [14.25; 15.75]\,\Omega$), verificar ausencia de cortocircuito entre `+5V`, `+3.3V` y `GND`, y confirmar orientación del cátodo del `1N5819` hacia `+5V`.

### 2.1 Conexiones de Circuito por Variante

#### Variante A: Arduino Nano V3 (5V nativo)
- `5V` y `GND` de fuente van al riel de potencia de la protoboard y a los pines `5V` y `GND` del Nano.
- Sensor Capacitivo: `VCC` a `5V`, `GND` a riel común, `AOUT` al pin analógico `A0`.
- Display OLED (0.96" SSD1306): `VCC` a `5V`, `GND` a riel, `SDA` a `A4`, `SCL` a `A5`.
- Driver MOSFET (IRLZ44N - Logic Level):
  - Gate (Pin 1): Resistencia en serie de 1 kΩ hacia pin digital `D3`. Resistencia pull-down de 10 kΩ entre Gate y GND.
  - Drain (Pin 2): Conectado al polo negativo (`-`) del conector de la bomba.
  - Source (Pin 3): Conectado sólidamente al riel común de tierra (`GND`).
- Diodo Flyback (1N5819 Schottky): Cátodo (franja plateada) al polo positivo `+5V`, ánodo al polo negativo de la bomba (Drain).
- Flotador de Nivel: Pin digital `D4` a un terminal del interruptor; el otro terminal a `GND` (usa `INPUT_PULLUP`).
- Pulsador Manual: Pin digital `D2` configurado a `GND` con `INPUT_PULLUP` interno.
- Buzzer Pasivo: Pin digital `D5` con resistencia de 220 Ω en serie hacia un polo del buzzer; otro polo a `GND`.

#### Variante B: ESP32 DevKit V1 (3.3V lógica)
- **Alimentación y Masa Común:** Fuente externa regulada de `5V` (2A–3A) conectada al pin `VIN`/`5V` del ESP32 y al polo positivo (`+`) del motor. Masa común (`GND`) sólidamente unificada entre fuente, ESP32 y terminales de tierra de los drivers.
- **Sensor Capacitivo v1.2:** **`VCC` conectado obligatoriamente al riel de `3.3V` del ESP32** (nunca a 5V para evitar sobretensión en el conversor analógico). Salida analógica conectada a `GPIO 34` (ADC1_CH6). `GND` al riel común.
- **Display OLED SSD1306:** `VCC` a `3.3V`, `GND` a tierra, `SDA` a `GPIO 21`, `SCL` a `GPIO 22`.
- **Driver de Bomba: Etapa Bipolar No Inversora NPN+PNP (2 Etapas) (Decisión Oficial de Diseño):**
  - **Q1 (Transistor NPN 2N2222A - Conmutador de Nivel):**
    - Base: Conectada a `GPIO 18` vía $R_1 = 2.2\,\text{k}\Omega$. Resistencia pull-down $R_{pd1} = 10\,\text{k}\Omega$ entre Base y `GND`.
    - Emisor: Conectado a `GND`.
    - Colector: Conectado a la Base de Q2 a través de $R_2 = 2.2\,\text{k}\Omega$.
  - **Q2 (Transistor PNP 2N3906 - Conmutador de Potencia de Compuerta):**
    - Emisor: Conectado directamente al riel de potencia de `+5V`.
    - Base: Resistencia de pull-up $R_{pu} = 10\,\text{k}\Omega$ entre Base y riel `+5V`. Conectada al Colector de Q1 mediante $R_2 = 2.2\,\text{k}\Omega$.
    - Colector: Conectado a la Compuerta (Gate) del MOSFET IRLZ44N.
  - **Q3 (MOSFET de Potencia N-Channel IRLZ44N):**
    - Gate (Pin 1): Conectada al Colector de Q2. Resistencia pull-down $R_{pd2} = 10\,\text{k}\Omega$ entre Gate y `GND`.
    - Drain (Pin 2): Conectado al polo negativo (`-`) de la microbomba JT-DC3W.
    - Source (Pin 3): Conectado sólidamente a `GND` común.
  - **Alternativa Comercial Provisional No Verificada (`HW-517`):** Prohibido adquirir o montar como reemplazo directo sin levantar antes su esquemático real, verificar polaridad no inversora en boot/reset y medir $V_{DS} \le 50\,\text{mV}$ bajo carga real.

---

## 🔬 Protocolo Oficial de Pruebas Físicas de Banco (ACT-01 a ACT-06)

> [!WARNING]
> **Prerrequisito Metrológico y Estado Real:** Ninguna de estas pruebas ha sido ejecutada todavía en hardware. Antes de registrar cualquier medición es obligatorio anotar en la bitácora: **marca, modelo, rango, resolución, exactitud especificada por el fabricante en dicho rango y estado de verificación/calibración** de cada instrumento, junto con la corriente de carga y los puntos exactos de contacto. Las pruebas que exigen osciloscopio (`ACT-01.2`, `ACT-03.2`, `ACT-04`, `ACT-05.3b`) permanecen en `PENDIENTE DE INSTRUMENTACIÓN EXTERNA`.

---

### ACT-01: Estado Seguro del Driver en Reposo, Encendido, Reinicio y Recuperación (`ACT-01.1` y `ACT-01.2`)
1. **Identificador:** `ACT-01` (dividida en comprobación preliminar **`ACT-01.1`** con DMM+LED y captura temporal obligatoria **`ACT-01.2`** con osciloscopio).
2. **Objetivo:** Verificar cuantitativamente que la etapa de potencia y la compuerta del MOSFET permanecen en corte estricto ($V_{GS} < 0.10\,\text{V}$) durante el arranque inicial, el reinicio físico, la desconexión del cable de señal y el estado de alta impedancia (High-Z) del microcontrolador.
3. **Riesgo cubierto:** F-07 (Inversión lógica, compuerta flotante o activación espuria de bomba durante transitorios de encendido).
4. **Equipo e instrumentos:** Multímetro Digital (DMM) calibrado (resolución $\le 1\,\text{mV}$) y osciloscopio digital (acoplamiento DC, base de tiempo 200 ms/div, trigger en flanco de subida de riel 5V).
5. **Montaje:** Driver NPN+PNP montado en protoboard con Q1 (`2N2222A`), Q2 (`2N3906`), Q3 (`IRLZ44N`) y resistencias ($R_1 = 2.2\,\text{k}\Omega$, $R_{pd1} = 10\,\text{k}\Omega$, $R_2 = 2.2\,\text{k}\Omega$, $R_{pu} = 10\,\text{k}\Omega$, $R_{pd2} = 10\,\text{k}\Omega$; en Nano, $R_G = 1\,\text{k}\Omega$ y $R_{pd} = 10\,\text{k}\Omega$ desde `D3`). Carga simulada con LED indicador + resistencia de $330\,\Omega$ entre `+5V` y `Drain` (`ACT-01.1`) y sondas 10X DC en `Pin 1 (Gate)` y `+5V`/`GPIO 18`/`D3` con masa corta en `Pin 3 (Source)` (`ACT-01.2`). Bomba mecánica desconectada.
6. **Precondiciones:** Fuente externa de 5.0 V regulada y estabilizada. Microcontrolador programado con firmware v1.2.1.
7. **Procedimiento:**
   - Medir $V_{GS}$ con multímetro entre Pin 1 (Gate) y Pin 3 (Source) del IRLZ44N en 4 escenarios:
     a) Fuente de 5V encendida, ESP32 apagado / desconectado.
     b) ESP32 energizado, durante los primeros 500 ms de bootloader.
     c) Pulsando y manteniendo presionado el botón físico EN/RESET del ESP32.
     d) Cable de señal desconectado de GPIO 18 (entrada flotante al driver).
8. **Mediciones a registrar:** $V_{B(Q1)}$, $V_{B(Q2)}$, $V_{Gate(Q3)}$, corriente de fuga $I_{leak}$ en Drain.
9. **Criterios de aceptación cuantificables (`ACT-01.1` preliminar vs. `ACT-01.2` captura temporal):**
   - **`ACT-01.1` (Comprobación preliminar con DMM + LED testigo $330\,\Omega$):** $V_{GS} \le 0.10\,\text{V}$ en los 4 escenarios y LED apagado. *Alcance:* comprobación complementaria; no descarta pulsos sub-milisegundo y **no cierra `ACT-01` por sí sola**.
   - **`ACT-01.2` (Protocolo reproducible en 2 modos con osciloscopio $\ge 20\,\text{MHz}$, 2CH, sondas 10X DC con masa corta de ambos canales en `Pin 3 Source`):** Para no confundir una ausencia de disparo con una forma de onda medida, es obligatorio ejecutar dos modos complementarios: (1) **Modo 1 — Captura determinista disparada por el evento (`Event-Triggered`, 11 oscilogramas obligatorios):** **CH1** en `Pin 1 (Gate)` ($200\,\text{mV/div}$ DC). En **encendido en frío (`ACT-01.2-A`, 1 captura)**, **CH2** en `+5V` ($1\,\text{V/div}$ DC), base **$50\,\text{ms/div}$** (ventana de $500\,\text{ms}$, $10\%$ pre-trigger), disparo *Single-Shot* ascendente en **CH2 a $+1.50\,\text{V}$**. En **reinicio y recuperación (`ACT-01.2-B`, 5 ciclos = 10 capturas)**, **CH2** en `RESET` (Nano) / `EN` (ESP32), rearmando el botón `Single` antes de cada acción para registrar **5 capturas en flanco de bajada de CH2 a $+1.50\,\text{V}$** (**$20\,\text{ms/div}$**, ventana $200\,\text{ms}$) y **5 capturas en flanco de subida de CH2 a $+1.50\,\text{V}$** (**$50\,\text{ms/div}$**, ventana $500\,\text{ms}$ hasta `setup()`). En las 11 formas de onda registradas debe medirse con cursor **$V_{GS(\text{peak})} \le 0.50\,\text{V}$** (y siempre $< 0.80\,\text{V}$). (2) **Modo 2 — Vigilancia rápida de glitches por disparo en compuerta (`Glitch-Trap`, base $20\,\mu\text{s/div}$, ventana $200\,\mu\text{s}$):** **CH1** en `Pin 1 (Gate)` ($200\,\text{mV/div}$ DC), **CH2** en `GPIO 18`/`D3`, disparo *Single-Shot* ascendente en **CH1 a $+0.50\,\text{V}$**. Se rearma `Single` (verificando estado `ARMED`/`WAIT`) antes de 1 encendido en frío (observado $5\,\text{s}$) y antes de cada uno de los 5 ciclos completos de `RESET`/`EN` ($2\,\text{s}$ presionado + $5\,\text{s}$ liberado). Si al concluir la ventana el equipo permanece en `ARMED`/`WAIT`, se registra en bitácora como `0 disparos >= +0.50 V` (con foto del estado `WAIT`); cualquier disparo $\ge +0.50\,\text{V}$ impide aprobar hasta su análisis y cualquier pico $\ge 0.80\,\text{V}$ reprueba `ACT-01.2`.
10. **Resultado esperado:** $V_{GS} = 0.00\,\text{V}$, corte absoluto garantizado por $R_{pd1}$, $R_{pu}$ y $R_{pd2}$.
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Captura oscilográfica del transitorio de boot y fotografía de pantalla del DMM (Pendiente).
13. **Estado:** `ACT-01.1` `PENDIENTE DE MEDICIÓN EN BANCO` · `ACT-01.2` `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` (ambas requeridas para cerrar `ACT-01`).
14. **Observaciones y acciones correctivas:** Si se detecta $V_{GS} > 0.20\,\text{V}$ en reset, verificar que $R_{pd1}$ y $R_{pu}$ no presenten soldadura fría o valores alterados.

---

### ACT-02: Compatibilidad de Niveles Lógicos y Excitación de Compuerta
1. **Identificador:** `ACT-02`
2. **Objetivo:** Comprobar que el circuito de adaptación de 2 etapas NPN+PNP entrega una tensión de compuerta plena ($V_{GS} \ge 4.50\,\text{V}$) al ser activado desde el GPIO de 3.3 V del ESP32, asegurando saturación profunda del MOSFET IRLZ44N dentro de especificación de catálogo ($R_{DS(on)} \le 0.035\,\Omega$).
3. **Riesgo cubierto:** Conducción parcial del MOSFET a 3.3V, sobrecalentamiento por resistencia de canal elevada y fallo de par de arranque de la bomba.
4. **Equipo y requisitos metrológicos:** Multímetro Digital (DMM) con escala mV DC (resolución $\le 0.1\,\text{mV}$; **obligatorio registrar exactitud especificada del fabricante en ese rango**, corriente de carga $I_D$ y contacto directo sobre patillas TO-220), amperímetro DC ($\ge 2\,\text{A}$) y termocupla tipo K o termómetro IR (con cinta negra mate $\varepsilon \approx 0.95$).
5. **Montaje, Trayectoria Eléctrica y Seguridad Térmica (MEN-02 / MEN-04):**
   - **Pre-driver:** Conectar Base de Q1 a GPIO 18 vía $R_1 = 2.2\,\text{k}\Omega$ (con $R_{pd1} = 10\,\text{k}\Omega$ a GND); Emisor de Q1 a GND; Colector de Q1 a Base de Q2 vía $R_2 = 2.2\,\text{k}\Omega$.
   - **Etapa PNP:** Emisor de Q2 al riel de $+5.0\,\text{V}$ estabilizado; Base de Q2 polarizada con $R_{pu} = 10\,\text{k}\Omega$ a $+5.0\,\text{V}$; Colector de Q2 a Gate de Q3 (con $R_{pd2} = 10\,\text{k}\Omega$ a GND).
   - **Carga de Prueba de Bajo Lado (Low-Side Test Load):**
     * Conectar un extremo de la resistencia de potencia de $15\,\Omega$, $5\,\text{W}$ directamente al riel de $+5.0\,\text{V}$ de la fuente externa.
     * Conectar el segundo extremo de la resistencia al terminal Drenador (Drain, pin 2) del MOSFET Q3 (Nodo N6).
     * Conectar el terminal Fuente (Source, pin 3) del MOSFET Q3 directamente a la masa común (GND).
     * *Disipación y montaje no inflamable:* A $5.0\,\text{V}$, $I_D \approx 333\,\text{mA}$ y $P \approx 1.67\,\text{W}$ (hasta $368\,\text{mA} / 1.93\,\text{W}$ a $5.25\,\text{V}$ con $14.25\,\Omega$). La resistencia cerámica de $15\,\Omega, 5\,\text{W}$ debe apoyarse elevada sobre **superficie cerámica/metálica no inflamable**, lejos de la protoboard.
   - **Instrumentación Térmica, Régimen de Pulso y Criterio de Parada Sin Contacto (MEN-04):**
     * **Prohibición de contacto manual:** Queda estrictamente prohibido tocar la resistencia de potencia o el MOSFET mientras el circuito esté energizado o inmediatamente después del pulso.
     * **Medición de temperatura:** Fijar mecánicamente una sonda termocupla tipo K sobre el cuerpo cerámico central de la resistencia de $15\,\Omega$ (o apuntar un termómetro IR a $5\text{--}10\,\text{cm}$ de su superficie).
     * **Secuencia con firmware de producción intacto (sin conflicto con límites de 20s/25s):** Se usa el binario de producción (`nanoatmega328`/`esp32dev`) disparando pulsos manuales supervisados de **$3.0\,\text{s}$** (`manualBurstDurationMs = 3000` ms vía botón `D2`/`GPIO 4`, un pulso por nodo) con pausas de enfriamiento $\ge 30\,\text{s}$ (superiores a `cooldownPeriodMs = 5000` ms), sin alcanzar ni modificar `maxAutoDurationMs` ($20.0\,\text{s}$) ni `maxSafetyWatchdogMs` ($25.0\,\text{s}$).
     * **Verificación de corriente:** Intercalar amperímetro en serie (escala 10 A o 2 A DC) entre la resistencia y el Drain, o medir tensión en bornes de la resistencia ($I = V_R / 15\,\Omega$).
     * **Tres Zonas Cuantitativas para $V_{DS}$ en patillas TO-220 (sin zona gris):** (1) **Aceptación (`APROBADO`):** $V_{DS} \le 15.0\,\text{mV}$ (con $V_{GS} \ge 4.50\,\text{V}$ e $I_D \in [290; 380]\,\text{mA}$); (2) **Zona Intermedia de Incumplimiento (`15.0 mV < VDS <= 30.0 mV` — `NO APROBADO`):** incumple aceptación, rechaza `ACT-02` y bloquea `ACT-03`; (3) **Parada de Emergencia (`ABORTAR`):** cortar fuente si **$V_{DS} > 30.0\,\text{mV}$**, $V_{GS} < 4.50\,\text{V}$, $T_{\text{MOSFET,IR}} \ge 45.0\,^\circ\text{C}$ *(umbral prudencial de aborto en taller, no equivalente metrológicamente a $60^\circ\text{C}$ por termopar)* o $T_{\text{res}} > 60.0\,^\circ\text{C}$. *(Nota: $V_{DS} \le 15.0\,\text{mV}$ es un criterio experimental de aceptación del proyecto para el montaje físico —que incluye la resistencia de contacto en patillas TO-220—, no un valor garantizado directamente por la hoja técnica para toda condición de operación).*
6. **Precondiciones:** `ACT-01.1` superada. Firmware de producción cargado (`manualBurstDurationMs = 3000` ms).
7. **Procedimiento:**
   - Activar el riego manual mediante pulsador.
   - Medir durante el intervalo de 3 s la tensión en GPIO 18 ($V_{OH}$), la tensión de base de Q1 ($V_{BE1}$), la tensión colector-emisor de Q1 ($V_{CE1}$), la tensión base-emisor de Q2 ($V_{EB2}$) y la tensión efectiva Gate-Source en Q3 ($V_{GS}$).
8. **Mediciones a registrar:** $V_{GPIO}$, $V_{BE1}$, $V_{CE1}$, $V_{EB2}$, $V_{EC2}$, $V_{GS}$.
9. **Criterios de aceptación cuantificables:** $V_{OH(\text{GPIO 18})} \ge 3.00\,\text{V}$, $V_{CE1} \le 0.20\,\text{V}$, $|V_{EC2}| \le 0.25\,\text{V}$, **$V_{GS} \ge 4.50\,\text{V}$** y **$V_{DS} \le 15.0\,\text{mV}$** en pines `Pin 2`/`Pin 3` del TO-220 a $I_D \in [290; 380]\,\text{mA}$ (toda lectura $V_{DS} > 15.0\,\text{mV}$ incumple aceptación).
10. **Resultado esperado:** $V_{GS} \approx 4.85\,\text{V}$, excitación plena del canal N del IRLZ44N.
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Registro numérico de tabla nodal firmado (Pendiente).
13. **Estado:** `PENDIENTE DE MEDICIÓN EN BANCO`.
14. **Observaciones y acciones correctivas:** Si $V_{GS} < 4.50\,\text{V}$, verificar la caída de tensión en el riel de 5V de la fuente bajo carga. Si la fuente cae por debajo de $4.8\,\text{V}$, sustituir la fuente por una unidad de 5V 3A de mayor regulación.

---

### ACT-03: Conmutación con Bomba Real — Régimen Estacionario (`ACT-03.1`) vs. Arranque Dinámico (`ACT-03.2`)
1. **Identificador:** `ACT-03` (dividida en **`ACT-03.1`** estacionaria con DMM y **`ACT-03.2`** dinámica con osciloscopio).
2. **Objetivo:** Comprobar la conmutación controlada con la microbomba sumergible JT-DC3W real sumergida en agua, midiendo la caída de tensión en canal ($V_{DS}$), la corriente de régimen ($I_{load}$) y la estabilidad térmica del encapsulado TO-220 tras un ciclo de bombeo máximo continuo ($T = 20\,\text{s}$).
3. **Riesgo cubierto:** F-05 (Sobrecarga de motor, embalamiento térmico del driver, calentamiento destructivo en gabinete cerrado).
4. **Equipo e instrumentos:** Multímetro Digital para corriente DC (escala 10 A), Multímetro para milivoltios DC, Termómetro infrarrojo o termocupla Tipo K calibrada, bandeja plástica antiderrames.
5. **Montaje y topología de medida sin bucle de masa:** Depósito con agua limpia y microbomba `JT-DC3W` sumergida con diodo `1N5819` en antiparalelo directo (cátodo a `+5V`, ánodo a `Drain`). Para `ACT-03.1`: DMM y termómetro IR/termocupla. Para `ACT-03.2` (osciloscopio 2CH): dado que los blindajes BNC de `CH1` y `CH2` comparten masa común, **si se utiliza resistencia *shunt* ($R_{\text{shunt}} = 0.10\,\Omega, 1\%, \ge 2\,\text{W}$) debe colocarse exclusivamente en bajo lado**, en serie entre el `Pin 3 (Source)` del `IRLZ44N` (Terminal 1 = `Nodo N_SHUNT`) y el riel `GND` de la fuente de $+5\,\text{V}$ (Terminal 2 = `Nodo GND_REF`, donde también se unen el `GND` del MCU y $R_{pd2}$). Las pinzas/resortes de masa de **ambas sondas (`CH1` y `CH2`) se conectan únicamente al mismo `Nodo GND_REF`** (jamás en extremos opuestos del shunt); la punta de **CH1** (10X DC) mide el riel `+5V` (`VIN`) y la punta de **CH2** (1X DC o 10X con límite BW $20\,\text{MHz}$, escala $20\text{--}50\,\text{mV/div}$) mide el `Nodo N_SHUNT` (`Pin 3 Source`), obteniendo $V_{\text{shunt}}(t) = V_{\text{Source}}(t) - V_{\text{GND\_REF}}$ sin puentear el shunt.
6. **Precondiciones:** ACT-01 y ACT-02 aprobadas. Prohibido terminantemente encender la microbomba en seco. Prohibido frenar manualmente el rotor para forzar corriente de bloqueo.
7. **Procedimiento:**
   - Registrar la temperatura ambiente del banco ($T_{\text{amb}}$).
   - Iniciar un ciclo de bombeo de 20 segundos continuos (tiempo máximo permitido por watchdog del firmware).
   - Registrar la corriente estable del motor ($I_{\text{bomba}}$).
   - Medir con puntas de prueba directamente sobre las patillas Drain (Pin 2) y Source (Pin 3) la caída $V_{DS}$.
   - Al finalizar los 20 segundos, medir de inmediato la temperatura superficial del encapsulado TO-220 del IRLZ44N ($T_{\text{case}}$) y del diodo 1N5819.
8. **Mediciones a registrar:** $T_{\text{amb}}$, $I_{\text{bomba}}$, $V_{DS}$, $T_{\text{case}}$, $\Delta T = T_{\text{case}} - T_{\text{amb}}$.
9. **Criterios de aceptación separados por régimen e instrumento:**
   - **`ACT-03.1` (Régimen estacionario a $20\,\text{s}$ en agua con DMM + termómetro — `PENDIENTE DE MEDICIÓN EN BANCO`):** (1) $V_{CC,\text{est}} \in [4.75; 5.25]\,\text{V}$; (2) corriente estacionaria **$I_{\text{bomba,est}} \le 400\,\text{mA}$**; (3) $V_{GS,\text{est}} \ge 4.50\,\text{V}$ y **$V_{DS,\text{est}} \le 50.0\,\text{mV}$** en patillas TO-220 a $I_{\text{bomba,est}}$; (4) **$\Delta T \le 15^\circ\text{C}$** y **$T_{\text{case}} \le 45^\circ\text{C}$**; (5) bomba sumergida sin reinicios del MCU.
   - **`ACT-03.2` (Verificación dinámica de arranque con osciloscopio 2CH — `PENDIENTE DE INSTRUMENTACIÓN EXTERNA`):** Con base de tiempo de $5\text{--}10\,\text{ms/div}$ y disparo *Single-Shot* en **CH2** (umbral $+20\,\text{mV}$ sobre `Nodo N_SHUNT` o $+0.20\,\text{A}$ en sonda Hall): (1) en **CH1** (`+5V` respecto a `GND_REF`), tensión mínima transitoria **$V_{CC,\text{mín(din)}} \ge 4.65\,\text{V}$**; (2) en **CH2**, corriente de pico calculada a partir del valor certificado/medido de la resistencia del shunt como **$I_{\text{arranque}} = \frac{V_{\text{shunt,peak}}}{R_{\text{shunt,real}}} \le 1.20\,\text{A}$** ($\le 120.0\,\text{mV}$ para $R_{\text{shunt,real}} = 0.100\,\Omega$). Si se emplea sonda de corriente AC/DC por efecto Hall en lugar de shunt, debe tener ancho de banda $\ge 100\,\text{kHz}$ DC, desmagnetizarse y ajustarse a $0.00\,\text{A}$ antes del disparo abrazando el conductor positivo de la bomba. No puede declararse cumplida solo con multímetro.
10. **Resultado esperado:** $V_{DS} \approx 12\,\text{mV} - 20\,\text{mV}$, potencia disipada $\approx 5\,\text{mW}$, elevación térmica $\Delta T < 2^\circ\text{C}$ (frío al tacto).
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Registro de prueba térmica y foto termográfica/termocupla (Pendiente).
13. **Estado:** `ACT-03.1` `PENDIENTE DE MEDICIÓN EN BANCO` · `ACT-03.2` `PENDIENTE DE INSTRUMENTACIÓN EXTERNA`.
14. **Observaciones y acciones correctivas:** Si $V_{DS} > 50\,\text{mV}$, suspender la prueba inmediatamente; indica que el MOSFET no está saturando adecuadamente o la pieza es un clon fuera de especificación.

---

### ACT-04: Supresión de Transitorios Inductivos (Flyback)
1. **Identificador:** `ACT-04`
2. **Objetivo:** Capturar mediante osciloscopio el pico de fuerza contraelectromotriz (Back-EMF) generado en el borne negativo de la bomba al conmutar de conducción a corte, verificando que el diodo Schottky 1N5819 suprime el transitorio de forma segura por debajo de 6.0 V.
3. **Riesgo cubierto:** F-06 (Destrucción por sobretensión del canal del MOSFET, reinicios espurios de CPU por acoplamiento de ruido en el riel de alimentación).
4. **Equipo e instrumentos:** Osciloscopio Digital con memoria (ancho de banda $\ge 20\,\text{MHz}$), sonda pasiva 10X compensada con cable de masa corto para evitar inductancia espuria en la punta, acoplamiento DC, trigger en flanco de subida en modo disparo único (*Single Shot* o *Normal*).
5. **Puntos exactos de medida y masa:** Con 2 sondas pasivas 10X compensadas en DC: **CH1** directamente en `Pin 2 (Drain)` del `IRLZ44N` (Nodo N6); **CH2** directamente en `Pin 1 (Gate)` (Nodo N5); **resorte/pinza corta de masa de ambas sondas directamente en `Pin 3 (Source)`** (`GND` común, Nodo N8). Bomba sumergida con `1N5819` en antiparalelo directo (cátodo a `+5V`, ánodo a `Drain`).
6. **Precondiciones:** ACT-03 aprobada. Confirmación visual y por multímetro de la polaridad del diodo (cátodo con banda plateada conectado al riel +5V; ánodo a Drain).
7. **Procedimiento de Captura en Dos Pasos (OBL-02):**
   - **Configuración del Nivel de Trigger:**
     * Nivel de trigger fijado en **$+4.50\,\text{V}$** (con flanco de subida `Rising Edge`) sobre la señal de Drenador (Nodo N6).
     * *Justificación del umbral de disparo:* En conducción, el drenador se encuentra en $V_{DS} \le 0.05\,\text{V}$. Al apagarse el MOSFET, la tensión asciende desde $\approx 0\,\text{V}$ hacia $+5.0\,\text{V}$, cruzando limpiamente el umbral de $+4.50\,\text{V}$ antes de alcanzar el pico flyback ($5.35\text{--}5.45\,\text{V}$). Un trigger previo de $+5.5\,\text{V}$ impedía el disparo al recortar el diodo Schottky eficazmente por debajo de dicho valor.
   - **Paso 1: Captura Macroscópica de Desconexión:**
     * Escala vertical: $1.0\,\text{V}/\text{div}$ o $2.0\,\text{V}/\text{div}$.
     * Base de tiempos: $20\,\mu\text{s}/\text{div}$ (o $10\,\mu\text{s}/\text{div}$).
     * Ejecutar un pulso de conmutación de 1 s de la bomba. Capturar la transición completa para descartar rebotes o perturbaciones de baja frecuencia en el riel.
   - **Paso 2: Captura Detallada del Transitorio Rápido (Zoom Flyback):**
     * Mantener trigger en $+4.50\,\text{V}$, ajustar base de tiempos a $500\,\text{ns}/\text{div}$ (o $1\,\mu\text{s}/\text{div}$).
     * Capturar el instante del corte de corriente y medir con cursores la amplitud máxima del sobreimpulso ($V_{\text{peak}}$) y el tiempo de restablecimiento ($t_{\text{clamp}}$).
8. **Mediciones a registrar:** Modelo del osciloscopio, atenuación de sonda (10X), nivel de trigger fijado ($+4.50\,\text{V}$), $V_{\text{peak}}$ en Drain, tiempo de retorno a $+5.0\,\text{V}$, presencia de timbreo parásito (*ringing*).
9. **Configuración de disparo y criterios cuantitativos/cualitativos (`PENDIENTE DE INSTRUMENTACIÓN EXTERNA`):**
   - **Disparos reproducibles (*Single-Shot*):** (1) Apagado macroscópico: trigger ascendente en **CH1 (`Drain`) a $+4.50\,\text{V}$** a **$20\,\mu\text{s/div}$**, midiendo en CH2 el tiempo de caída $t_{f(G)}$ ($90\% \to 10\%$); (2) Zoom Flyback: trigger en **CH1 a $+4.50\,\text{V}$** a **$1\,\mu\text{s/div}$** ($500\,\text{ns/div}$), midiendo $V_{DS(\text{peak})}$ y $t_{\text{clamp}}$; (3) Encendido: trigger ascendente en **CH2 (`Gate`) a $+1.00\,\text{V}$** a **$2\,\mu\text{s/div}$**.
   - **Umbrales cuantitativos:** En CH1 (`Drain`), **$V_{DS(\text{peak})} \le V_{CC} + 0.60\,\text{V}$** (y $\le 6.00\,\text{V}$). En CH2 (`Gate`), durante el apagado, una vez que $V_{GS}$ baja de $0.80\,\text{V}$ ningún rebote posterior puede superar **$0.80\,\text{V}$**; en encendido, **$V_{GS(\text{peak})} \le 6.00\,\text{V}$**. El tiempo $t_{f(G)}$ y las oscilaciones amortiguadas sub-umbral se registran documentalmente (etapa restringida a ON/OFF sin PWM).
10. **Resultado esperado:** Disparo garantizado en $+4.50\,\text{V}$, pico recortado limpiamente en $\approx 5.40\,\text{V}$ sin rebotes destructivos.
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Captura de pantalla del osciloscopio (.PNG / .CSV) con cursores de tensión (Pendiente).
13. **Estado:** `PENDIENTE DE INSTRUMENTACIÓN EXTERNA`.
14. **Observaciones y acciones correctivas:** Si el osciloscopio registra picos $> 12\,\text{V}$, el diodo 1N5819 está dañado, invertido o el cableado del diodo presenta inductancia parásita excesiva (debe ubicarse pegado a los terminales de la bomba).

---

### ACT-05: Enclavamiento Fail-Safe de Boya — Estado Estacionario y Latencias Lógica (`ACT-05.3a`) vs. Eléctrica (`ACT-05.3b`)
1. **Identificador:** `ACT-05` (desglosada en **`ACT-05.1`**, **`ACT-05.2`**, **`ACT-05.3a`** y **`ACT-05.3b`**).
2. **Objetivo:** Comprobar física y lógicamente que la entrada del sensor de nivel por boya magnética (CS-CO058) conmuta limpiamente con su resistencia de pull-up interna, entra en enclavamiento de seguridad (`FAULT / NO_WATER_LEVEL`) en $< 50\,\text{ms}$ al detectar nivel bajo y mantiene la bomba incondicionalmente apagada ante desconexión o rotura de cable.
3. **Riesgo cubierto:** F-01 (Marcha en seco de la bomba, cavitación y quema de sellos), F-12 (Cable de flotador cortado o suelto).
4. **Equipo e instrumentos:** Multímetro Digital (Voltaje DC), depósito de agua de prueba con boya magnética montada vertical/horizontalmente, Monitor Serie a 115200 baudios.
5. **Montaje:** Interruptor de flotador conectado entre `D4` (Nano) / `GPIO 19` (ESP32, `INPUT_PULLUP`) y `GND`. Para `ACT-05.1`/`05.2`: DMM entre `Pin 1 (Gate)` y `Pin 3 (Source)`. Para `ACT-05.3a`: analizador lógico 2CH u osciloscopio 2CH en `D4`/`GPIO 19` y `D3`/`GPIO 18`. Para `ACT-05.3b`: **exclusivamente osciloscopio digital 2CH** con masas de ambas sondas en `Pin 3 (Source)`, ejecutando **dos adquisiciones independientes y secuenciales**: `ACT-05.3b-1` (`CH1` en flotador y `CH2` en `Pin 2 Drain`) y `ACT-05.3b-2` (`CH1` en flotador y `CH2` en `Pin 1 Gate`).
6. **Precondiciones:** Firmware v1.2.1 cargado en placa. Lazo de control ejecutándose.
7. **Procedimiento:**
   - Escenario 1 (Depósito con agua): Boya arriba. Medir tensión en pin de entrada. Solicitar riego manual. La bomba debe encender.
   - Escenario 2 (Depósito agotado durante riego): Con la bomba activa, descender manualmente la boya. Registrar el tiempo de corte de la bomba y la indicación en display OLED / Monitor Serie.
   - Escenario 3 (Desconexión de cable / Falla de circuito): Con la bomba en reposo y agua presente, desconectar físicamente uno de los cables del flotador de la bornera. Intentar forzar riego mediante pulsador físico y solicitud web.
8. **Mediciones a registrar:** Tensión en pin con boya alta ($V_{\text{in, closed}}$), tensión con boya baja ($V_{\text{in, open}}$), tiempo de respuesta de corte ante caída de boya, estado de salida de bomba.
9. **Sub-verificaciones, instrumentación y criterios cuantificables:**
   - **`ACT-05.1` (Estado estacionario y bloqueo `FAULT` — `PENDIENTE DE ENSAYO FÍSICO`):** En `D4` (Nano) / `GPIO 19` (ESP32), boya alta mide $\le 0.30\,\text{V}$ (`LOW`); boya baja o cable desconectado sube a $\ge 3.00\,\text{V}$ (ESP32) / $\ge 4.50\,\text{V}$ (Nano) por `INPUT_PULLUP`, bloqueando el bombeo (`"SIN AGUA!"`) y rechazando riego manual/web.
   - **`ACT-05.2` (Corte en compuerta durante fallo — `PENDIENTE DE MEDICIÓN EN BANCO`):** Con DMM entre `Pin 1 (Gate)` y `Pin 3 (Source)` en estado `FAULT`, **$V_{GS} \le 0.10\,\text{V}$**.
   - **`ACT-05.3a` (Latencia lógica de firmware — `PENDIENTE DE INSTRUMENTACIÓN EXTERNA`):** Entre entrada de boya (`D4`/`GPIO 19`) y salida del MCU (`D3`/`GPIO 18`), medida con **analizador lógico 2CH u osciloscopio 2CH**: **$\le 60\,\text{ms}$**. *Advertencia:* solo verifica la orden digital; **no demuestra descarga de compuerta ni apagado eléctrico de la bomba**.
   - **`ACT-05.3b` (Latencia eléctrica completa mediante 2 adquisiciones obligatorias con osciloscopio 2CH — `PENDIENTE DE INSTRUMENTACIÓN EXTERNA`):** Con bomba sumergida en marcha, base de $10\,\text{ms/div}$ (ventana $100\,\text{ms}$) y disparo *Single-Shot* ascendente en **CH1** (`D4` a $+2.50\,\text{V}$ en Nano / `GPIO 19` a $+1.50\,\text{V}$ en ESP32), masa de ambas sondas en `Pin 3 (Source)`: (1) **Adquisición `ACT-05.3b-1` (Transición del nodo de potencia en `Drain`):** con **CH2 en `Pin 2 (Drain)`** (Nodo N6), el tiempo $\Delta t_{\text{Drain}}$ desde el cruce de umbral en CH1 hasta que `Drain` asciende y se estabiliza en $\ge 0.90 \times V_{CC}$ ($\ge 4.50\,\text{V}$ tras el recorte inductivo) debe cumplir **$\Delta t_{\text{Drain}} \le 80.0\,\text{ms}$**; (2) **Adquisición `ACT-05.3b-2` (Descarga de compuerta en `Gate`):** rearmando el ensayo con **CH2 en `Pin 1 (Gate)`** (Nodo N5), el tiempo $\Delta t_{\text{Gate}}$ desde el cruce en CH1 hasta que la compuerta cae permanentemente a **$V_{GS} \le 0.10\,\text{V}$** debe cumplir **$\Delta t_{\text{Gate}} \le 80.0\,\text{ms}$**. **Ambas capturas (`ACT-05.3b-1` y `ACT-05.3b-2`) son obligatorias** y deben cumplir $\le 80.0\,\text{ms}$ para aprobar `ACT-05.3b`.
10. **Resultado esperado:** Bloqueo total e inmediato de la bomba, transición a `FAULT`, advertencia acústica y visual activa.
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Captura de log serie y video de respuesta de corte (Pendiente).
13. **Estado:** `ACT-05.1`/`05.2` `PENDIENTE DE MEDICIÓN EN BANCO` · `ACT-05.3a`/`05.3b` `PENDIENTE DE INSTRUMENTACIÓN EXTERNA`.
14. **Observaciones y acciones correctivas:** Si la entrada no sube a $> 3.0\,\text{V}$ al desconectar el cable, verificar la activación por software de `pinMode(pin, INPUT_PULLUP)` en el firmware.

---

### ACT-06: Límites Temporales de Riego (20 s vs 25 s), Recuperación de FSM y Validación HTTP Real (OBL-01 / OBL-04)
1. **Identificador:** `ACT-06`
2. **Objetivo:** Verificar en silicio real y de manera independiente los tres mecanismos de protección de software y red:
   - **Subprotocolo ACT-06.A1 (Límite Operativo de Riego Automático — 20.0 s en Firmware de Producción):** Comprobar que con el firmware de producción intacto (`maxAutoDurationMs = 20000 ms`), un ciclo automático que no alcance la meta de humedad es cortado exactamente a los **$20.0\,\text{s} \pm 0.5\,\text{s}$** con razón identificable `StopReason::AUTO_MAX_DURATION_REACHED` (`"TIMEOUT 20S"` en OLED), entrando en estado de bloqueo seguro `FAULT (MAX_TIMEOUT_EXCEEDED)`.
   - **Subprotocolo ACT-06.A2 (Watchdog Físico Absoluto — 25.0 s mediante Configuración de Prueba Aislada):** Comprobar que la barrera redundante de segundo nivel (`maxSafetyWatchdogMs = 25000 ms`) corta incondicionalmente a los **$25.0\,\text{s} \pm 0.5\,\text{s}$** con razón identificable `StopReason::SAFETY_TIMEOUT` (`"WDT 25S ERR"` en OLED) cuando se ejecuta bajo el flag de preprocesador aislado de banco (`-DFLORACARE_BENCH_TEST_WATCHDOG_25S`), sin alterar las constantes del firmware de producción.
   - **Subprotocolo ACT-06.B (Seguridad HTTP, Rate Limiter y Control de Actuador):** Respuesta determinista del servidor WebServer real compilado en el ESP32 ante 7 escenarios HTTP estructurados, verificación del limitador de frecuencia (429) activado específicamente por token inválido (`lastFailedAuthMs`), y preservación del apagado de la bomba ante peticiones rechazadas.
3. **Riesgo cubierto:** F-05 (Límite de ciclo automático de 20 s y Watchdog absoluto de 25 s), F-08, F-16, F-17, F-18 (Seguridad HTTP, ataques de temporización, desbordamiento de memoria heap en silicio).
4. **Equipo e instrumentos:** Cronómetro digital calibrado, estación de trabajo con Python / cURL / Postman conectada a la misma subred Wi-Fi privada del ESP32, Monitor Serie a 115200 baudios, Multímetro monitoreando N5 ($V_{GS}$) y N6 ($V_{DS}$).
5. **Montaje:** ESP32 DevKit V1 conectado a red local de laboratorio privada (VLAN IoT aislada). Sonda capacitiva mantenida en aire seco (<20% de humedad) para forzar demanda de riego continuo. Bomba monitoreada eléctricamente (sumergida en agua o sustituida por carga ficticia de $15\,\Omega$, $5\,\text{W}$).
6. **Precondiciones:** `ACT-01.1`, `ACT-02`, `ACT-03.1` y `ACT-05.1`/`05.2` verificadas en banco (o carga ficticia de $15\,\Omega, 5\,\text{W}$ conectada). Archivo `Credentials.h` con token $\ge 16$ caracteres no predeterminado.
7. **Procedimiento Operativo:**

   #### Subprotocolo ACT-06.A1: Validación del Límite Operativo Automático (20.0 s — Firmware de Producción)
   - Con el firmware de producción estándar cargado (`maxAutoDurationMs = 20000 ms`, sin flags de prueba) y el sensor en aire seco (<20%), permitir que el controlador inicie el riego automático (`IrrigationSource::AUTO_CYCLE`).
   - Iniciar simultáneamente el cronómetro.
   - Dejar correr el ciclo sin humedecer el sensor (simulando rotura de manguera o sonda fuera de maceta).
   - Registrar el instante exacto en que el firmware corta la salida (`GPIO 18` a `LOW`, $V_{GS} \le 0.10\,\text{V}$).
   - Verificar en el Monitor Serie la traza identificable: `[FAULT] AUTO_MAX_DURATION_REACHED (20000 ms) -> MAX_TIMEOUT_EXCEEDED` y en la pantalla OLED el mensaje `"TIMEOUT 20S"`.
   - Pulsar el botón manual durante los siguientes 5 segundos y posteriormente para verificar que el controlador permanece enclavado en `ControllerState::FAULT` y rechaza todo rearme hasta ejecutar recuperación explícita o reinicio.

   #### Subprotocolo ACT-06.A2: Validación Aislada del Watchdog Absoluto (25.0 s — Sin Modificar Código de Producción)
   - *Fundamento técnico y delimitación de alcance de evidencia (`OBL-04` / `NUEVO-01`):* En operación normal de producción, el límite específico de riego automático ($20.000\,\text{ms}$) se alcanza antes que el watchdog global ($25.000\,\text{ms}$). La verificación de la barrera de $25.000\,\text{ms}$ (`elapsedMs >= config_.maxSafetyWatchdogMs`) se estructura en niveles estrictamente diferenciados que **no deben confundirse ni atribuirse entre sí**:
     1. **Nivel 1 — Prueba de Lógica Simulada en Python (`test_31_absolute_safety_watchdog_25s_isolated` en `tests/test_irrigation_suite.py`):** Ejercita exclusivamente la clase simulada `MockIrrigationController` en Python. Valida el modelo lógico equivalente, pero **no ejecuta el código C++ real ni demuestra por sí sola el comportamiento del firmware**.
     2. **Nivel 2 — Ejecución Nativa del Controlador C++ Real (`tests/test_native_controller.cpp` y `test_32_native_cpp_controller_20s_and_25s_watchdog`):** Compila con `g++ -std=c++17` y ejecuta directamente la cabecera de producción `include/IrrigationController.h` en dos binarios nativos dentro de CI:
        - Sin flag (modo producción): comprueba que `FloraCare::IrrigationController` corta a $20.000\,\text{ms}$ con `StopReason::AUTO_MAX_DURATION_REACHED`.
        - Con `-DFLORACARE_BENCH_TEST_WATCHDOG_25S` (modo aislado): comprueba que `FloraCare::IrrigationController` permanece en `WATERING` a $20.000\,\text{ms}$ y $24.999\,\text{ms}$, y corta exactamente a $25.000\,\text{ms}$ con `StopReason::SAFETY_TIMEOUT`.
     3. **Nivel 3 — Compilación Cruzada Explícita del Firmware de Banco en CI (`platformio.ini` / `ci.yml`):** Además de los entornos de producción (`nanoatmega328` y `esp32dev`), CI compila explícitamente los entornos aislados `pio run -e nanoatmega328_bench_wdt25s` y `pio run -e esp32dev_bench_wdt25s` (que aplican `-D FLORACARE_BENCH_TEST_WATCHDOG_25S` sobre `regador_nano_pro.cpp` y `regador_esp32.cpp`).
     4. **Nivel 4 — Ejecución Física en Banco (`PENDIENTE DE MEDICIÓN EN BANCO`):** Las validaciones automatizadas de los Niveles 1, 2 y 3 **no sustituyen** la prueba física en la placa ni la medición eléctrica de la salida del driver. Para ejecutarla en el laboratorio:
        - Compilar y cargar temporalmente el entorno aislado reproducible: `pio run -e esp32dev_bench_wdt25s -t upload` (o `pio run -e nanoatmega328_bench_wdt25s -t upload`).
        - Con la sonda en aire seco (<20%), iniciar el ciclo automático y el cronómetro.
        - Confirmar que el bombeo supera la marca de $20.0\,\text{s}$ y es cortado incondicionalmente por el techo del watchdog a los **$25.0\,\text{s} \pm 0.5\,\text{s}$** ($V_{GS} \le 0.10\,\text{V}$).
        - Verificar en el Monitor Serie la traza exclusiva de esta ruta: `[FAULT] SAFETY_TIMEOUT_WATCHDOG (25000 ms) -> MAX_TIMEOUT_EXCEEDED` y en la pantalla OLED el mensaje `"WDT 25S ERR"`.
        - **Paso Obligatorio de Restauración:** Recompilar y flashear de inmediato el entorno de producción estándar (`pio run -e esp32dev -t upload` / `pio run -e nanoatmega328 -t upload`) y confirmar que el corte vuelve a ocurrir a los $20.0\,\text{s}$ (`"TIMEOUT 20S"`) antes de dar por cerrado el subprotocolo.

   #### Subprotocolo ACT-06.B: Secuencia Estructurada de 7 Escenarios HTTP (ESP32 WebServer)
   Ejecutar en orden estricto los siguientes 7 escenarios mediante cURL, registrando código devuelto, tiempo y estado de bomba:

   | ID Escenario | Petición y Cabeceras HTTP | Comportamiento Interno de `lastFailedAuthMs` / Intervalo | Código Esperado | Estado de Bomba |
   | :--- | :--- | :--- | :---: | :---: |
   | **HTTP-REQ-01** | `GET /api/water` | Inicio de suite (no modifica `lastFailedAuthMs`) | **`405 Method Not Allowed`** | OFF ($V_{GS} \le 0.10\,\text{V}$) |
   | *Pausa TCP 1* | Separación entre solicitudes | Esperar $500\,\text{ms}$ (evita solapamiento de sockets) | -- | Inactiva |
   | **HTTP-REQ-02** | `POST /api/water` (Sin cabecera `Authorization` ni token) | Ausencia de token (`chosenToken.length() == 0`); **no actualiza `lastFailedAuthMs`** | **`401 Unauthorized`** | OFF ($V_{GS} \le 0.10\,\text{V}$) |
   | *Pausa TCP 2* | Separación limpia de estado entre solicitudes | Esperar $500\,\text{ms}$ | -- | Inactiva |
   | **HTTP-REQ-03** | `POST /api/water` con `Authorization: Bearer TOKEN_INVALIDO` | Fallo de comparación (`!authorized`); **registra `lastFailedAuthMs = nowMs` y arma ventana de $1.000\,\text{ms}$** | **`401 Unauthorized`** | OFF ($V_{GS} \le 0.10\,\text{V}$) |
   | **HTTP-REQ-04** | `POST /api/water` con cualquier payload | **Ráfaga inmediata ($< 100\,\text{ms}$ tras REQ-03)** dentro de la ventana `(nowMs - lastFailedAuthMs) < 1000` | **`429 Too Many Requests`** | OFF ($V_{GS} \le 0.10\,\text{V}$) |
   | *Reinicio Rate Limiter* | **Pausa de Despenalización Obligatoria** | **Esperar $\ge 1.500\,\text{ms}$** para que expire la ventana de $1.000\,\text{ms}$ armada en REQ-03 | -- | Inactiva |
   | **HTTP-REQ-05** | `POST /api/water` con token válido pero `Origin: http://sitio-malicioso.com` | Intento CSRF con origen cruzado tras expirar rate limit | **`403 Forbidden`** | OFF ($V_{GS} \le 0.10\,\text{V}$) |
   | *Pausa TCP 3* | Separación entre solicitudes | Esperar $500\,\text{ms}$ | -- | Inactiva |
   | **HTTP-REQ-06** | `POST /api/water` con token válido pero cuerpo $> 256$ bytes | Carga útil sobredimensionada (`clientContentLength() > 256`) | **`413 Payload Too Large`** | OFF ($V_{GS} \le 0.10\,\text{V}$) |
   | *Pausa TCP 4* | Separación entre solicitudes | Esperar $500\,\text{ms}$ | -- | Inactiva |
   | **HTTP-REQ-07** | `POST /api/water` con token válido, `Origin` canónico y body válido | Petición legítima autorizada | **`303 See Other`** | **ON durante $3.0\,\text{s}$** luego OFF |

   - Monitorear en el Monitor Serie el valor de `ESP.getFreeHeap()` antes de iniciar REQ-01 y tras finalizar REQ-07.

8. **Mediciones a registrar:**
   - Subprotocolo A1: Tiempo de corte automático en producción ($T_{\text{auto20}}$), mensaje OLED (`"TIMEOUT 20S"`), traza serie (`AUTO_MAX_DURATION_REACHED`).
   - Subprotocolo A2: Tiempo de corte por watchdog absoluto en modo aislado ($T_{\text{wdt25}}$), mensaje OLED (`"WDT 25S ERR"`), traza serie (`SAFETY_TIMEOUT_WATCHDOG`), confirmación de reflasheo del binario de producción.
   - Subprotocolo B: Códigos de estado HTTP para REQ-01 a REQ-07, confirmación de bomba inactiva en REQ-01..06, duración de pulso en REQ-07, telemetría de heap ($Heap_{\text{init}}$, $Heap_{\text{final}}$).
9. **Criterios de aceptación cuantificables:**
   - **Criterio A1 (Límite Automático 20 s):** $T_{\text{auto20}} = 20.0\,\text{s} \pm 0.5\,\text{s}$; causa `StopReason::AUTO_MAX_DURATION_REACHED`, transición a `FAULT (MAX_TIMEOUT_EXCEEDED)`, compuerta $V_{GS} \le 0.10\,\text{V}$.
   - **Criterio A2 (Watchdog Absoluto 25 s):** $T_{\text{wdt25}} = 25.0\,\text{s} \pm 0.5\,\text{s}$ bajo compilación de prueba aislada; causa `StopReason::SAFETY_TIMEOUT`, transición a `FAULT (MAX_TIMEOUT_EXCEEDED)`, compuerta $V_{GS} \le 0.10\,\text{V}$, y restauración verificada del firmware de producción ($20.0\,\text{s}$).
   - **Criterio B (HTTP y Rate Limiting):**
     * Coincidencia estricta y secuencial en los 7 códigos HTTP: `405`, `401`, `401`, `429`, `403`, `413`, `303`.
     * Bomba estrictamente apagada ($V_{GS} \le 0.10\,\text{V}$) en los escenarios 1 a 6.
     * Riego accionado exclusivamente en REQ-07 con duración de pulso de $3.0\,\text{s} \pm 0.2\,\text{s}$.
     * Estabilidad de memoria heap: $\Delta Heap = Heap_{\text{init}} - Heap_{\text{final}} \le 200\,\text{bytes}$ tras completar los 7 escenarios.
10. **Resultado esperado:** Corte automático de producción a los 20.0 s (`AUTO_MAX_DURATION_REACHED`); corte de watchdog absoluto a los 25.0 s (`SAFETY_TIMEOUT`) en prueba aislada; suite de 7 escenarios HTTP cumple secuencialmente sin fugas de memoria.
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Registros de cURL con timestamps y log serie con trazas de parada y lecturas de heap (Pendiente).
13. **Estado:** `PENDIENTE DE MEDICIÓN EN BANCO`.
14. **Observaciones y acciones correctivas:** No dejar nunca instalado el binario con `-DFLORACARE_BENCH_TEST_WATCHDOG_25S` tras concluir ACT-06.A2. Si se detecta fuga de heap acumulativa, revisar instancias `String` en los callbacks de `WebServer`.

---

## 📌 Fase 4: Ensamble Hidráulico y Prevención de Efecto Sifón

> [!WARNING] El Fenómeno del Sifón Hidráulico
> Si el depósito de agua se encuentra a mayor cota (altura) que la maceta, una vez que la bomba arranque y purgue el aire del conducto, el agua continuará fluyendo por gravedad **aún con la bomba completamente apagada**, inundando la planta de forma descontrolada. Este problema **no puede corregirse por software**.

### Reglas de Diseño e Instalación Hidráulica Obligatorias:
1. **Posicionamiento relativo:** La superficie del nivel de agua del depósito debe ubicarse **a la misma altura o ligeramente por debajo** de la superficie superior de la maceta.
2. **Corte de sifón por espacio de aire (Air Gap):**
   - El extremo de salida del tubo de silicona/vinilo **nunca debe quedar enterrado o sumergido en la tierra húmeda o en agua estancada**.
   - Debe fijarse con una abrazadera o estaca rígida manteniendo una separación libre de aire (Air Gap) de **al menos 20 mm (2 cm)** sobre la superficie del sustrato.
3. **Agujero rompe-sifón (Siphon Break Hole):** En caso de que el recorrido del tubo obligatoriamente pase por encima del nivel de agua, practica una perforación microscópica (0.5 mm con una aguja caliente) en el tramo de manguera que queda dentro del depósito, por encima del nivel máximo de agua. Esto introduce aire e interrumpe la columna de vacío cuando la bomba se detiene.
4. **Filtro de succión:** Coloca un tramo de malla fina de nylon o tul alrededor de la rejilla de entrada de la microbomba sumergible para evitar que suciedad o pelusas bloqueen el impulsor magnético.

---

## 📌 Fase 5: Matriz Resumen de Estados de Pruebas de Banco

| Código de Prueba | Subsistema / Verificación Específica | Estado Actual | Instrumento Obligatorio | Condición / Bloqueo Técnico |
| :---: | :--- | :---: | :--- | :--- |
| **`ACT-01.1`** | Corte estático en boot, reset y cable suelto ($V_{GS} \le 0.10\,\text{V}$) | `PENDIENTE DE MEDICIÓN EN BANCO` | DMM + LED testigo ($330\,\Omega$) | Comprobación preliminar; **no cierra `ACT-01`** |
| **`ACT-01.2`** | Captura determinista Modo 1 (11 trazas, $V_{GS(\text{peak})} \le 0.50\,\text{V}$ y $<0.80\,\text{V}$) + Modo 2 (`0` disparos $\ge +0.50\,\text{V}$) | `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` | Osciloscopio $\ge 20\,\text{MHz}$ (2CH, Modos 1 y 2) | Requiere laboratorio con osciloscopio |
| **`ACT-02`** | Excitación estática ($V_{GS} \ge 4.50\,\text{V}, V_{DS} \le 15.0\,\text{mV}$ a $15\,\Omega$, pulso $3\,\text{s}$) | `PENDIENTE DE MEDICIÓN EN BANCO` | DMM (exactitud registrada) + Termómetro | Superar `ACT-01.1`; zona $>15\,\text{mV}$ rechaza prueba |
| **`ACT-03.1`** | Conmutación estacionaria con bomba sumergida ($V_{DS} \le 50\,\text{mV}, I_{\text{est}} \le 400\,\text{mA}$) | `PENDIENTE DE MEDICIÓN EN BANCO` | DMM + Termómetro + Agua | Superar `ACT-01.1` y `ACT-02` |
| **`ACT-03.2`** | Arranque dinámico ($V_{CC,\text{mín}} \ge 4.65\,\text{V}, I_{\text{arranque}} = V_{\text{shunt,peak}}/R_{\text{shunt,real}} \le 1.20\,\text{A}$) | `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` | Osciloscopio 2CH + Shunt bajo lado $0.10\,\Omega$ (masa común `GND_REF`) o Sonda Hall | No verificable solo con DMM |
| **`ACT-04`** | Recorte Flyback ($V_{DS(\text{peak})} \le V_{CC}+0.60\,\text{V}$), $t_{f(G)}$ y rebote $<0.80\,\text{V}$ | `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` | Osciloscopio $\ge 20\,\text{MHz}$ (2 sondas 10X en Drain/Gate) | Requiere laboratorio con osciloscopio |
| **`ACT-05.1`** | Bloqueo estacionario `FAULT` ante boya baja o hilo abierto (`D4`/`GPIO 19`) | `PENDIENTE DE ENSAYO FÍSICO` | Boya en depósito + DMM + OLED/Serie | Montaje físico pendiente |
| **`ACT-05.2`** | Tensión de corte en compuerta durante fallo ($V_{GS} \le 0.10\,\text{V}$) | `PENDIENTE DE MEDICIÓN EN BANCO` | DMM en Nodo N5 | Montaje físico pendiente |
| **`ACT-05.3a`** | Latencia lógica (`D4`/`GPIO 19` $\to$ `D3`/`GPIO 18` $\le 60\,\text{ms}$) | `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` | Analizador lógico 2CH u Osciloscopio 2CH | Solo cubre orden digital; **no cierra latencia física** |
| **`ACT-05.3b`** | Latencia eléctrica en 2 capturas: `ACT-05.3b-1` (`Drain` $\ge 0.9V_{CC}$ en $\le 80\,\text{ms}$) y `ACT-05.3b-2` (`Gate` $\le 0.10\,\text{V}$ en $\le 80\,\text{ms}$) | `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` | **Osciloscopio 2CH** (2 capturas obligatorias) | Requiere laboratorio con osciloscopio |
| **`ACT-06`** | Límite auto $20\,\text{s}$ (`A1`), Watchdog aislado $25\,\text{s}$ (`A2`) y 7 casos HTTP (`B`) | `PENDIENTE DE MEDICIÓN EN BANCO` | Cronómetro + DMM + cURL en LAN privada | Restaurar firmware de producción tras `ACT-06.A2` |

> [!CAUTION]
> **Prohibición de Operación Desatendida:**
> El sistema no debe dejarse operando sin supervisión humana continua hasta que todas las pruebas y sub-verificaciones de la matriz anterior (incluidas las capturas con osciloscopio `ACT-01.2`, `ACT-03.2`, `ACT-04` y `ACT-05.3b`) hayan sido ejecutadas, firmadas con fecha y sus resultados se encuentren estrictamente dentro de los criterios de aceptación cuantitativos.
