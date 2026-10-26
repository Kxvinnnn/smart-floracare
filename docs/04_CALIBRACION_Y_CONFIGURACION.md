# 🎯 04. Protocolo de Calibración y Ajuste de Umbrales

El sensor capacitivo de humedad mide los cambios en la constante dieléctrica del medio circundante. A diferencia de los sensores resistivos de dos puntas metálicas, este sensor no sufre corrosión galvánica, pero **requiere ser calibrado para cada sensor individual**.

---

## 1. Comportamiento Inverso de la Tensión

Una característica fundamental del sensor capacitivo v1.2 es que su salida analógica es **inversamente proporcional a la humedad**:

- **Aire completamente seco (0% humedad):** El valor analógico ADC es **MÁXIMO** (típicamente entre `520` y `650` en Arduino Nano de 10 bits, o `2800` a `3200` en ESP32 de 12 bits).
- **Sumergido en un vaso con agua (100% humedad):** El valor analógico ADC es **MÍNIMO** (típicamente entre `240` y `310` en Arduino Nano, o `1100` a `1500` en ESP32).

---

## 2. Procedimiento de Calibración Paso a Paso

### Paso 1: Medición en Aire (0% Humedad)
1. Conecta el sensor al Arduino según el diagrama.
2. Limpia y seca completamente la placa del sensor con un paño seco.
3. Sostén el sensor en el aire sin tocar la placa con los dedos (para no alterar la capacitancia).
4. Anota el valor leído en el Monitor Serie: este será tu `VAL_AIRE`.
   - *Ejemplo típico en Arduino Nano:* `580`

### Paso 2: Medición en Agua (100% Humedad)
1. Llena un vaso con agua corriente limpia.
2. Sumerge la pala del sensor **únicamente hasta la línea blanca transversal de advertencia** (¡nunca sumerjas la parte superior donde van los componentes electrónicos!).
3. Espera 10 segundos a que la lectura se estabilice.
4. Anota el valor en el Monitor Serie: este será tu `VAL_AGUA`.
   - *Ejemplo típico en Arduino Nano:* `270`

---

## 3. Fórmula de Conversión Matemática y Validación Defensiva

En versiones previas se utilizaba una simple función `map()` sin comprobación previa. En la arquitectura actual (`SensorValidator`), la conversión se efectúa mediante una rutina defensiva que previene divisiones por cero y detecta anomalías de hardware:

```cpp
// Verificación matemática de la calibración
if (prof.airRaw <= prof.waterRaw || (prof.airRaw - prof.waterRaw) < prof.minDelta) {
    return SensorHealth::INVALID_CALIBRATION;
}

// Detección de fallos físicos en el cableado
if (rawAdc < prof.minPhysLimit) {
    return SensorHealth::SHORT_CIRCUIT_OR_LOW; // Posible cortocircuito a GND
}
if (rawAdc > prof.maxPhysLimit) {
    return SensorHealth::DISCONNECTED_OR_HIGH; // Posible desconexión / cable suelto
}

// Conversión segura con protección de rango
int32_t deltaSpan = static_cast<int32_t>(prof.airRaw) - static_cast<int32_t>(prof.waterRaw);
int32_t diff = static_cast<int32_t>(prof.airRaw) - static_cast<int32_t>(rawAdc);
int32_t percent = (diff * 100) / deltaSpan;
outPercent = static_cast<uint8_t>(constrain(percent, 0, 100));
```

> [!WARNING] Protección contra Fallos de Sensor
> Si un sensor se desconecta, el valor ADC puede flotar cerca del voltaje de referencia ($1023$ en Nano, $4095$ en ESP32). Un algoritmo ingenuo calcularía un porcentaje negativo que, tras aplicar `constrain()`, se convertiría en $0\%$ (suelo completamente seco), provocando que la bomba se active innecesariamente hasta inundar la planta. Con `SensorValidator`, este caso se identifica como `DISCONNECTED_OR_HIGH` y el controlador **se niega a regar**, entrando en modo de alerta segura.

---

## 4. Umbrales Recomendados según la Planta y Perfiles Activos en Firmware

En el video de referencia se utiliza un lirio de la paz (*Spathiphyllum* o cuna de Moisés), el cual requiere bastante humedad. Sin embargo, cada tipo de planta tiene requerimientos distintos:

| Tipo de Planta | Umbral de Inicio (Seco) | Umbral de Parada (Húmedo) | Frecuencia de Riego Aprox. |
| :--- | :--- | :--- | :--- |
| **Plantas tropicales (Lirio de paz, Helechos, Pothos)** | **25%** | **70%** | Media / Alta |
| **Plantas de interior estándar (Ficus, Monsteras, Drácenas)** | **20%** | **60%** | Media |
| **Suculentas y Cactus** | **10%** | **35%** | Muy baja (riegos espaciados) |
| **Huerto / Hierbas aromáticas (Albahaca, Menta, Perejil)** | **30%** | **75%** | Alta |

### 4.1 Configuración Predeterminada Activa en Código

Para evitar confusiones entre recomendaciones agronómicas y valores predeterminados en el firmware:
- **Firmware ESP32 (`regador_esp32.cpp`):** Viene precargado con el **Perfil Tropical** (`autoStartMoisturePct = 25%`, `autoStopMoisturePct = 70%`), ideal para la maceta de Lirio de la Paz (*Spathiphyllum*) de la abuela.
- **Firmware Arduino Nano (`regador_nano_pro.cpp`):** Viene precargado con el **Perfil Estándar de Saturación Profunda** (`autoStartMoisturePct = 20%`, `autoStopMoisturePct = 70%`), asegurando menor frecuencia de arranque y mayor penetración hídrica en sustratos densos.
- **Temporizaciones de seguridad (Ambos firmwares):** Duración máxima por ciclo de riego automático = `20 s`; Watchdog lógico independiente = `25 s`; Tiempo de cooldown posterior al riego = `5 s`.
- **Micropausas de muestreo ADC (16x Oversampling):** 
  - Arduino Nano: $250\,\mu\text{s}$ por muestra (`delayMicroseconds(250)`), acumulando $4.0\,\text{ms}$ por ciclo de lectura.
  - ESP32: $200\,\mu\text{s}$ por muestra (`delayMicroseconds(200)`), acumulando $3.2\,\text{ms}$ por ciclo de lectura. Ambas pausas son transparentes para la FSM y no bloquean el lazo periódico.

---

## 5. Código de Calibración Rápida (Sketch de Prueba)

Carga este pequeño sketch antes de instalar el sistema definitivo para verificar tus valores exactos:

```cpp
// Sketch de Calibracion Rapida para Sensor de Suelo
const int PIN_SENSOR = A0;

void setup() {
  Serial.begin(9600);
  Serial.println("--- INICIANDO CALIBRACION ---");
  Serial.println("1. Lee el valor en el aire (seco).");
  Serial.println("2. Introduce en agua hasta la linea (humedo).");
}

void loop() {
  int suma = 0;
  for(int i = 0; i < 10; i++) {
    suma += analogRead(PIN_SENSOR);
    delay(20);
  }
  int valorPromedio = suma / 10;

  Serial.print("Lectura Cruda ADC (A0): ");
  Serial.println(valorPromedio);
  delay(500);
}
```
