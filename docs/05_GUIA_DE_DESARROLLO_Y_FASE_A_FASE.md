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

Antes de insertar los microcontroladores en sus zócalos o aplicar cualquier tensión eléctrica externa, ejecute las siguientes mediciones con un multímetro digital (DMM):

| Código / Punto de Prueba | Medición con DMM | Valor Esperado | Criterio de Aceptación / Rechazo |
| :--- | :--- | :--- | :--- |
| **ACT-01: Aislamiento Riel Potencia** | Resistencia entre riel 5V y GND (placa sin energizar) | Curva de carga capacitiva ascendente estabilizada en $> 5\,\text{k}\Omega$ | **Rechazo inmediato si $R < 10\,\Omega$** o hay continuidad directa (bip corto franco). |
| **ACT-01b: Gate a GND del Driver** | Resistencia entre pin Gate/Input y Source/GND | $10\,\text{k}\Omega \pm 5\%$ (resistencia pull-down) | $< 100\,\Omega$ (gate en corto) o abierta ($> 1\,\text{M}\Omega$) |
| **ACT-02: Polaridad Directa Flyback** | Modo diodo: Sonda roja en Ánodo (Drain), negra en Cátodo (+5V) | $0.25\,\text{V} - 0.40\,\text{V}$ (1N5819 Schottky) | $0.00\,\text{V}$ (diodo perforado) o abierto en directa |
| **ACT-02b: Polaridad Inversa Flyback**| Modo diodo: Sonda roja en Cátodo (+5V), negra en Ánodo (Drain) | `OL` o circuito abierto ($> 2.0\,\text{V}$) | Tensión baja o conducción inversa (diodo invertido) |
| **Continuidad de GND** | Continuidad entre GND de MCU, GND de fuente y retorno de bomba | $0.0\,\Omega - 0.5\,\Omega$ con bip audible | $> 1.0\,\Omega$ o circuito abierto (tierra flotante) |

> [!CAUTION] ¡Peligro de Destrucción por Polaridad Invertida!
> Si el diodo 1N5819 se conecta invertido (cátodo a Drain y ánodo a +5V), al encender el MOSFET se producirá un cortocircuito directo entre 5V y GND a través del diodo, destruyendo el MOSFET de forma instantánea. Verifica la franja blanca/plateada antes de alimentar.

**Criterio de Aceptación Fase 2 (ACT-01 / ACT-02):** Todos los valores registrados dentro de tolerancia. Ausencia demostrable de cortocircuitos de baja impedancia ($R < 10\,\Omega$).

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

- **Sensor de Humedad Capacitivo (ACT-03):**
  - Ejecutar ráfaga de 10 lecturas en serie con el sensor suspendido en el aire seco ($ADC_{\text{aire}}$). Calcular media y desviación estándar $\sigma$.
  - Sumergir la sonda hasta la línea límite en un vaso con agua corriente y registrar 10 lecturas consecutivas ($ADC_{\text{agua}}$).
  - **Criterio Cuantitativo de Aceptación (ACT-03):**
    - En Arduino Nano (10 bits, 5V): $|ADC_{\text{aire}} - ADC_{\text{agua}}| \ge 150\,\text{cuentas}$ con $\sigma \le 5\,\text{cuentas}$.
    - En ESP32 (12 bits muestreado a 3.3V): $|ADC_{\text{aire}} - ADC_{\text{agua}}| \ge 200\,\text{cuentas}$ con $\sigma \le 15\,\text{cuentas}$.
    - La cabecera superior y componentes SMD deben permanecer totalmente secos y sin filtración (barniz íntegro).
- **Flotador de Nivel de Agua (ACT-04):**
  - Elevar la boya manualmente: el monitor serie y display deben indicar `WATER OK` (contacto cerrado a GND, nivel 0 lógico).
  - Dejar caer la boya o desconectar un cable: el display debe indicar `LOW WATER`, la FSM debe conmutar inmediatamente a `FAULT`, el zumbador emitirá tren de pulsos suaves y cualquier solicitud de bomba debe ser rechazada de inmediato.
- **Pulsador Manual de Riego:**
  - Presionar el botón: el LED de prueba conectado a Drain debe conmutar encendido exactamente durante 3 segundos y apagarse limpiamente.

**Criterio de Aceptación Fase 3:** Telemetría visual en OLED correcta, respuestas cuantitativas según ACT-03 ($|\Delta ADC| \ge 150/200$), interbloqueo fail-safe ACT-04 verificado y actuador de prueba conmutando sin bloqueos.

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

**Criterio de Aceptación Fase 4:** Con el depósito lleno y la bomba apagada, no debe observarse ninguna gota saliendo del extremo de la manguera por gravedad.

---

## 📌 Fase 5: Protocolo de Validación en Banco y Pruebas de Fallo

Con la bomba real conectada al circuito hidráulico de prueba y alimentada con su fuente de 5V 2A–3A definitiva, ejecute los siguientes 5 ensayos de estrés supervisados:

### Ensayo 5.1: Prueba de Bloqueo por Depósito Vacío (Interlock Fail-Safe)
- **Procedimiento:** Con el sustrato seco (<20% de humedad), desplace la boya del flotador hacia abajo o desconecte su terminal para simular depósito vacío. Accione el botón manual y envíe una solicitud HTTP/automática.
- **Resultado Esperado:** La bomba **permanece apagada en todo momento**. La FSM se mantiene en `FAULT`, la pantalla indica recarga obligatoria y el buzzer emite señal audible no intrusiva.

### Ensayo 5.2: Prueba de Parada Idempotente por Nivel Óptimo
- **Procedimiento:** Con el sensor en tierra seca, permita que el sistema inicie el ciclo de riego. Mientras la bomba impulsa líquido, vierta agua sobre la sonda hasta alcanzar $\ge 70\%$ de humedad.
- **Resultado Esperado:** En el instante en que el promedio móvil filtrado alcanza el 70%, la bomba se apaga de inmediato (`NORMAL_TARGET_REACHED`), sin sobreimpulsos ni rebotes de conmutación.

### Ensayo 5.3: Prueba de Watchdog de Tiempo Límite (Falla de Caudal)
- **Procedimiento:** Mantenga el sensor al aire libre (humedad < 20%) simulando desprendimiento de la sonda o rotura de manguera. Permita que la bomba arranque.
- **Resultado Esperado:** Al transcurrir exactamente el tiempo de seguridad máximo configurado ($T_{\text{max}} = 20\,\text{s}$), el controlador corta incondicionalmente la alimentación del motor (`MAX_TIMEOUT_EXCEEDED`), entra en `FAULT` e inhibe reintentos inmediatos.

### Ensayo 5.4: Prueba de Periodo de Cooldown (Anti-Ciclado Rápido)
- **Procedimiento:** Tras completar un ciclo de riego de 3 segundos, pulse repetidamente el botón manual de forma consecutiva e insistente.
- **Resultado Esperado:** El sistema ignora todas las pulsaciones consecutivas hasta que transcurre el tiempo de reposo (*cooldown* de 5 segundos), protegiendo la bomba y evitando calentamiento prematuro.

### Ensayo 5.5: Estabilidad Térmica de la Etapa de Potencia (ACT-05)
- **Procedimiento:** Durante un ciclo de bombeo continuo supervisado de duración máxima autorizada ($T = 20\,\text{s}$), mida la temperatura del encapsulado del MOSFET / Módulo Driver y del diodo flyback 1N5819 con termómetro IR o termocupla.
- **Criterio Cuantitativo de Aceptación (ACT-05):**
  - La elevación de temperatura sobre la temperatura ambiente del banco debe ser $\Delta T \le 15^\circ\text{C}$ (temperatura de encapsulado $< 40^\circ\text{C}$ a $25^\circ\text{C}$ ambiente).
  - Caída de tensión entre Drain y Source en saturación: $V_{DS} \le 0.05\,\text{V}$.
  - Ausencia total de olores a sobrecalentamiento o deriva térmica.

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
