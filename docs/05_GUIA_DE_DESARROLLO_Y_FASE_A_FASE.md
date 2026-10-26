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

Antes de suministrar energía eléctrica a la protoboard, realiza los siguientes ensayos con un Multímetro Digital (DMM) en modo continuidad (bip) y resistencia ($\Omega$):

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
  - **Alternativa Comercial HW-517:** Clasificada como `PENDIENTE DE EVALUACIÓN / NO RECOMENDADO`. Sujeto a verificación previa de esquemático y caída $V_{DS} \le 50\,\text{mV}$.

---

## 🔬 Protocolo Oficial de Pruebas Físicas de Banco (ACT-01 a ACT-06)

> [!WARNING]
> Las siguientes pruebas corresponden al protocolo de validación instrumental sobre hardware real.
> **Ninguna de estas pruebas ha sido ejecutada en silicio todavía**; todos los procedimientos se encuentran en estado de preparación técnica controlada. Está estrictamente prohibido marcar una prueba como aprobada sin contar con mediciones numéricas firmadas en banco.

---

### ACT-01: Estado Seguro del Driver (Fail-Safe en Reposo, Boot y Reset)
1. **Identificador:** `ACT-01`
2. **Objetivo:** Verificar cuantitativamente que la etapa de potencia y la compuerta del MOSFET permanecen en corte estricto ($V_{GS} < 0.10\,\text{V}$) durante el arranque inicial, el reinicio físico, la desconexión del cable de señal y el estado de alta impedancia (High-Z) del microcontrolador.
3. **Riesgo cubierto:** F-07 (Inversión lógica, compuerta flotante o activación espuria de bomba durante transitorios de encendido).
4. **Equipo e instrumentos:** Multímetro Digital (DMM) calibrado (resolución $\le 1\,\text{mV}$) y osciloscopio digital (acoplamiento DC, base de tiempo 200 ms/div, trigger en flanco de subida de riel 5V).
5. **Montaje:** Driver NPN+PNP montado en protoboard con Q1 (2N2222A), Q2 (2N3906), Q3 (IRLZ44N) y resistencias ($R_1 = 2.2\,\text{k}\Omega$, $R_{pd1} = 10\,\text{k}\Omega$, $R_2 = 2.2\,\text{k}\Omega$, $R_{pu} = 10\,\text{k}\Omega$, $R_{pd2} = 10\,\text{k}\Omega$). Carga simulada con LED indicador + resistencia de 330 Ω entre +5V y Drain. Bomba mecánica desconectada.
6. **Precondiciones:** Fuente externa de 5.0 V regulada y estabilizada. Microcontrolador programado con firmware v1.2.1.
7. **Procedimiento:**
   - Medir $V_{GS}$ con multímetro entre Pin 1 (Gate) y Pin 3 (Source) del IRLZ44N en 4 escenarios:
     a) Fuente de 5V encendida, ESP32 apagado / desconectado.
     b) ESP32 energizado, durante los primeros 500 ms de bootloader.
     c) Pulsando y manteniendo presionado el botón físico EN/RESET del ESP32.
     d) Cable de señal desconectado de GPIO 18 (entrada flotante al driver).
8. **Mediciones a registrar:** $V_{B(Q1)}$, $V_{B(Q2)}$, $V_{Gate(Q3)}$, corriente de fuga $I_{leak}$ en Drain.
9. **Criterios de aceptación cuantificables:**
   - $V_{GS} \le 0.10\,\text{V}$ ($100\,\text{mV}$) en los cuatro escenarios evaluados.
   - LED indicador de carga completamente apagado ($I_{leak} < 10\,\mu\text{A}$).
   - Osciloscopio confirma ausencia de pulsos espurios superiores a $0.50\,\text{V}$ durante la rampa de encendido de la fuente.
10. **Resultado esperado:** $V_{GS} = 0.00\,\text{V}$, corte absoluto garantizado por $R_{pd1}$, $R_{pu}$ y $R_{pd2}$.
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Captura oscilográfica del transitorio de boot y fotografía de pantalla del DMM (Pendiente).
13. **Estado:** `LISTA PARA BANCO`.
14. **Observaciones y acciones correctivas:** Si se detecta $V_{GS} > 0.20\,\text{V}$ en reset, verificar que $R_{pd1}$ y $R_{pu}$ no presenten soldadura fría o valores alterados.

---

### ACT-02: Compatibilidad de Niveles Lógicos y Excitación de Compuerta
1. **Identificador:** `ACT-02`
2. **Objetivo:** Comprobar que el circuito de adaptación de 2 etapas NPN+PNP entrega una tensión de compuerta plena ($V_{GS} \ge 4.50\,\text{V}$) al ser activado desde el GPIO de 3.3 V del ESP32, asegurando saturación profunda del MOSFET IRLZ44N dentro de especificación de catálogo ($R_{DS(on)} \le 0.035\,\Omega$).
3. **Riesgo cubierto:** Conducción parcial del MOSFET a 3.3V, sobrecalentamiento por resistencia de canal elevada y fallo de par de arranque de la bomba.
4. **Equipo e instrumentos:** Multímetro Digital (DMM) calibrado (resolución 1 mV), termocupla tipo K o termómetro infrarrojo (IR) sin contacto.
5. **Montaje, Trayectoria Eléctrica y Seguridad Térmica (MEN-02 / MEN-04):**
   - **Pre-driver:** Conectar Base de Q1 a GPIO 18 vía $R_1 = 2.2\,\text{k}\Omega$ (con $R_{pd1} = 10\,\text{k}\Omega$ a GND); Emisor de Q1 a GND; Colector de Q1 a Base de Q2 vía $R_2 = 2.2\,\text{k}\Omega$.
   - **Etapa PNP:** Emisor de Q2 al riel de $+5.0\,\text{V}$ estabilizado; Base de Q2 polarizada con $R_{pu} = 10\,\text{k}\Omega$ a $+5.0\,\text{V}$; Colector de Q2 a Gate de Q3 (con $R_{pd2} = 10\,\text{k}\Omega$ a GND).
   - **Carga de Prueba de Bajo Lado (Low-Side Test Load):**
     * Conectar un extremo de la resistencia de potencia de $15\,\Omega$, $5\,\text{W}$ directamente al riel de $+5.0\,\text{V}$ de la fuente externa.
     * Conectar el segundo extremo de la resistencia al terminal Drenador (Drain, pin 2) del MOSFET Q3 (Nodo N6).
     * Conectar el terminal Fuente (Source, pin 3) del MOSFET Q3 directamente a la masa común (GND).
     * *Comprobación de disipación y corriente teórica:* $I = \frac{5.0\,\text{V}}{15\,\Omega} \approx 0.333\,\text{A} = 333\,\text{mA}$; $P = \frac{V^2}{R} = \frac{25}{15} \approx 1.67\,\text{W}$.
   - **Instrumentación Térmica, Régimen de Pulso y Criterio de Parada Sin Contacto (MEN-04):**
     * **Prohibición de contacto manual:** Queda estrictamente prohibido tocar la resistencia de potencia o el MOSFET mientras el circuito esté energizado o inmediatamente después del pulso.
     * **Medición de temperatura:** Fijar mecánicamente una sonda termocupla tipo K sobre el cuerpo cerámico central de la resistencia de $15\,\Omega$ (o apuntar un termómetro IR a $5\text{--}10\,\text{cm}$ de su superficie).
     * **Duración y ciclo de trabajo:** La carga solo se energizará durante pulsos supervisados de $3.0\,\text{s}$, dejando un intervalo de enfriamiento mínimo de $30\,\text{s}$ entre activaciones.
     * **Verificación de corriente:** Intercalar amperímetro en serie (escala 10 A o 2 A DC) entre la resistencia y el Drain, o medir tensión en bornes de la resistencia ($I = V_R / 15\,\Omega$).
     * **Criterio de Parada de Emergencia Instrumental:** Cortar de inmediato el interruptor de la fuente de $+5\,\text{V}$ si la lectura de termocupla/IR supera **$60.0\,^\circ\text{C}$**, si la corriente medida se desvía fuera de la ventana **$300\text{--}370\,\text{mA}$**, o si la caída $V_{DS}$ en Q3 supera **$0.50\,\text{V}$** durante la conducción.
     * **Manipulación segura:** Desenergizar siempre la fuente externa y comprobar $0.0\,\text{V}$ con el multímetro antes de modificar cualquier conexión.
6. **Precondiciones:** ACT-01 aprobada. Firmware configurado para emitir comando de riego manual de 3 s.
7. **Procedimiento:**
   - Activar el riego manual mediante pulsador.
   - Medir durante el intervalo de 3 s la tensión en GPIO 18 ($V_{OH}$), la tensión de base de Q1 ($V_{BE1}$), la tensión colector-emisor de Q1 ($V_{CE1}$), la tensión base-emisor de Q2 ($V_{EB2}$) y la tensión efectiva Gate-Source en Q3 ($V_{GS}$).
8. **Mediciones a registrar:** $V_{GPIO}$, $V_{BE1}$, $V_{CE1}$, $V_{EB2}$, $V_{EC2}$, $V_{GS}$.
9. **Criterios de aceptación cuantificables:**
   - $V_{OH(GPIO)} \ge 3.00\,\text{V}$.
   - $V_{CE1(sat)} \le 0.20\,\text{V}$ (Q1 saturado).
   - $V_{EC2(sat)} \le 0.20\,\text{V}$ (Q2 saturado).
   - **$V_{GS} \ge 4.50\,\text{V}$** (Criterio crítico de aceptación; nominal calculado: $4.85\,\text{V} - 4.90\,\text{V}$).
10. **Resultado esperado:** $V_{GS} \approx 4.85\,\text{V}$, excitación plena del canal N del IRLZ44N.
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Registro numérico de tabla nodal firmado (Pendiente).
13. **Estado:** `LISTA PARA BANCO`.
14. **Observaciones y acciones correctivas:** Si $V_{GS} < 4.50\,\text{V}$, verificar la caída de tensión en el riel de 5V de la fuente bajo carga. Si la fuente cae por debajo de $4.8\,\text{V}$, sustituir la fuente por una unidad de 5V 3A de mayor regulación.

---

### ACT-03: Conmutación con Carga Real de Bomba y Estabilidad Térmica
1. **Identificador:** `ACT-03`
2. **Objetivo:** Comprobar la conmutación controlada con la microbomba sumergible JT-DC3W real sumergida en agua, midiendo la caída de tensión en canal ($V_{DS}$), la corriente de régimen ($I_{load}$) y la estabilidad térmica del encapsulado TO-220 tras un ciclo de bombeo máximo continuo ($T = 20\,\text{s}$).
3. **Riesgo cubierto:** F-05 (Sobrecarga de motor, embalamiento térmico del driver, calentamiento destructivo en gabinete cerrado).
4. **Equipo e instrumentos:** Multímetro Digital para corriente DC (escala 10 A), Multímetro para milivoltios DC, Termómetro infrarrojo o termocupla Tipo K calibrada, bandeja plástica antiderrames.
5. **Montaje:** Depósito con agua limpia, microbomba JT-DC3W sumergida impulsando agua en circuito cerrado hacia el depósito. Bomba conectada entre +5V y Drain de Q3.
6. **Precondiciones:** ACT-01 y ACT-02 aprobadas. Prohibido terminantemente encender la microbomba en seco. Prohibido frenar manualmente el rotor para forzar corriente de bloqueo.
7. **Procedimiento:**
   - Registrar la temperatura ambiente del banco ($T_{\text{amb}}$).
   - Iniciar un ciclo de bombeo de 20 segundos continuos (tiempo máximo permitido por watchdog del firmware).
   - Registrar la corriente estable del motor ($I_{\text{bomba}}$).
   - Medir con puntas de prueba directamente sobre las patillas Drain (Pin 2) y Source (Pin 3) la caída $V_{DS}$.
   - Al finalizar los 20 segundos, medir de inmediato la temperatura superficial del encapsulado TO-220 del IRLZ44N ($T_{\text{case}}$) y del diodo 1N5819.
8. **Mediciones a registrar:** $T_{\text{amb}}$, $I_{\text{bomba}}$, $V_{DS}$, $T_{\text{case}}$, $\Delta T = T_{\text{case}} - T_{\text{amb}}$.
9. **Criterios de aceptación cuantificables:**
   - Corriente de régimen: $I_{\text{bomba}} \le 400\,\text{mA}$ en agua.
   - Caída de tensión Drain-Source: **$V_{DS} \le 50\,\text{mV}$** ($0.05\,\text{V}$) a régimen nominal.
   - Elevación térmica: **$\Delta T \le 15^\circ\text{C}$** sobre temperatura ambiente tras 20 s.
   - Temperatura máxima de encapsulado: $T_{\text{case}} \le 45^\circ\text{C}$.
10. **Resultado esperado:** $V_{DS} \approx 12\,\text{mV} - 20\,\text{mV}$, potencia disipada $\approx 5\,\text{mW}$, elevación térmica $\Delta T < 2^\circ\text{C}$ (frío al tacto).
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Registro de prueba térmica y foto termográfica/termocupla (Pendiente).
13. **Estado:** `LISTA PARA BANCO`.
14. **Observaciones y acciones correctivas:** Si $V_{DS} > 50\,\text{mV}$, suspender la prueba inmediatamente; indica que el MOSFET no está saturando adecuadamente o la pieza es un clon fuera de especificación.

---

### ACT-04: Supresión de Transitorios Inductivos (Flyback)
1. **Identificador:** `ACT-04`
2. **Objetivo:** Capturar mediante osciloscopio el pico de fuerza contraelectromotriz (Back-EMF) generado en el borne negativo de la bomba al conmutar de conducción a corte, verificando que el diodo Schottky 1N5819 suprime el transitorio de forma segura por debajo de 6.0 V.
3. **Riesgo cubierto:** F-06 (Destrucción por sobretensión del canal del MOSFET, reinicios espurios de CPU por acoplamiento de ruido en el riel de alimentación).
4. **Equipo e instrumentos:** Osciloscopio Digital con memoria (ancho de banda $\ge 20\,\text{MHz}$), sonda pasiva 10X compensada con cable de masa corto para evitar inductancia espuria en la punta, acoplamiento DC, trigger en flanco de subida en modo disparo único (*Single Shot* o *Normal*).
5. **Montaje:** Conexión de sonda de osciloscopio: Pinza de tierra al Source (GND común); punta de sonda directamente sobre el pin Drain (Pin 2 / Tab) del IRLZ44N (Nodo N6). Bomba sumergible conectada en agua. Diodo 1N5819 soldado en antiparalelo directo a bornes del motor (cátodo a +5V, ánodo a Drain).
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
9. **Criterios de aceptación cuantificables:**
   - Tensión máxima de pico en Drain: **$V_{\text{peak}} \le 6.0\,\text{V}$** (Nominal esperado con diodo Schottky 1N5819: $V_{\text{fuente}} + V_F \approx 5.0\,\text{V} + 0.40\,\text{V} \approx \mathbf{5.40\,\text{V}}$).
   - Ausencia de oscilaciones no amortiguadas que superen 6.0 V.
10. **Resultado esperado:** Disparo garantizado en $+4.50\,\text{V}$, pico recortado limpiamente en $\approx 5.40\,\text{V}$ sin rebotes destructivos.
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Captura de pantalla del osciloscopio (.PNG / .CSV) con cursores de tensión (Pendiente).
13. **Estado:** `LISTA PARA BANCO`.
14. **Observaciones y acciones correctivas:** Si el osciloscopio registra picos $> 12\,\text{V}$, el diodo 1N5819 está dañado, invertido o el cableado del diodo presenta inductancia parásita excesiva (debe ubicarse pegado a los terminales de la bomba).

---

### ACT-05: Sensor de Boya y Enclavamiento de Nivel Seguro (Fail-Safe Interlock)
1. **Identificador:** `ACT-05`
2. **Objetivo:** Comprobar física y lógicamente que la entrada del sensor de nivel por boya magnética (CS-CO058) conmuta limpiamente con su resistencia de pull-up interna, entra en enclavamiento de seguridad (`FAULT / NO_WATER_LEVEL`) en $< 50\,\text{ms}$ al detectar nivel bajo y mantiene la bomba incondicionalmente apagada ante desconexión o rotura de cable.
3. **Riesgo cubierto:** F-01 (Marcha en seco de la bomba, cavitación y quema de sellos), F-12 (Cable de flotador cortado o suelto).
4. **Equipo e instrumentos:** Multímetro Digital (Voltaje DC), depósito de agua de prueba con boya magnética montada vertical/horizontalmente, Monitor Serie a 115200 baudios.
5. **Montaje:** Interruptor de flotador conectado entre el pin digital del microcontrolador (`D4` en Nano / `GPIO 19` en ESP32) y `GND`. Salida de bomba monitoreada con voltímetro.
6. **Precondiciones:** Firmware v1.2.1 cargado en placa. Lazo de control ejecutándose.
7. **Procedimiento:**
   - Escenario 1 (Depósito con agua): Boya arriba. Medir tensión en pin de entrada. Solicitar riego manual. La bomba debe encender.
   - Escenario 2 (Depósito agotado durante riego): Con la bomba activa, descender manualmente la boya. Registrar el tiempo de corte de la bomba y la indicación en display OLED / Monitor Serie.
   - Escenario 3 (Desconexión de cable / Falla de circuito): Con la bomba en reposo y agua presente, desconectar físicamente uno de los cables del flotador de la bornera. Intentar forzar riego mediante pulsador físico y solicitud web.
8. **Mediciones a registrar:** Tensión en pin con boya alta ($V_{\text{in, closed}}$), tensión con boya baja ($V_{\text{in, open}}$), tiempo de respuesta de corte ante caída de boya, estado de salida de bomba.
9. **Criterios de aceptación cuantificables:**
   - Boya alta (agua OK): Contacto cerrado a GND, $V_{\text{in}} \le 0.30\,\text{V}$ (`digitalRead == LOW`).
   - Boya baja (sin agua): Contacto abierto, $V_{\text{in}} \ge 3.00\,\text{V}$ en ESP32 / $\ge 4.50\,\text{V}$ en Nano (`digitalRead == HIGH`).
   - Cable desconectado: Entrada traccionada a nivel alto por `INPUT_PULLUP` $\implies$ estado seguro (`LOW WATER`).
   - Tiempo de detención de bomba ante caída de nivel: $\le 50\,\text{ms}$ (dentro del ciclo de debouncing).
   - Tensión en compuerta de MOSFET ante fallo (MEN-01): **$V_{GS} \le 0.10\,\text{V}$** (Criterio instrumental reproducible unificado con ACT-01; corte seguro garantizado, muy por debajo del umbral mínimo de conducción $V_{GS(th)} \ge 1.0\,\text{V}$).
10. **Resultado esperado:** Bloqueo total e inmediato de la bomba, transición a `FAULT`, advertencia acústica y visual activa.
11. **Resultado real:** PENDIENTE DE MEDICIÓN EN BANCO.
12. **Evidencia:** Captura de log serie y video de respuesta de corte (Pendiente).
13. **Estado:** `LISTA PARA BANCO`.
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
5. **Montaje:** ESP32 DevKit V1 conectado a red local de laboratorio privada (VLAN IoT aislada). Sonda capacitiva mantenida en aire seco (<20% de humedad) para forzar demanda de riego continuo. Bomba monitoreada eléctricamente (se permite mantener la bomba sumergida o sustituida por carga de prueba de $15\,\Omega$, $5\,\text{W}$).
6. **Precondiciones:** ACT-01 a ACT-05 aprobadas. Archivo `Credentials.h` configurado con token seguro de más de 16 caracteres no predeterminado.
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
13. **Estado:** `LISTA PARA BANCO`.
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

| Código de Prueba | Subsistema Evaluado | Estado Actual | Evidencia Requerida | Bloqueo Técnico |
| :---: | :--- | :---: | :--- | :--- |
| **ACT-01** | Estado seguro del driver (corte en boot/reset) | `LISTA PARA BANCO` | Oscilograma de arranque y $V_{GS} \le 0.1\,\text{V}$ | Disponibilidad de protoboard e instrumental |
| **ACT-02** | Excitación compuerta MOSFET ($V_{GS} \ge 4.5\,\text{V}$) | `LISTA PARA BANCO` | Tabla de tensiones nodales bajo carga simulada | Aprobación previa de ACT-01 |
| **ACT-03** | Conmutación real con bomba ($V_{DS} \le 50\,\text{mV}$) | `LISTA PARA BANCO` | Termografía / DMM tras 20 s en agua | Aprobación previa de ACT-01 y ACT-02 |
| **ACT-04** | Supresión Back-EMF con diodo 1N5819 ($V_{\text{peak}} \le 6\,\text{V}$)| `LISTA PARA BANCO` | Oscilograma de disparo único en corte | Aprobación previa de ACT-03 |
| **ACT-05** | Sensor de nivel fail-safe (corte en $\le 50\,\text{ms}$) | `LISTA PARA BANCO` | Log serie y medición de entrada pull-up | Montaje de boya en depósito de prueba |
| **ACT-06** | Límite auto (20s), Watchdog aislado (25s) y HTTP real | `LISTA PARA BANCO` | Cronometraje A1/A2, trazas serie y 7 peticiones cURL | Conexión Wi-Fi de banco en red privada |

> [!CAUTION]
> **Prohibición de Operación Desatendida:**
> El sistema no debe dejarse operando sin supervisión humana continua hasta que las 6 pruebas de la matriz anterior hayan sido ejecutadas, firmadas con fecha y sus resultados se encuentren estrictamente dentro de los criterios de aceptación cuantitativos.
