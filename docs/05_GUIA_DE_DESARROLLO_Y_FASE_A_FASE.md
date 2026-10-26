# 🛠️ 05. Guía de Desarrollo, Montaje y Comisionamiento Fase a Fase

Esta guía describe el procedimiento riguroso para construir, probar en frío, verificar eléctricamente, comisionar y desplegar el sistema de autorriego **Smart FloraCare** en sus dos variantes de hardware (**Arduino Nano V3** y **ESP32 DevKit V1**).

---

## 📋 Resumen del Procedimiento de Puesta en Marcha

El protocolo de comisionamiento sigue 6 fases secuenciales e independientes. **No se debe avanzar a la siguiente fase sin haber completado y registrado como APROBADA la anterior.**

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

## 📌 Fase 2: Montaje en Protoboard y Chequeos en Frío (Cold Checks)

Antes de suministrar energía eléctrica a la protoboard, realiza los siguientes ensayos con un Multímetro Digital (DMM) en modo continuidad (bip) y resistencia ($\Omega$).

### 2.1 Conexiones de Circuito por Variante

#### Variante A: Arduino Nano V3 (5V nativo)
- `5V` y `GND` de fuente van al riel de potencia de la protoboard y a los pines `5V` y `GND` del Nano.
- Sensor Capacitivo: `VCC` a `5V`, `GND` a riel común, `AOUT` al pin analógico `A0`.
- Display OLED (0.96" SSD1306): `VCC` a `5V`, `GND` a riel, `SDA` a `A4`, `SCL` a `A5`.
- Driver MOSFET (IRLZ44N o FQP30N06L):
  - Gate (Pin 1): Resistencia en serie de 1 kΩ hacia pin digital `D3`. Resistencia pull-down de 10 kΩ entre Gate y GND.
  - Drain (Pin 2): Conectado al polo negativo (`-`) del conector de la bomba.
  - Source (Pin 3): Conectado sólidamente al riel común de tierra (`GND`).
- Diodo Flyback (1N5819 Schottky): Cátodo (franja plateada) al polo positivo `+5V`, ánodo al polo negativo de la bomba (Drain).
- Flotador de Nivel: Pin digital `D4` a un terminal del interruptor; el otro terminal a `GND` (usa `INPUT_PULLUP`).
- Pulsador Manual: Pin digital `D2` con resistencia de 10 kΩ pull-down externa o configuración directa a `GND` con `INPUT_PULLUP`.
- Buzzer Pasivo: Pin digital `D5` con resistencia de 100–220 Ω en serie hacia un polo del buzzer; otro polo a `GND`.

#### Variante B: ESP32 DevKit V1 (3.3V lógica)
- Fuente externa: `5V` al pin `VIN`/`5V` del ESP32 y al polo positivo del motor. Tierra común (`GND`).
- Sensor Capacitivo: **`VCC` conectado al riel de `3.3V` del ESP32** (no al riel de 5V, para asegurar que la salida no exceda los 3.3V). Salida a `GPIO 34` (ADC1_CH6).
- Display OLED: `VCC` a `3.3V`, `GND` a tierra, `SDA` a `GPIO 21`, `SCL` a `GPIO 22`.
- Driver MOSFET: Gate con resistencia de 1 kΩ hacia `GPIO 18`. Pull-down de 10 kΩ a `GND`.
- Diodo Flyback: 1N5819 en antiparalelo con la bomba (cátodo a +5V, ánodo a Drain).
- Flotador de Nivel: `GPIO 19` a interruptor con pull-up (`INPUT_PULLUP`).
- Pulsador Manual: `GPIO 4` con debouncing no bloqueante.
- Buzzer Pasivo: `GPIO 23` con resistencia limitadora de 220 Ω.

### 2.2 Protocolo de Ensayos en Frío (Sin Conectar Fuente)

#### Protocolo de Desconexión Previa Obligatoria:
Para evitar mediciones erróneas causadas por caminos parásitos o componentes activos:
1. **Desconectar la fuente de alimentación externa de 5V** y desenchufar el cable USB de la computadora.
2. **Retirar el microcontrolador** (Nano o ESP32) de su zócalo o protoboard.
3. **Desconectar el motor de la bomba** (dejar sus cables desconectados temporalmente; la bobina del motor tiene una resistencia interna de $10\,\Omega - 30\,\Omega$ que falsearía la medición del diodo flyback).
4. **Desconectar el sensor de humedad y la pantalla OLED**.

Ejecute las siguientes mediciones con un multímetro digital (DMM) calibrado sobre la placa base/protoboard:

| Código / Punto de Prueba | Medición con DMM | Procedimiento y Dinámica de Medición | Criterio de Aceptación / Rechazo |
| :--- | :--- | :--- | :--- |
| **ACT-01: Aislamiento Riel Potencia** | Resistencia entre riel 5V y GND (placa aislada sin MCU ni módulos) | La corriente del DMM cargará los condensadores de desacople ($100\,\mu\text{F}$ / $100\,\text{nF}$). La lectura iniciará en valores bajos y ascenderá de forma continua durante 2 a 5 segundos hasta estabilizarse en $> 5\,\text{k}\Omega$ (o circuito abierto `OL`). | **Aceptado:** Ascenso dinámico hacia $> 5\,\text{k}\Omega$.<br/>**Rechazo Inmediato:** $R < 10\,\Omega$ estático o continuidad acústica permanente (cortocircuito franco de pista o puente de estaño). |
| **ACT-01b: Gate a GND del Driver** | Resistencia entre pin Gate/Input y Source/GND | Medir resistencia directa con escala de $20\,\text{k}\Omega$. | $10\,\text{k}\Omega \pm 5\%$ (resistencia pull-down de seguridad). Rechazar si $< 100\,\Omega$ o circuito abierto ($> 1\,\text{M}\Omega$). |
| **ACT-02: Polaridad Directa Flyback** | Modo diodo: Sonda roja en Ánodo (Drain), negra en Cátodo (+5V) | **Bomba desconectada**. Mide directamente la unión semiconductora del 1N5819 Schottky. | **$0.25\,\text{V} - 0.40\,\text{V}$**. Rechazar si indica $0.00\,\text{V}$ (diodo perforado en corto) o `OL` (diodo abierto o falso contacto). |
| **ACT-02b: Polaridad Inversa Flyback**| Modo diodo: Sonda roja en Cátodo (+5V), negra en Ánodo (Drain) | **Bomba desconectada**. Verifica polarización inversa del diodo. | `OL` o $> 2.0\,\text{V}$ (bloqueo inverso total). Rechazar si conduce o muestra caída baja (diodo soldado al revés). |
| **Continuidad de GND** | Continuidad entre GND de bornera de alimentación, GND del MCU y Source del MOSFET | Escala de continuidad / resistencia baja. | $0.0\,\Omega - 0.5\,\Omega$ con bip audible claro. Rechazar si $> 1.0\,\Omega$ (tierra flotante). |

> [!CAUTION] ¡Peligro Crítico: Verificación de Diodo Flyback!
> Si el diodo 1N5819 se conecta invertido (cátodo a Drain y ánodo a +5V), al activarse el MOSFET se creará un cortocircuito directo entre el riel de 5V y GND a través del diodo, destruyendo instantáneamente el transistor de conmutación. Confirme visualmente la banda plateada (cátodo) hacia el polo positivo (+5V) antes de avanzar.

**Criterio de Aceptación Fase 2 (ACT-01 / ACT-02):** Todos los valores registrados dentro de tolerancia. Cero cortocircuitos francos y polaridad del diodo 1N5819 100% verificada con la bomba desconectada.

---

## 📌 Fase 3: Energizado Progresivo y Prueba de Subsistemas

El encendido del circuito debe realizarse de forma escalonada, **sin conectar la bomba todavía** (usa un LED de prueba con resistencia de 330 Ω entre +5V y Drain para verificar visualmente la conmutación).

### Paso 3.1: Prueba del Microcontrolador y Rieles de Tensión
1. Conecta el cable USB a la placa.
2. Mide con el DMM en escala de Voltaje DC:
   - En Arduino Nano: Mide entre pin `5V` y `GND`. Debe arrojar entre $4.75\,\text{V}$ y $5.10\,\text{V}$.
   - En ESP32: Mide entre pin `3V3` y `GND`. Debe arrojar entre $3.25\,\text{V}$ y $3.35\,\text{V}$.
3. Comprueba que el regulador de tensión en la placa permanezca a temperatura ambiente (frío al tacto).

### Paso 3.2: Carga del Firmware y Comunicación I2C
1. Compila y flashea el firmware correspondiente mediante PlatformIO:
   - Para Nano: `uvx platformio run -e nanoatmega328 -t upload`
   - Para ESP32: `uvx platformio run -e esp32dev -t upload`
2. La pantalla OLED debe inicializarse en menos de 1 segundo mostrando el logotipo y las lecturas de telemetría.
3. Abre el Monitor Serie a 115200 baudios. Verifica que se impriman los mensajes de inicialización y el estado de la FSM en `IDLE`.

### Paso 3.3: Verificación de Entradas Analógicas y Digitales (ACT-03 y ACT-04)

#### A. Calibración y Sensibilidad del Sensor Capacitivo (ACT-03):
1. **Ensayo de Rango Extremo (Aire vs Agua):**
   - Registrar una ráfaga de 10 lecturas consecutivas con el sensor suspendido en aire seco ($ADC_{\text{aire}}$). Calcular media ($\mu$) y desviación estándar ($\sigma$).
   - Sumergir la sonda verticalmente hasta la línea blanca serigrafiada en un vaso con agua limpia ($ADC_{\text{agua}}$) y registrar 10 lecturas.
   - **Criterios de Sensibilidad Cuantitativa:**
     - En Arduino Nano (10 bits, 5.0V): $|ADC_{\text{aire}} - ADC_{\text{agua}}| \ge 150\,\text{cuentas}$ con $\sigma \le 5\,\text{cuentas}$.
     - En ESP32 (12 bits muestreado a 3.3V): $|ADC_{\text{aire}} - ADC_{\text{agua}}| \ge 200\,\text{cuentas}$ con $\sigma \le 15\,\text{cuentas}$.
2. **Calibración con Sustrato Real de la Planta (Maceta):**
   - Insertar el sensor en la maceta con tierra completamente seca (antes del primer riego). Anotar el valor $ADC_{\text{seco}}$.
   - Regar la maceta a capacidad de campo hasta que comience a drenar por la base. Dejar reposar 15 minutos para que el agua gravitacional escurra y registrar $ADC_{\text{campo}}$.
   - **Mapeo en `CalibrationProfile` de `SensorValidator.h`:**
     La función `SensorValidator::calculateMoisturePercent` realiza la conversión determinista:
     $$\text{Humedad (\%)} = \frac{\text{airRaw} - \text{rawADC}}{\text{airRaw} - \text{waterRaw}} \times 100$$
     Para calibrar el sistema al sustrato real, configure en el perfil de calibración del firmware:
     - `prof.airRaw = ADC_seco` (corresponde al $0\%$ de humedad útil).
     - `prof.waterRaw = ADC_campo` (corresponde al $100\%$ de capacidad de campo).
     - `prof.minDelta = (ADC_seco - ADC_campo) / 2`.
   - Con esta asignación, los umbrales de la FSM en `ControllerConfig`:
     - `autoStartMoisturePct: 20` (inicia el riego cuando el agua disponible cae al $\le 20\%$).
     - `autoStopMoisturePct: 70` (corta el riego al recuperar el $70\%$ de capacidad de campo).
   - Verificar que la electrónica SMD superior y el conector del sensor permanezcan secos y barnizados, sin humedad capilar.

#### B. Interbloqueo Fail-Safe del Flotador de Nivel (ACT-04):
1. **Verificación en Arduino Nano (Pin D4 con `INPUT_PULLUP`):**
   - Boya arriba (depósito con agua): Contacto cerrado a GND $\to$ `digitalRead(4) == LOW` $\to$ `waterLevelOk = true`. OLED muestra `WATER OK`.
   - Boya abajo o cable cortado/desconectado: Entrada en estado flotante traccionada a 5V por pull-up interno $\to$ `digitalRead(4) == HIGH` $\to$ `waterLevelOk = false`. OLED muestra `LOW WATER`, FSM entra en `FAULT`.
   - **Prueba de Bloqueo:** Presionar el botón manual o enviar solicitud de riego. Verificar con voltímetro que el pin D3 (Gate) permanece en $0.00\,\text{V}$.
2. **Verificación en ESP32 (GPIO 19 con `INPUT_PULLUP`):**
   - Repetir exactamente el mismo ensayo. Desconectar la borna del sensor de nivel simulando desprendimiento físico.
   - Comprobar en el Monitor Serie el mensaje `[INTERLOCK] Agua baja detectada. Bombeo bloqueado.` y verificar que el GPIO 18 (Trigger) permanece en $0.00\,\text{V}$.

#### C. Pulsador Manual de Riego:
- Presionar el botón manual una vez: el LED de prueba conectado a Drain debe encenderse exactamente durante 3 segundos y apagarse limpiamente sin trepidaciones.

**Criterio de Aceptación Fase 3:** Telemetría visual en OLED correcta, respuestas cuantitativas según ACT-03 ($|\Delta ADC| \ge 150/200$), calibración de sustrato documentada, interbloqueo fail-safe ACT-04 verificado en ambas plataformas (cero voltios en salida ante fallo) y pulsador conmutando sin bloqueos.

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

#### Distinción Operativa entre Sifón Crítico y Drenaje Residual (ACT-06):
- **Drenaje Capilar Residual Normal:** Al detenerse la bomba, el agua remanente en los últimos centímetros de manguera puede escurrir por gravedad durante **1 a 3 segundos** (volumen insignificante de 1 a 2 ml).
- **Efecto Sifón (Fallo Catastrófico):** El agua continúa saliendo en chorro continuo o goteo constante ininterrumpido después de 5 segundos de haber cortado la alimentación de la bomba.
- **Protocolo de Validación ACT-06:** Llenar el depósito al 100%. Encender la bomba durante 5 segundos y apagar. Tras 5 segundos de corte, secar la boquilla con papel absorbente y dejar reposar durante 30 minutos continuos. El papel absorbente debe permanecer **completamente seco** durante toda la prueba.

**Criterio de Aceptación Fase 4 (ACT-06):** Con el depósito lleno y la bomba apagada, el flujo se interrumpe totalmente tras 3 segundos y se registran cero gotas por gravedad durante 30 minutos de reposo supervisado.

---

## 📌 Fase 5: Protocolo de Validación en Banco y Pruebas de Fallo

Con la bomba real conectada al circuito hidráulico de prueba y alimentada con su fuente de 5V 2A–3A definitiva, ejecute los siguientes 5 ensayos de estrés supervisados:

### Ensayo 5.1: Prueba de Bloqueo por Depósito Vacío (Interlock Fail-Safe)
- **Procedimiento:** Con el sustrato seco (<20% de humedad), desplace la boya del flotador hacia abajo o desconecte su terminal para simular depósito vacío. Accione el botón manual y envíe una solicitud HTTP/automática.
- **Resultado Esperado:** La bomba **permanece apagada en todo momento**. La FSM se mantiene en `FAULT`, la pantalla indica recarga obligatoria y el buzzer emite señal audible no intrusiva.

### Ensayo 5.2: Prueba de Parada Idempotente por Nivel Óptimo
- **Procedimiento:** Con el sensor en tierra seca, permita que el sistema inicie el ciclo de riego. Mientras la bomba impulsa líquido, vierta agua sobre la sonda hasta alcanzar $\ge 70\%$ de humedad.
- **Resultado Esperado:** En el instante en que el promedio móvil filtrado alcanza el 70%, la bomba se apaga de inmediato (`NORMAL_TARGET_REACHED`), sin sobreimpulsos ni rebotes de conmutación.

### Ensayo 5.3: Prueba de Temporizador de Seguridad de Tiempo Límite (Falla de Caudal)
- **Procedimiento:** Mantenga el sensor al aire libre (humedad < 20%) simulando desprendimiento de la sonda o rotura de manguera. Permita que la bomba arranque.
- **Resultado Esperado:** Al transcurrir exactamente el tiempo de seguridad máximo configurado ($T_{\text{max}} = 20\,\text{s}$), el controlador corta incondicionalmente la alimentación del motor (`MAX_TIMEOUT_EXCEEDED`), entra en `FAULT` e inhibe reintentos inmediatos.
- **Naturaleza del Temporizador de Seguridad:**
  Este mecanismo (`maxAutoDurationMs: 20000` y `maxSafetyWatchdogMs: 25000`) es un **temporizador lógico por software** integrado en la máquina de estados dentro de `controller.update(nowMs, ...)`. Depende de que el lazo `loop()` continúe ciclando.
  - Para evitar que un cuelgue del software deje encendida la bomba, el firmware está diseñado de forma estrictamente no bloqueante (cero `delay()` durante el riego y cero bucles `while`).
  - Como salvaguarda física complementaria ante un cuelgue imprevisto de la CPU o bloqueo del bus I2C, la etapa de potencia no inversora (driver NPN+PNP o MOSFET con pull-down) garantiza que si el microcontrolador entra en reset o se bloquea, la señal caerá o será reiniciada por el hardware watchdog interno (WDT de AVR o Task WDT de ESP32), apagando inmediatamente la bomba.

### Ensayo 5.4: Prueba de Periodo de Cooldown (Anti-Ciclado Rápido)
- **Procedimiento:** Tras completar un ciclo de riego de 3 segundos, pulse repetidamente el botón manual de forma consecutiva e insistente.
- **Resultado Esperado:** El sistema ignora todas las pulsaciones consecutivas hasta que transcurre el tiempo de reposo (*cooldown* de 5 segundos), protegiendo la bomba y evitando calentamiento prematuro.

### Ensayo 5.5: Estabilidad Térmica de la Etapa de Potencia (ACT-05)
- **Procedimiento Instrumental:**
  1. Conectar la microbomba real sumergida en agua impulsando caudal hacia la maceta.
  2. Medir y registrar los parámetros eléctricos en régimen permanente con multímetro:
     - Tensión de alimentación en bornes: $V_{\text{fuente}} \approx 5.0\,\text{V}$.
     - Corriente de operación del motor: $I_{\text{bomba}} \approx 200\,\text{mA} - 350\,\text{mA}$.
     - Caída de tensión en saturación entre Drain y Source: $V_{DS}$.
  3. Registrar la temperatura ambiente del banco de pruebas ($T_{\text{amb}}$).
  4. Ejecutar un ciclo continuo de bombeo de duración máxima ($T = 20\,\text{s}$).
  5. Inmediatamente al finalizar los 20 segundos, medir con termómetro infrarrojo o termocupla tipo K la temperatura del encapsulado del MOSFET / Módulo Driver ($T_{\text{case}}$) y del diodo 1N5819.
- **Plantilla de Registro de Banco (ACT-05):**

| Parámetro Instrumental | Valor Medido en Banco | Límite Máximo Admisible | Evaluación |
| :--- | :---: | :---: | :---: |
| **Temperatura Ambiente ($T_{\text{amb}}$)** | $--^\circ\text{C}$ | Referencia ($20^\circ\text{C}-30^\circ\text{C}$) | Nominal |
| **Corriente de Bomba ($I_{\text{bomba}}$)** | $--\,\text{mA}$ | $\le 500\,\text{mA}$ | Conforme |
| **Caída Drain-Source ($V_{DS}$)** | $--\,\text{mV}$ | $\le 50\,\text{mV}$ ($0.05\,\text{V}$) | Saturación |
| **Temp. Encapsulado Driver ($T_{\text{case}}$)** | $--^\circ\text{C}$ | $\le 45^\circ\text{C}$ | Térmico |
| **Elevación Térmica ($\Delta T = T_{\text{case}} - T_{\text{amb}}$)**| $--^\circ\text{C}$ | **$\le 15^\circ\text{C}$** | **ACT-05 OK** |
| **Temp. Diodo Flyback 1N5819** | $--^\circ\text{C}$ | $\le 40^\circ\text{C}$ | Conforme |

**Criterio Cuantitativo de Aceptación (ACT-05):**
- $\Delta T \le 15^\circ\text{C}$ sobre la temperatura ambiente tras 20 segundos continuos a plena carga.
- Caída $V_{DS} \le 50\,\text{mV}$ (indicativa de $R_{DS(on)} \le 0.14\,\Omega$ a $350\,\text{mA}$).
- Ausencia total de olores a sobrecalentamiento, zumbidos anómalos o deriva térmica.

**Criterio de Aceptación Fase 5:** Los 5 ensayos aprobados al 100% con registro documental de mediciones.

---

## 📌 Fase 6: Instalación Definitiva en Maceta de Interior

1. **Alojamiento en Gabinete:**
   - Instala la placa perforada y la pantalla en una caja estanca o de conexiones plásticas.
   - Asegura la sujeción de los cables con prensaestopas o pasamuros para evitar que tirones accidentales desprendan bornes.
2. **Inserción del Sensor:**
   - Realiza un corte vertical en la tierra con una espátula o cuchara pequeña a 5–7 cm del tallo principal.
   - Introduce el sensor verticalmente hasta el límite serigrafiado blanco y presiona la tierra suavemente alrededor para asegurar buen contacto dieléctrico sin bolsas de aire.
3. **Instalación de la Manguera:**
   - Fija la boquilla de salida con una pequeña estaca de madera o plástico apuntando a la base de la raíz, respetando el Air Gap de 2 cm.
4. **Entrega e Instrucciones al Usuario (Abuela):**
   - Explica el significado del pitido suave intermitente: *"Si suena un pitido suave y la pantalla dice AGUA, significa que hay que rellenar el frasco"*.
   - Explica la función del botón manual: *"Si ves la tierra algo seca y quieres darle un chorrito extra de agua, presiona este botón una vez"*.
