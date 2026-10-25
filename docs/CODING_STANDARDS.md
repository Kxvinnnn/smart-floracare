# 📏 Estándares de Codificación y Calidad de Software Embebido

Este proyecto aplica un subconjunto riguroso de buenas prácticas derivadas de las normas internacionales **MISRA C++:2008** y la **Google C++ Style Guide** adaptadas a microcontroladores de recursos restringidos (AVR / ESP32).

---

## 1. Gestión de Memoria y Rendimiento

1. **Cero Asignación Dinámica en Runtime:**
   - Queda estrictamente prohibido el uso de `malloc()`, `free()`, operadores `new`/`delete` y objetos de la clase dinámica `String` dentro del ciclo `loop()` o funciones llamadas periódicamente.
   - Todo buffer debe tener tamaño fijo conocido en tiempo de compilación (`static const` o `constexpr`).
2. **Uso de Memoria Flash para Cadenas Literales:**
   - Todo texto estático enviado a la pantalla OLED o al puerto serie debe encapsularse en la macro `F()` (ej. `Serial.println(F("OK"))`) para que resida exclusivamente en memoria Flash (PROGMEM) y no consuma la valiosa memoria RAM (2 KB en ATmega328P).
3. **Determinismo Temporal:**
   - Ninguna función de procesamiento debe contener bucles infinitos o esperas activas no acotadas.

---

## 2. Tipado Explícito y Portabilidad (ISO C99 / C++11 `<stdint.h>`)

Se prohíbe el uso de tipos ambiguos dependientes de la arquitectura como `int` o `long` sin cualificar. Se deben emplear siempre los tipos con ancho de bits explícito:

| Tipo Estándar | Rango | Uso Obligatorio en el Proyecto |
| :--- | :--- | :--- |
| `uint8_t` | `0` a `255` | Números de pin, porcentajes de humedad (0-100), contadores de ciclo pequeños. |
| `int8_t` | `-128` a `127` | Valores diferenciales pequeños o temperaturas relativas. |
| `uint16_t` | `0` a `65,535` | Lecturas brutas del convertidor analógico digital (ADC 0-1023 / 0-4095), frecuencias sonoras en Hz. |
| `uint32_t` | `0` a `4,294,967,295` | Marcas de tiempo de reloj milisegundo (`millis()`). Previene desbordamiento en cálculos de tiempo. |
| `bool` | `true` o `false` | Banderas booleanas lógicas de estado. |

---

## 3. Convenciones de Nomenclatura

- **Constantes y Definiciones de Hardware:** `UPPER_SNAKE_CASE` (ej. `PIN_DRIVER_BOMBA`, `TIEMPO_MAXIMO_BOMBEO_MS`).
- **Funciones y Métodos:** `camelCase()` comenzando por un verbo que describa la acción (ej. `leerPorcentajeHumedad()`, `actualizarPantalla()`, `emitirAvisoSuave()`).
- **Variables Locales y Miembros:** `camelCase` conciso y semánticamente representativo (ej. `humedadActual`, `tiempoInicioRiego`).
- **Enumeraciones y Tipos Personalizados:** `PascalCase` con prefijo semántico (ej. `enum class EstadoSistema`).

---

## 4. Arquitectura de Control No Bloqueante

- La función `loop()` principal debe completarse en menos de **10 milisegundos** por iteración.
- Para temporizar tareas, se debe emplear el patrón estándar de resta temporal inmune al desbordamiento de 49.7 días de `millis()`:

```cpp
// Patrón canónico no bloqueante
uint32_t tiempoActual = millis();
if ((uint32_t)(tiempoActual - marcaTiempoAnterior) >= INTERVALO_DESEADO_MS) {
    marcaTiempoAnterior = tiempoActual;
    ejecutarTareaPeriodica();
}
```
