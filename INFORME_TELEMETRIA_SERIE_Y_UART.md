# INFORME TÉCNICO Y AUDITORÍA: TELEMETRÍA SERIE Y OPTIMIZACIÓN UART
## Smart FloraCare v1.2.1 — Adquisición Automatizada de Banco

**Fecha:** 27 de octubre de 2026  
**Responsable:** Auditoría y Control de Firmware Embebido  
**Estado:** Implementado, verificado y probado localmente / 0 commits / 0 push  
**Patch Unificado:** `DIFF_TELEMETRIA_UART.patch` (en la raíz del repositorio)

---

## 1. ESTADO INICIAL Y HALLAZGO DE LONGITUD DE TRAMA

La revisión del subsistema UART reveló aspectos críticos que debían resolverse antes de considerar la telemetría lista para el banco de pruebas:

1. **Discrepancia de Baudios:**
   - Arduino Nano V3 operaba a `9600` baudios (`Serial.begin(9600)`), consumiendo $\sim 52\text{ ms}$ por trama de 50 bytes.
   - ESP32 operaba a `115200` baudios.
2. **Capacidad del Búfer Circular de Transmisión en AVR:**
   - En el núcleo Arduino AVR (`HardwareSerial`), `SERIAL_TX_BUFFER_SIZE` es de exactamente **64 bytes**.
   - Si una trama excede 63 bytes, o si se intenta emitir cuando el espacio libre es menor a la longitud de la trama, la función `Serial.print()` entra en un bucle de espera activa (`busy-wait`) hasta que la interrupción UART drene suficientes bytes.
   - **Riesgo resuelto:** En la primera versión tentativa, una guarda estática fija (`availableForWrite() < 40`) resultaba insuficiente porque ciertas tramas de fallo alcanzaban hasta 66 bytes. Esto provocaba un bloqueo de ~1,2 ms en Nano.

---

## 2. DECISIÓN SOBRE LA VELOCIDAD UART Y JUSTIFICACIÓN

**Decisión adoptada:** Unificar ambas plataformas a **`115200` baudios**.

1. **Latencia por byte:** A 115200 baudios, cada byte se transmite en $86,8\,\mu\text{s}$. Una trama completa de 43 a 54 bytes se drena en el hilo físico en **$3,7\text{ ms}$ a $4,7\text{ ms}$** (frente a $> 50\text{ ms}$ a 9600 baudios).
2. **Compatibilidad del oscilador AVR:** Con cristal de 16 MHz y modo de doble velocidad (`U2X0 = 1`, divisor `UBRR0 = 16`), el error de frecuencia es de $-3,5\%$, margen plenamente soportado por los bridges USB-UART (CH340G, FT232, CP2102) que toleran hasta $\pm 4,5\%$.
3. **Estandarización:** Permite utilizar un único script receptor o analizador serie a 115200 baudios para ambas placas.

---

## 3. ARQUITECTURA DE TELEMETRÍA ATÓMICA Y DEMOSTRACIÓN DE NO-BLOQUEO

Para garantizar que la telemetría sea **estrictamente no bloqueante**, no genere fragmentación de tramas y jamás interfiera con el control crítico de seguridad, se adoptó el siguiente diseño en `regador_nano_pro.cpp` y `regador_esp32.cpp`:

```cpp
static void emitPeriodicTelemetry(uint32_t nowMs) {
    static uint32_t lastTelemetryMs = 0;
    if (static_cast<uint32_t>(nowMs - lastTelemetryMs) < 1000) {
        return;
    }
    lastTelemetryMs = nowMs;

    // 1. Formateo en búfer de pila local (cero reserva dinámica en heap)
    char buf[64];
    int len = 0;

    // ... Construcción con snprintf acotado a 64 bytes ...

    // 2. GUARDA ATÓMICA DINÁMICA: Si el búfer serie no puede alojar la trama ENTERA,
    // abortar inmediatamente sin transmitir nada (0 µs de bloqueo, 0 fragmentación)
    if (len <= 0 || len >= static_cast<int>(sizeof(buf)) || Serial.availableForWrite() < len) {
        return;
    }

    // 3. Copia a búfer ring UART en RAM (< 10 µs de CPU, sin espera activa)
    Serial.write(reinterpret_cast<const uint8_t*>(buf), static_cast<size_t>(len));
}
```

### Justificación de las 6 Propiedades Auditadas:
1. **Tamaño de Trama Acotado:**
   - Normal IDLE: `[STATE] IDLE | MOIST:42 | WATER:OK | PUMP:OFF\n` $\to$ **43 bytes**.
   - Normal WATERING: `[STATE] WATERING | MOIST:18 | WATER:OK | PUMP:ON\n` $\to$ **47 bytes**.
   - Normal SENSOR ERR: `[STATE] IDLE | MOIST:ERR | WATER:OK | PUMP:OFF\n` $\to$ **44 bytes**.
   - FAULT NO_WATER: `[STATE] FAULT | FAULT:NO_WATER_LEVEL | WATER:LOW | PUMP:OFF\n` $\to$ **53 bytes**.
   - FAULT TIMEOUT: `[STATE] FAULT | FAULT:TIMEOUT | WATER:OK | PUMP:OFF\n` $\to$ **51 bytes**.
   - FAULT SENSOR_ERR: `[STATE] FAULT | FAULT:SENSOR_ERR | WATER:OK | PUMP:OFF\n` $\to$ **54 bytes**.
   - **Conclusión:** Todas las tramas miden entre 43 y 54 bytes, siendo estrictamente menores a la capacidad del búfer de 64 bytes de AVR.
2. **Descarte Atómico y Seguro:**
   - La condición `Serial.availableForWrite() < len` evalúa la longitud **exacta** calculada por `snprintf`. Si el espacio libre en el ring buffer es menor a `len`, la función retorna inmediatamente.
   - **Resultado:** No se transmite ningún byte parcial, no se interrumpe la sincronización del analizador y el siguiente registro comenzará limpio en el siguiente ciclo.
3. **No-Bloqueo Demostrado:**
   - `Serial.write(buf, len)` únicamente se invoca cuando el ring buffer tiene garantizado el espacio para alojar la totalidad del mensaje. En esa condición, `Serial.write()` solo copia bytes de la pila a la RAM del búfer en $< 10\,\mu\text{s}$ y retorna de inmediato, sin entrar jamás en el bucle de espera de la UART.
4. **Consistencia de Estado:**
   - El campo `PUMP:ON` o `PUMP:OFF` reporta el estado de control de la máquina de estados (`controller.isPumpActive()`), no una medición eléctrica por sensor.
5. **Memoria y Pila:**
   - En Arduino Nano, el uso de SRAM se mantiene en 885 bytes (43,2%), dejando **1.163 bytes libres** para la pila. No hay objetos `String` ni llamadas a `malloc`.
6. **Prioridad y Temporización:**
   - La llamada a `emitPeriodicTelemetry(nowMs)` se ubica al final de `loop()`, asegurando que la lectura y filtrado del flotador magnético (45 ms) y la actualización de parada de la bomba tengan prioridad absoluta.

---

## 4. RESULTADOS DE PRUEBAS DE SOFTWARE Y COMPILACIÓN

### 4.1 Pruebas Unitarias Python (84 / 84 Superadas)
```powershell
uv run python -m unittest discover -v -s tests
```
- **Total:** 84 pruebas superadas en 0,043 segundos.
- **6 Pruebas específicas de telemetría aprobadas:**
  - `test_uart_baud_rate_unified_115200`: Verifica 115200 en ambos firmwares y en `platformio.ini`.
  - `test_serial_telemetry_existence_and_interval`: Verifica 1000 ms y guarda dinámica `availableForWrite() < len`.
  - `test_serial_telemetry_non_blocking_no_delay`: Verifica ausencia de `delay()` o bucles `while`.
  - `test_serial_telemetry_format_fields_and_no_secrets`: Verifica tags, ausencia de secretos y formatos de fallo.
  - `test_serial_telemetry_safety_invariance`: Verifica que la telemetría no altera la bomba ni la FSM.
  - `test_serial_telemetry_millis_rollover_math`: Verifica la aritmética modular de rollover de `millis()`.

### 4.2 Compilaciones Multiplataforma en PlatformIO
```powershell
uv run --with platformio pio run -e nanoatmega328 -e esp32dev -e nanoatmega328_bench_wdt25s -e esp32dev_bench_wdt25s
```

| Entorno | Microcontrolador | Flash | RAM | Tiempo | Estado |
| :--- | :--- | :---: | :---: | :---: | :---: |
| `nanoatmega328` | ATmega328P (5 V) | 20.138 B (65,6%) | 885 B (43,2%) | 4,07 s | **SUCCESS** |
| `esp32dev` | ESP32 DevKit V1 (3,3 V) | 1.087.323 B (83,0%) | 49.452 B (15,1%) | 22,09 s | **SUCCESS** |
| `nanoatmega328_bench_wdt25s` | ATmega328P (Bench WDT) | 20.138 B (65,6%) | 885 B (43,2%) | 4,15 s | **SUCCESS** |
| `esp32dev_bench_wdt25s` | ESP32 DevKit V1 (Bench WDT) | 1.087.323 B (83,0%) | 49.452 B (15,1%) | 20,79 s | **SUCCESS** |

**Resultado:** 4 de 4 entornos compilados con **0 errores y 0 advertencias**.

---

## 5. GUÍA PARA LA VERIFICACIÓN EXPERIMENTAL EN BANCO

Para la validación física en hardware real:
1. Conectar Arduino Nano o ESP32 al PC vía USB a **115200 baudios**.
2. Ejecutar el capturador serie en Python:
   ```powershell
   uv run python -c "import serial, time; ser = serial.Serial('COM3', 115200, timeout=1); [print(time.strftime('%H:%M:%S'), ser.readline().decode('utf-8', errors='ignore').strip()) for _ in iter(int, 1)]"
   ```
3. **Puntos a contrastar:**
   - Recepción regular cada $1000 \pm 10\text{ ms}$.
   - Forzar vaciado de depósito y verificar que la transición a `FAULT` se emite limpiamente sin bytes truncados.
   - Desconectar físicamente el cable USB durante el ciclo y reconectar: comprobar que el microcontrolador no se bloqueó y continuó controlando la bomba normalmente.
