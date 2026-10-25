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

## 3. Fórmula de Conversión Matemática

En el código en C++, la conversión de la lectura cruda `ADC` a porcentaje de humedad (`0%` a `100%`) se realiza mediante:

```cpp
int porcentajeHumedad = map(lecturaADC, VAL_AIRE, VAL_AGUA, 0, 100);
porcentajeHumedad = constrain(porcentajeHumedad, 0, 100);
```

> [!NOTE]
> Observa que `VAL_AIRE` se coloca antes que `VAL_AGUA` en la función `map()`. Como `VAL_AIRE > VAL_AGUA`, la función invierte automáticamente la escala para que más agua signifique mayor porcentaje.

---

## 4. Umbrales Recomendados según la Planta de tu Abuela

En el video de referencia se utiliza un lirio de la paz (*Spathiphyllum* o cuna de Moisés), el cual requiere bastante humedad. Sin embargo, cada tipo de planta tiene requerimientos distintos:

| Tipo de Planta | Umbral de Inicio (Seco) | Umbral de Parada (Húmedo) | Frecuencia de Riego Aprox. |
| :--- | :--- | :--- | :--- |
| **Plantas tropicales (Lirio de paz, Helechos, Pothos)** | **25%** | **70%** | Media / Alta |
| **Plantas de interior estándar (Ficus, Monsteras, Drácenas)** | **20%** | **60%** | Media |
| **Suculentas y Cactus** | **10%** | **35%** | Muy baja (riegos espaciados) |
| **Huerto / Hierbas aromáticas (Albahaca, Menta, Perejil)** | **30%** | **75%** | Alta |

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
