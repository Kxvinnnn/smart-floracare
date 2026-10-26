# 📐 01. Arquitectura y Especificación del Sistema

Este documento describe la arquitectura formal, el modelo en capas, la máquina de estados finita (FSM) y los mecanismos de seguridad y tolerancia a fallos del sistema de autorriego **Smart FloraCare**.

---

## 1. Arquitectura de Software en Capas (Layered Architecture)

Siguiendo las mejores prácticas de ingeniería de software embebido para sistemas confiables de misión crítica doméstica, el sistema se organiza en capas desacopladas con dependencias unidireccionales (de arriba hacia abajo):

```text
+--------------------------------------------------------------------------+
| CAPA 5: PRESENTACIÓN Y HMI (Human-Machine Interface)                     |
|  - DisplayView: Renderizado gráfico en OLED SSD1306 (128x64 I2C)         |
|  - AcousticAlert: Avisos auditivos sutiles con buzzer pasivo (PWM/tone)  |
|  - WebAPI (ESP32): Endpoint REST autenticado POST /api/water             |
+--------------------------------------------------------------------------+
                                    ▲
                                    │ eventos / datos filtrados
+--------------------------------------------------------------------------+
| CAPA 4: DOMINIO Y APLICACIÓN                                             |
|  - IrrigationController: FSM centralizada (IDLE, VALIDATING, WATERING,   |
|    STOPPED, FAULT, RECOVERY).                                            |
|  - Safety Interlocks: Watchdog físico anti-inundación (25s), corte por   |
|    boya de nivel bajo, periodo de enfriamiento (cooldown).               |
|  - SensorValidator: Validación estricta de rangos físicos y coherencia.  |
+--------------------------------------------------------------------------+
                                    ▲
                                    │ señales procesadas
+--------------------------------------------------------------------------+
| CAPA 3: SERVICIOS Y FILTRADO DE SEÑAL                                    |
|  - SignalFilter: Sobremuestreo analógico (16x) y descarte de atípicos    |
|  - CalibrationMapper: Conversión inversa ADC -> Humedad % [0 - 100%]     |
+--------------------------------------------------------------------------+
                                    ▲
                                    │ lecturas / comandos de driver
+--------------------------------------------------------------------------+
| CAPA 2: CONTROLADORES DE DISPOSITIVO (DRIVERS)                           |
|  - PumpDriver: Control de potencia MOSFET Logic-Level (IRLZ44N/FQP30N06L)|
|  - CapacitiveSensorDriver: Interfaz analógica libre de electrólisis      |
|  - FloatSwitchDriver: Interfaz digital debounced (50 ms) para flotador   |
|  - ButtonDriver: Antirrebote por software no bloqueante (40 ms)          |
+--------------------------------------------------------------------------+
                                    ▲
                                    │ registros / pines
+--------------------------------------------------------------------------+
| CAPA 1: ABSTRACCIÓN DE HARDWARE (HAL / TARGET)                           |
|  - Arduino AVR (ATmega328P @ 16 MHz, 5V)                                 |
|  - Espressif ESP32 (Xtensa Dual-Core 240 MHz, 3.3V)                      |
+--------------------------------------------------------------------------+
```

---

## 2. Decisiones de Ingeniería y Comparativa de Componentes

### 2.1 Limitaciones del Diseño Tradicional / Proyecto Base
1. **Caída de tensión del TIP120 (Darlington):** El TIP120 presenta un $V_{CE(sat)}$ típico de 0.8 V a 1.2 V. En un circuito alimentado a 5 V, la bomba recibe únicamente ~3.8 V a 4.2 V, reduciendo drásticamente el caudal y disipando calor innecesario en el transistor.
2. **Ausencia de alerta acústica intuitiva:** Una pantalla OLED de 0.96" tiene caracteres de 2 a 3 mm. Para una persona mayor, percatarse del aviso de falta de agua requiere inspeccionar de cerca la planta.
3. **Desgaste por cavitación y funcionamiento en seco:** Si el depósito se vacía, la bomba succiona aire continuamente hasta que el temporizador venza, deteriorando los sellos de la microbomba sumergible.

### 2.2 Decisiones de Arquitectura Adoptadas
1. **MOSFET Canal N Logic-Level (IRLZ44N o FQP30N06L):**
   - Resistencia de conducción ultra baja: $R_{DS(on)} < 0.035\,\Omega$ a $V_{GS} = 3.3\,\text{V}-5\,\text{V}$.
   - Caída de tensión despreciable ($< 0.02\,\text{V}$ a 400 mA): la microbomba opera a máxima eficiencia hidráulica sin requerir disipador de calor.
2. **Interruptor de Flotador de Nivel (Boya Magnética):**
   - Interbloqueo por hardware y software: si el flotador cae indicando depósito vacío, el controlador transiciona inmediatamente a estado de fallo (`FAULT`), impidiendo cualquier activación de la bomba.
3. **Zumbador Pasivo Piezoeléctrico con Atenuación Acústica:**
   - Modulado por señal cuadrada (`tone()`) a frecuencias suaves (800–1000 Hz) con pulsos cortos (100 ms) cada varios segundos.
   - Resistencia serie limitadora de 100–220 Ω para evitar saturación sonora molesta en ambientes interiores.
4. **Pulsador de Riego Manual con Antirrebote:**
   - Permite suministrar un pulso calibrado (3 segundos por defecto) bajo demanda sin desconfigurar la automatización.

---

## 3. Máquina de Estados Finita (FSM) Formal

La lógica de control está centralizada en `FloraCare::IrrigationController` ([include/IrrigationController.h](file:///c:/Users/kevincipher/Desktop/ProyectoRegadorDePlantas/include/IrrigationController.h)). No contiene llamadas bloqueantes y gestiona el tiempo de manera segura frente al desbordamiento de `millis()`.

```mermaid
stateDiagram-v2
    [*] --> IDLE
    
    state IDLE {
        description: Monitoreo en reposo y evaluación de disparadores.
    }
    
    IDLE --> VALIDATING : Solicitud Auto (Hum < 20%) | Manual | Web
    
    state VALIDATING {
        description: Comprobacion de nivel de agua, cooldown y validez del sensor.
    }
    
    VALIDATING --> WATERING : Condiciones Validadas (Nivel OK, Sensor OK, Cooldown OK)
    VALIDATING --> FAULT : Deposito Vacio | Sensor Anomalo
    VALIDATING --> IDLE : Rechazo por Cooldown activo
    
    state WATERING {
        description: Bomba activa. Control estricto de tiempo y lectura continua.
    }
    
    WATERING --> STOPPED : Objetivo Alcanzado (Hum >= 70%) | Pulso Manual Completo
    WATERING --> FAULT : Watchdog Superado (> 25s) | Deposito Vacio en marcha | Falla de Sensor
    
    state STOPPED {
        description: Corte fisico garantizado (idempotente). Inicio de temporizador de enfriamiento.
    }
    
    STOPPED --> IDLE : Transicion inmediata al liberar actuador
    
    state FAULT {
        description: Bloqueo incondicional de bomba. Alarma sonora y visual activa.
    }
    
    FAULT --> RECOVERY : Deposito rellenado Y Condiciones restablecidas
    
    state RECOVERY {
        description: Confirmacion estable de senal (debouncing de rearme).
    }
    
    RECOVERY --> IDLE : Sistema restablecido y listo para operar
```

### Tabla de Transición de Estados

| Estado Origen | Evento / Condición | Estado Destino | Acción de Seguridad Asociada |
| :--- | :--- | :--- | :--- |
| `IDLE` | Solicitud de riego (Auto, Manual, Web) | `VALIDATING` | Captura origen y marca temporal |
| `VALIDATING` | Nivel OK, Sensor OK, Cooldown vencido | `WATERING` | Enciende MOSFET (`digitalWrite(HIGH)`), registra tiempo inicio |
| `VALIDATING` | Flotador abierto (Sin agua) | `FAULT` | Mantiene bomba apagada, código `NO_WATER_LEVEL` |
| `VALIDATING` | En periodo de cooldown (< 5 s) | `IDLE` | Rechaza solicitud sin alterar estado de reposo |
| `WATERING` | Humedad $\ge 70\%$ (modo Auto) | `STOPPED` | Apaga MOSFET, motivo `NORMAL_TARGET_REACHED` |
| `WATERING` | Tiempo $\ge$ 3 s (modo Manual/Web) | `STOPPED` | Apaga MOSFET, motivo `MANUAL_BURST_COMPLETE` |
| `WATERING` | Tiempo activo $\ge$ 25 s (Watchdog) | `FAULT` | Corte incondicional forzado, código `MAX_TIMEOUT_EXCEEDED` |
| `WATERING` | Flotador detecta vacío durante riego | `FAULT` | Corte inmediato, código `NO_WATER_LEVEL` |
| `STOPPED` | Actuador en `LOW` garantizado | `IDLE` | Inicia cooldown y vuelve a monitoreo |
| `FAULT` | Flotador cerrado + condición normalizada | `RECOVERY` | Inicia temporizador de estabilidad de rearme |
| `RECOVERY` | Verificación de estabilidad superada | `IDLE` | Borra código de fallo y restablece monitoreo |

---

## 4. Estrategia de Filtrado de Señal y Validación de Datos

Los sensores capacitivos pueden verse afectados por rizado eléctrico generado por la conmutación del motor o variaciones de carga en la fuente de alimentación:

1. **Sobremuestreo (Oversampling):** Se toman 16 muestras consecutivas separadas por intervalos de 5 ms. Se ordenan y se descartan los extremos (recorte de ruido impulsivo), promediando las muestras centrales.
2. **Histéresis Robusta:**
   - Inicio automático: $\le 20\%$ de humedad de suelo.
   - Detención automática: $\ge 70\%$ de humedad de suelo.
   - Banda muerta del $50\%$ para impedir oscilaciones de encendido/apagado por difusión del agua en el sustrato.
3. **Validación de Rangos Físicos (`SensorValidator`):**
   - Si la lectura analógica cruda del ADC se encuentra fuera del rango de calibración admisible por más de un $15\%$, se cataloga como fallo (`ADC_OUT_OF_BOUNDS` o `DISCONNECTED`), inhibiendo el riego para evitar lecturas erróneas por cable cortado.
4. **Watchdog Absoluto Anti-Inundación:**
   - Límite duro no modificable por software de aplicación de 25 segundos continuos. Ante cualquier congelamiento de lectura, el controlador corta la corriente de la compuerta del MOSFET.
