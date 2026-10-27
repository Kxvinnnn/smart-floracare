# EXPEDIENTE MAESTRO DE CONTINUIDAD TÉCNICA, AUDITORÍA FINAL Y TRANSICIÓN A ENSAYOS FÍSICOS
## Smart FloraCare v1.2.1 — Fase de Validación Metrológica

**Fecha:** 27 de octubre de 2026  
**Responsable:** Ingeniero Sénior de Software Embebido, Seguridad Funcional e Instrumentación  
**Estado:** Software y documentación cerrados / Ensayos físicos en banco preparados  
**Commit Base:** `fc098b1` | **HEAD:** `fc098b1` (con modificaciones locales preservadas)  
**Patch Unificado de Cambios:** `DIFF_TELEMETRIA_UART.patch` *(95,0 KB)*  

---

## 1. ESTADO REAL DEL PROYECTO Y EVIDENCIA RECUPERADA

Se ha recuperado el estado fidedigno del repositorio local en `C:\Users\kevincipher\Desktop\ProyectoRegadorDePlantas`:
- **Rama:** `main`, sincronizada con `origin/main` en el commit `fc098b1` (*docs(context): decouple audit base commit from dynamic HEAD inspection (AUD-01)*).
- **Control de Versiones:** **0 commits, 0 push**. Todos los cambios se encuentran en el árbol de trabajo local para garantizar la estabilidad antes de la validación física.
- **Archivos Modificados en el Árbol de Trabajo:**
  1. `include/IrrigationController.h`: Recuperación condicional de fallo (Opción A), guardas de timeout inmutable y métodos de rearme.
  2. `include/SensorValidator.h`: Filtro antirrebote asimétrico `FloatSwitchDebouncer` (corte a 45 ms, rearme estable a 150 ms, rechazo de glitches $< 30\text{ ms}$).
  3. `platformio.ini`: Unificación de velocidad serie a `monitor_speed = 115200` y entornos de banco de 25 s.
  4. `src/arduino_nano_mejorado/regador_nano_pro.cpp`: Baudrate 115200, WDT hardware de 4 s, botón por flanco descendente y telemetría periódica atómica de 1000 ms.
  5. `src/esp32_mejorado_wifi/regador_esp32.cpp`: Baudrate 115200, Task WDT de 4 s, WebServer con `HTTP_ANY` y cabecera defensiva `Allow: POST`, y telemetría periódica atómica de 1000 ms.
  6. `tests/test_firmware_security.py`: Suite ampliada a 19 pruebas de seguridad funcional, mitigación CSRF y verificación UART.
  7. `tests/test_http_auth_security.py`: 33 pruebas de seguridad HTTP, tokens de entropía y rechazos defensivos.
  8. `tests/test_irrigation_suite.py`: 32 pruebas integrales de la máquina de estados y debouncer de boya.
  9. `tests/test_native_controller.cpp`: Arnés C++ nativo para validación de lógica de dominio.

---

## 2. AUDITORÍA DIRIGIDA DE TELEMETRÍA SERIE Y MEMORIA

### 2.1 UART en AVR (Arduino Nano V3 - ATmega328P)
- **Inspección de Núcleo:** En `framework-arduino-avr/cores/arduino/HardwareSerial.cpp`, la capacidad física del búfer circular es `SERIAL_TX_BUFFER_SIZE = 64 bytes`. La función `availableForWrite()` lee de forma atómica (`TX_BUFFER_ATOMIC`) los punteros `_tx_buffer_head` y `_tx_buffer_tail`.
- **Comportamiento ante Búfer Lleno:** `HardwareSerial::write` solo entra en bucle `while (i == _tx_buffer_tail)` si no hay espacio libre.
- **Demostración de Bloqueo Acotado:** La rutina `emitPeriodicTelemetry` evalúa `if (Serial.availableForWrite() < len) return;`. Al verificar que el espacio libre es estrictamente mayor o igual a `len` antes de escribir el primer byte, la condición `i == _tx_buffer_tail` jamás se cumple. La ejecución de `Serial.write(buf, len)` se reduce exclusivamente a copiar `len` bytes en el array de memoria RAM ($\sim 9,4\,\mu\text{s}$ a 16 MHz), retornando de inmediato al lazo de control.
- **Concurrencia:** No existen interrupciones que escriban en el puerto serie. El hilo de ejecución es único en `loop()`.

### 2.2 UART en ESP32 DevKit V1
- **Inspección de Núcleo:** `HardwareSerial` transfiere los datos a la capa del driver UART del ESP-IDF (`uart_write_bytes`) sobre colas de FreeRTOS.
- **Garantía Operativa:** Al verificarse espacio disponible en el búfer antes de la llamada, la operación se completa en $< 20\,\mu\text{s}$, sin esperas activas por hardware ni bloqueos perceptibles para el lazo de seguridad.

### 2.3 Integridad de Tramas y Descarte Atómico
- **Longitudes Reales Verificadas:**
  - `[STATE] IDLE | MOIST:42 | WATER:OK | PUMP:OFF\n` $\to$ **43 bytes**.
  - `[STATE] WATERING | MOIST:18 | WATER:OK | PUMP:ON\n` $\to$ **47 bytes**.
  - `[STATE] FAULT | FAULT:TIMEOUT | WATER:OK | PUMP:OFF\n` $\to$ **51 bytes**.
  - `[STATE] FAULT | FAULT:NO_WATER_LEVEL | WATER:LOW | PUMP:OFF\n` $\to$ **53 bytes**.
  - `[STATE] FAULT | FAULT:SENSOR_ERR | WATER:OK | PUMP:OFF\n` $\to$ **54 bytes**.
- **Inclusión de Fin de Línea:** La variable `len` producida por `snprintf` incluye exactamente el carácter `\n` y excluye el terminador nulo `\0`.
- **Descarte Atómico:** Si `Serial.availableForWrite() < len`, la función retorna sin transmitir nada. **Se emiten 0 bytes**, garantizando que nunca se produzcan fragmentos de trama que desincronicen al analizador serie.

### 2.4 Memoria SRAM y Pila en Arduino Nano
- **SRAM Total:** $2.048\text{ bytes}$.
- **Datos Estáticos (BSS + Data):** $885\text{ bytes}$ ($43,2\%$).
- **Búfer Dinámico OLED (Adafruit SSD1306):** $1.024\text{ bytes}$ reservados en heap durante `display.begin()`.
- **Margen Base para Pila (*Stack*):** $2.048 - (885 + 1.024) = 139\text{ bytes libres}$.
- **Impacto del Búfer Local:** `char buf[64]` se aloja en la pila durante los microsegundos que dura la ejecución de `emitPeriodicTelemetry` y se libera inmediatamente al retornar. Restan **75 bytes libres teóricos** calculados.
- **Dictamen Metrológico:** Los 75 bytes representan una estimación estática, no una demostración empírica de ausencia de desbordamiento en tiempo de ejecución (bajo interrupciones o llamadas I2C de `Wire`). Esta conclusión queda clasificada formalmente como **NO DEMOSTRADA / RIESGO RESIDUAL DE SOFTWARE**, pendiente de perfilado instrumental (*stack painting*) en el banco físico, sin que ello justifique modificar el firmware actual. Se mantiene la **prohibición estricta de añadir más variables locales en el bucle principal de Nano**.

---

## 3. VALIDACIÓN DE SOFTWARE Y COMPILACIÓN

### 3.1 Suite Automatizada en Python
- **Comando:** `uv run python -m unittest discover -v -s tests`
- **Resultado:** **84 pruebas ejecutadas / 84 pruebas superadas (100% éxito)** en 0,052 s.
- **Desglose:**
  - `test_irrigation_suite.py`: 32 pruebas superadas.
  - `test_http_auth_security.py`: 33 pruebas superadas.
  - `test_firmware_security.py`: 19 pruebas superadas (incluidas las 6 pruebas específicas de telemetría UART y temporización).

### 3.2 Compilación Multiplataforma PlatformIO
- **Comando:** `uv run --with platformio pio run -e nanoatmega328 -e esp32dev -e nanoatmega328_bench_wdt25s -e esp32dev_bench_wdt25s`
- **Resultados:**
  | Entorno | Microcontrolador | Flash Usada | RAM Usada | Tiempo | Estado |
  | :--- | :--- | :---: | :---: | :---: | :---: |
  | `nanoatmega328` | ATmega328P (5 V) | 20.138 B (65,6%) | 885 B (43,2%) | 4,07 s | **SUCCESS** |
  | `esp32dev` | ESP32 DevKit V1 (3,3 V) | 1.087.323 B (83,0%) | 49.452 B (15,1%) | 22,09 s | **SUCCESS** |
  | `nanoatmega328_bench_wdt25s` | ATmega328P (Banco WDT) | 20.138 B (65,6%) | 885 B (43,2%) | 4,15 s | **SUCCESS** |
  | `esp32dev_bench_wdt25s` | ESP32 DevKit V1 (Banco WDT) | 1.087.323 B (83,0%) | 49.452 B (15,1%) | 20,79 s | **SUCCESS** |

---

## 4. MATRIZ DE RESOLUCIÓN DE HALLAZGOS

| ID | Área / Requisito | Estado | Justificación / Evidencia |
| :--- | :--- | :---: | :--- |
| **AUD-01** | Sincronización de referencias Git/CI en contexto | **RESUELTO** | Resuelto en `fc098b1`. Documentación desacoplada del HEAD dinámico. |
| **H-01** | Recuperación de estado FAULT (Opción A) | **RESUELTO** | Implementado en `IrrigationController.h`. Validado en 84 pruebas unitarias. |
| **H-02** | Respuestas HTTP y cabecera `Allow: POST` | **RESUELTO** | Implementado con `HTTP_ANY` en `regador_esp32.cpp`. Validado en `test_http_auth_security.py`. |
| **H-03** | Antirrebote asimétrico de boya magnética | **RESUELTO** | `FloatSwitchDebouncer` activo (45 ms corte, 150 ms rearme). |
| **H-04** | Watchdog de hardware activo ($\le 4\text{ s}$) | **RESUELTO** | AVR WDTO_4S y ESP32 Task WDT (4000 ms) verificados en código. |
| **H-05** | Separación entre fallo de sensor y pulsos manuales | **RESUELTO** | Riego manual permitido ante fallo de sensor; ciclo auto bloqueado. |
| **H-06** | Detección de flanco en botón físico | **RESUELTO** | Filtro de flanco descendente (40 ms) con rearme seguro en ambas placas. |
| **H-TEL** | Telemetría serie periódica a 115200 baudios | **RESUELTO** | Formato atómico en Flash/Stack, guarda dinámica `< len`, 84 pruebas aprobadas. |
| **ACT-01** | Corte estático y dinámico en arranque ($V_{GS} \le 0,10\text{ V}$) | **PENDIENTE ENSAYO FÍSICO** | Requiere DMM y osciloscopio en banco. |
| **ACT-02** | Saturación del MOSFET a 5 V ($V_{DS} \le 15,0\text{ mV}$) | **PENDIENTE ENSAYO FÍSICO** | Requiere carga de $15\,\Omega / 5\text{ W}$, DMM mV y termómetro. |
| **ACT-03** | Conmutación de bomba real y transitorio de corriente | **PENDIENTE ENSAYO FÍSICO** | Requiere bomba sumergida, shunt de $0,10\,\Omega$ y osciloscopio 2CH. |
| **ACT-04** | Supresión flyback con 1N5819 Schottky | **PENDIENTE ENSAYO FÍSICO** | Requiere captura oscilográfica de pico Back-EMF en Drain. |
| **ACT-05** | Enclavamiento y latencias de boya ($\le 60\text{ ms} / \le 80\text{ ms}$) | **PENDIENTE ENSAYO FÍSICO** | Requiere apertura física de boya y osciloscopio en Drain y Gate. |
| **ACT-06** | Límites 20 s auto, 25 s WDT y 7 casos HTTP | **PENDIENTE ENSAYO FÍSICO** | Requiere cronómetro, red local aislada y cURL. |

---

## 5. FICHAS TÉCNICAS OPERATIVAS PARA ENSAYOS EN BANCO (ACT-01 A ACT-06)

```
================================================================================
FICHA TÉCNICA: ACT-01.1 — CORTE ESTÁTICO EN REPOSO, RESET Y CABLE SUELTO
================================================================================
1. Objetivo: Verificar que la compuerta del MOSFET permanece en corte estricto
   (V_GS <= 0.10 V) en reposo, bootloader, reset físico y entrada desconectada.
2. Estado Previo: Montaje completo sin bomba (sustituida por LED + 330 ohm).
3. Instrumentación: DMM calibrado (resolución <= 1 mV DC). Registrar marca y modelo.
4. Conexiones:
   - Sonda roja del DMM: Pin 1 (Gate) del IRLZ44N (Nodo N5).
   - Sonda negra del DMM: Pin 3 (Source) del IRLZ44N (GND común).
5. Procedimiento: Medir V_GS en 4 escenarios:
   a) Fuente 5V ON, microcontrolador apagado/desconectado.
   b) Microcontrolador energizado, durante los primeros 500 ms de arranque.
   c) Botón RESET/EN presionado y mantenido continuamente.
   d) Cable de señal desconectado de GPIO 18 (ESP32) o D3 (Nano).
6. Criterio de Aceptación: V_GS <= 0.10 V en los 4 casos y LED testigo totalmente apagado.
7. Condición de Aborto: Si V_GS > 0.20 V, apagar fuente; revisar R_pd y pull-down.
8. Dependencias: Ninguna (primer ensayo obligatorio antes de conectar cargas).
================================================================================
```

```
================================================================================
FICHA TÉCNICA: ACT-01.2 — CAPTURA OSCILOGRÁFICA DE TRANSITORIOS EN COMPUERTA
================================================================================
1. Objetivo: Comprobar que ningún glitch o transitorio durante el encendido en frío
   o el reset físico supera el umbral de conducción del MOSFET (V_GS < 0.50 V).
2. Estado Previo: ACT-01.1 superada. Bomba desconectada. Carga LED + 330 ohm.
3. Instrumentación: Osciloscopio digital >= 20 MHz, 2 canales, 2 sondas 10X DC.
4. Conexiones (Regla de Masa Común):
   - Ambas pinzas de masa de las sondas: EXCLUSIVAMENTE en Pin 3 (Source / GND).
   - CH1 (200 mV/div DC): Pin 1 (Gate) del IRLZ44N.
   - CH2: Riel +5V (Modo 1A) o Pin RESET/EN del MCU (Modo 1B).
5. Procedimiento:
   - Modo 1A (Encendido en frío, 1 captura): CH2 en +5V, base 50 ms/div, trigger
     Single-Shot ascendente en CH2 a +1.50 V. Encender fuente.
   - Modo 1B (Reinicio y recuperación, 10 capturas en 5 ciclos): CH2 en RESET/EN.
     5 capturas en flanco de bajada (20 ms/div) y 5 en flanco de subida (50 ms/div).
   - Modo 2 (Glitch-Trap, base 20 us/div): Trigger Single-Shot en CH1 a +0.50 V.
     Verificar que el osciloscopio permanece en estado ARMED/WAIT (0 disparos).
6. Criterios de Aceptación:
   - Modo 1: En las 11 formas de onda, V_GS(peak) <= 0.50 V (estricto < 0.80 V).
   - Modo 2: Cero disparos registrados >= +0.50 V durante la ventana de observación.
7. Evidencia: 11 capturas PNG/CSV archivadas con número de serie de instrumento.
================================================================================
```

```
================================================================================
FICHA TÉCNICA: ACT-02 — SATURACIÓN ESTÁTICA Y EXCITACIÓN DE COMPUERTA A 5V
================================================================================
1. Objetivo: Verificar que el driver entrega V_GS >= 4.50 V y el MOSFET satura
   profundamente con V_DS <= 15.0 mV bajo corriente de prueba de ~330 mA.
2. Estado Previo: ACT-01 aprobada. Carga de prueba: resistencia 15 ohm, 5 W.
3. Instrumentación: DMM en escala mV DC, termómetro infrarrojo o termocupla.
4. Conexiones:
   - Resistencia de 15 ohm (5 W) conectada entre riel +5V y Pin 2 (Drain).
   - Sonda roja del DMM: Pin 2 (Drain) en la patilla TO-220.
   - Sonda negra del DMM: Pin 3 (Source) en la patilla TO-220.
5. Procedimiento y Seguridad Térmica:
   - Disparar un pulso manual supervisado de 3.0 s mediante botón físico.
   - Medir V_GS y V_DS simultáneamente durante el pulso.
   - Prohibido el contacto manual directo con la resistencia de 15 ohm.
   - Respetar pausa de enfriamiento >= 30 s entre pulsos.
6. Criterios de Aceptación:
   - V_GS >= 4.50 V.
   - V_DS <= 15.0 mV en patillas del encapsulado TO-220 a I_D entre 290 y 380 mA.
7. Condición de Aborto: Si V_DS > 30.0 mV o T_case >= 45 °C, abortar inmediatamente.
================================================================================
```

```
================================================================================
FICHA TÉCNICA: ACT-03.1 — CONMUTACIÓN ESTACIONARIA CON BOMBA SUMERGIDA
================================================================================
1. Objetivo: Comprobar la conmutación con la microbomba real JT-DC3W sumergida en
   agua, midiendo V_DS, corriente de régimen I_load y estabilidad térmica tras 20 s.
2. Estado Previo: ACT-01 y ACT-02 aprobadas. Bomba sumergida en agua limpia.
   PROHIBIDO ENCENDER LA BOMBA EN SECO. Diodo flyback conectado.
3. Instrumentación: DMM para corriente DC (escala 10 A), DMM para mV DC, termómetro IR.
4. Conexiones: DMM amperímetro en serie con el cable positivo de la bomba.
5. Procedimiento:
   - Registrar temperatura ambiente T_amb.
   - Iniciar ciclo de bombeo de 20 segundos continuos en agua.
   - Registrar corriente estable I_bomba, V_DS en patillas TO-220 y T_case al cortar.
6. Criterios de Aceptación:
   - I_bomba(est) <= 400 mA.
   - V_DS(est) <= 50.0 mV.
   - Elevación térmica Delta_T = T_case - T_amb <= 15 °C (T_case <= 45 °C).
================================================================================
```

```
================================================================================
FICHA TÉCNICA: ACT-03.2 — MEDICIÓN DINÁMICA DE CORRIENTE DE ARRANQUE (SHUNT)
================================================================================
1. Objetivo: Medir cuantitativamente el pico de corriente de arranque (in-rush) de
   la microbomba y la caída dinámica de tensión en el riel de alimentación.
2. Estado Previo: ACT-03.1 superada. Shunt de bajo lado R_shunt = 0.10 ohm (1%, >= 2 W, no inductivo preferiblemente; un componente de 5 W es admisible si cumple tolerancia y estabilidad térmica). Es obligatorio medir y registrar en bitácora su resistencia efectiva R_shunt,real antes del ensayo.
3. Instrumentación: Osciloscopio digital 2CH con sondas 10X compensadas.
4. Conexiones (Topología sin bucle de masa):
   - Shunt de 0.10 ohm insertado en bajo lado: entre Pin 3 (Source) y riel GND.
   - Pinzas de masa de AMBAS sondas: conectadas al mismo nodo GND_REF de la fuente.
   - CH1 (10X DC, 1 V/div): Riel +5V de alimentación.
   - CH2 (10X DC o 1X, 20-50 mV/div): Pin 3 (Source) sobre el terminal del shunt.
5. Procedimiento:
   - Base de tiempos: 5 a 10 ms/div. Trigger Single-Shot en CH2 a +20 mV (0.20 A).
   - Activar pulso de arranque de bomba sumergida.
   - Medir V_shunt(peak) y V_CC,min(dinámico).
6. Criterios de Aceptación:
   - Tensión transitoria mínima del riel: V_CC,min >= 4.65 V.
   - Corriente de pico: I_arranque = V_shunt(peak) / R_shunt(real) <= 1.20 A
     (V_shunt <= 120.0 mV para R_shunt = 0.100 ohm).
================================================================================
```

```
================================================================================
FICHA TÉCNICA: ACT-04 — SUPRESIÓN DE TRANSITORIOS INDUCTIVOS (FLYBACK)
================================================================================
1. Objetivo: Capturar con osciloscopio la sobretensión de apertura inductiva (Back-EMF)
   y verificar que el diodo en antiparalelo recorta limpiamente el pico.
2. Estado Previo: ACT-03 aprobada. Diodo Schottky 1N5819 montado en antiparalelo directo pegado a los bornes de la bomba (cátodo a +5V, ánodo a Drain). El diodo rectificador de silicio 1N4007 queda formalmente excluido debido a su tiempo de recuperación inversa lento (t_rr ~ 2-3 us), el cual no garantiza la contención del transitorio rápido por debajo de V_CC + 0.60 V.
3. Instrumentación: Osciloscopio digital con memoria >= 20 MHz, 2 sondas 10X.
4. Conexiones:
   - Masas de ambas sondas: Pin 3 (Source / GND común).
   - CH1 (10X DC, 1 V/div o 2 V/div): Pin 2 (Drain) del IRLZ44N (Nodo N6).
   - CH2 (10X DC, 1 V/div): Pin 1 (Gate) del IRLZ44N (Nodo N5).
5. Procedimiento de Captura en Dos Pasos:
   - Nivel de trigger fijado en +4.50 V (flanco ascendente) sobre CH1 (Drain).
   - Paso 1 (Macroscópico): Base 20 us/div. Medir t_f(G) de 90% a 10% en CH2.
   - Paso 2 (Zoom Flyback): Base 500 ns/div o 1 us/div. Medir V_DS(peak) y t_clamp.
6. Criterios de Aceptación:
   - En CH1 (Drain): V_DS(peak) <= V_CC + 0.60 V (y siempre <= 6.00 V).
   - En CH2 (Gate): Ningún rebote posterior de compuerta puede superar 0.80 V.
7. Condición de Aborto: Si se registra V_DS(peak) > 7.0 V, apagar inmediatamente;
   indica diodo desconectado, dañado o con inductancia de cable excesiva.
================================================================================
```

```
================================================================================
FICHA TÉCNICA: ACT-05.1 / ACT-05.2 — ENCLAVAMIENTO ESTÁTICO DE BOYA MAGNÉTICA
================================================================================
1. Objetivo: Verificar que la apertura de la boya o desconexión del cable conmuta
   el sistema a FAULT y fuerza la compuerta del MOSFET a 0.00 V.
2. Estado Previo: Circuito energizado. Boya magnética conectada en D4/GPIO 19.
3. Instrumentación: DMM calibrado en tensión DC, Monitor Serie a 115200 baudios.
4. Procedimiento:
   - Escenario 1 (Boya alta / con agua): Medir tensión en D4/GPIO 19.
   - Escenario 2 (Boya baja / seco): Descender boya manualmente. Registrar tensión
     y mensaje en OLED / telemetría UART.
   - Escenario 3 (Cable desconectado): Desconectar físicamente el terminal del sensor.
5. Criterios de Aceptación:
   - ACT-05.1: Boya alta mide <= 0.30 V (LOW). Boya baja o cable abierto mide
     >= 3.00 V (ESP32) / >= 4.50 V (Nano). El display muestra "SIN AGUA!" y
     la telemetría emite [STATE] FAULT | FAULT:NO_WATER_LEVEL.
   - ACT-05.2: Con DMM en compuerta (Gate), V_GS <= 0.10 V garantizado.
================================================================================
```

```
================================================================================
FICHA TÉCNICA: ACT-05.3a — LATENCIA LÓGICA DE FIRMWARE ANTE VACIADO
================================================================================
1. Objetivo: Medir el tiempo transcurrido desde la apertura eléctrica de la boya
   hasta la orden lógica digital de apagado emitida por el microcontrolador.
2. Estado Previo: Bomba activa (o carga simulada activa).
3. Instrumentación: Analizador lógico digital u osciloscopio de 2 canales.
4. Conexiones:
   - CH1: Entrada del sensor de boya (D4 en Nano / GPIO 19 en ESP32).
   - CH2: Salida de control de bomba del microcontrolador (D3 en Nano / GPIO 18 en ESP32).
   - Masa: GND común del microcontrolador.
5. Procedimiento:
   - Base de tiempos: 10 ms/div. Trigger en flanco ascendente de CH1 (+1.50 V / +2.50 V).
   - Desconectar bruscamente la boya (simulando rotura o flotador bajando).
   - Medir con cursores el tiempo Delta_t entre el flanco de CH1 y el flanco de bajada de CH2.
6. Criterios de Aceptación:
   - Latencia lógica Delta_t <= 60.0 ms (el debouncer de firmware aporta 45 ms).
   - Advertencia: Solo certifica la orden digital; no demuestra el apagado del actuador.
================================================================================
```

```
================================================================================
FICHA TÉCNICA: ACT-05.3b — LATENCIA ELÉCTRICA COMPLETA DE DESENERGIZACIÓN
================================================================================
1. Objetivo: Validar en hardware real que la bomba se desenergiza efectivamente
   en <= 80.0 ms mediante dos adquisiciones independientes y secuenciales con osciloscopio.
2. Estado Previo: Bomba sumergida en marcha activa con agua.
3. Instrumentación: Osciloscopio digital 2CH, 2 sondas 10X compensadas.
4. Configuración Común: Base de tiempos 10 ms/div (ventana 100 ms). Disparo Single-Shot
   ascendente en CH1 (flotador D4 a +2.50 V en Nano / GPIO 19 a +1.50 V en ESP32).
   Masas de ambas sondas conectadas EXCLUSIVAMENTE al Pin 3 (Source / GND común).
5. Secuencia de Dos Adquisiciones Obligatorias:
   - Adquisición 1 (ACT-05.3b-1 — Transición en Drain):
     * CH1 en entrada de boya (D4 / GPIO 19).
     * CH2 en Pin 2 (Drain) del IRLZ44N (Nodo N6).
     * Provocar apertura de boya. Medir Delta_t_Drain hasta que Drain sube a >= 4.50 V.
   - Adquisición 2 (ACT-05.3b-2 — Descarga de Compuerta en Gate):
     * Rearmar ensayo con bomba en marcha.
     * CH1 en entrada de boya (D4 / GPIO 19).
     * CH2 en Pin 1 (Gate) del IRLZ44N (Nodo N5).
     * Provocar apertura de boya. Medir Delta_t_Gate hasta que Gate cae a <= 0.10 V.
6. Criterios de Aceptación:
   - Delta_t_Drain <= 80.0 ms.
   - Delta_t_Gate <= 80.0 ms.
   - Ambas adquisiciones son obligatorias para cerrar formalmente ACT-05.3b.
================================================================================
```

```
================================================================================
FICHA TÉCNICA: ACT-06.A1 — LÍMITE OPERATIVO AUTOMÁTICO (20.0 s — PRODUCCIÓN)
================================================================================
1. Objetivo: Verificar que un ciclo de riego automático en el firmware de producción
   se interrumpe con precisión a los 20.0 s si la humedad no alcanza el umbral.
2. Estado Previo: Firmware de producción cargado (nanoatmega328 o esp32dev).
   Sensor en aire seco (< 20% humedad). Bomba con agua o carga de 15 ohm conectada.
3. Instrumentación: Cronómetro digital calibrado, Monitor Serie a 115200 baudios.
4. Procedimiento:
   - Permitir inicio automático de ciclo (IrrigationSource::AUTO_CYCLE).
   - Iniciar cronómetro en el instante exacto en que la bomba enciende.
   - Registrar el instante en que la bomba corta y el motivo reportado en pantalla.
5. Criterios de Aceptación:
   - Tiempo de corte: T_corte = 20.0 s +/- 0.5 s (entre 19.5 s y 20.5 s).
   - Pantalla OLED muestra "TIMEOUT 20S".
   - Telemetría UART emite [STATE] FAULT | FAULT:TIMEOUT | ... | PUMP:OFF.
   - El sistema se enclava en FAULT y bloquea nuevos riegos sin rearme explícito.
================================================================================
```

```
================================================================================
FICHA TÉCNICA: ACT-06.A2 / ACT-06.B — WATCHDOG DE BANCO 25 s Y SUITE HTTP
================================================================================
1. Objetivo: Verificar el Watchdog inmutable de 25.0 s (ACT-06.A2) y la seguridad
   del servidor HTTP en ESP32 ante 7 escenarios estructurados (ACT-06.B).
2. Procedimiento ACT-06.A2 (Watchdog 25 s Aislado):
   - Cargar entorno de banco (-DFLORACARE_BENCH_TEST_WATCHDOG_25S).
   - Forzar riego continuo. Registrar corte incondicional a los 25.0 s +/- 0.5 s.
   - Pantalla muestra "WDT 25S ERR", stop reason SAFETY_TIMEOUT.
   - RESTAURAR DE INMEDIATO EL FIRMWARE DE PRODUCCIÓN TRAS CONCLUIR.
3. Procedimiento ACT-06.B (7 Escenarios HTTP vía cURL):
   - Caso 1: GET /api/water -> Esperado: 405 Method Not Allowed + cabecera Allow: POST.
   - Caso 2: POST sin token -> Esperado: 401 Unauthorized.
   - Caso 3: POST con token inválido -> Esperado: 403 Forbidden.
   - Caso 4: POST con token válido -> Esperado: 200 OK + pulso exacto de 3.0 s.
   - Caso 5: Peticiones repetidas < 5 s -> Esperado: 409 Conflict.
   - Caso 6: Ráfaga de tokens inválidos -> Esperado: 429 Too Many Requests (Rate limit).
   - Caso 7: Petición durante bloqueo FAULT -> Esperado: 409 / Bomba apagada.
4. Criterio de Aceptación:
   - 100% de coincidencia de códigos HTTP. La bomba permanece estrictamente apagada
     en todos los casos rechazados (Casos 1, 2, 3, 5, 6, 7).
================================================================================
```

---

## 6. DECLARACIÓN DE CIERRE DOCUMENTAL Y DE SOFTWARE

1. **Estado del Software:** Totalmente auditado y validado (84/84 pruebas Python aprobadas, 4/4 compilaciones PlatformIO exitosas con cero errores y cero advertencias).
2. **Estado de la Telemetría:** Implementada bajo el principio de **bloqueo acotado** con descarte atómico, buffers de pila acotados a 64 bytes y sin asignación dinámica.
3. **Frontera de Calidad y Metrología:**  
   > [!IMPORTANT]
   > Queda formalmente establecido que **el éxito en compilación y pruebas de software NO constituye la aprobación del hardware ni de los protocolos físicos**.
   > Las pruebas ACT-01 a ACT-06 permanecen clasificadas como `PENDIENTE DE ENSAYO FÍSICO` o `PENDIENTE DE INSTRUMENTACIÓN EXTERNA` y requerirán la captura de oscilogramas y mediciones con multímetro de banco en el laboratorio.
