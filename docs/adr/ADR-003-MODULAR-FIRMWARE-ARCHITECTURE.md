# ADR-003: Arquitectura Modular de Firmware y Tipado Explícito

## Estado
Aceptado

## Contexto
El código para microcontroladores AVR (ATmega328P con 2 KB de SRAM y 32 KB de Flash) y Xtensa/ESP32 suele sufrir de problemas comunes en proyectos que crecen sin orden:
- Dependencia masiva de variables globales no controladas.
- Fragmentación de memoria por uso de `String` dinámico en heap.
- Tipos de datos primitivos ambiguos (`int` puede ser de 16 bits en AVR y de 32 bits en ESP32/ARM, generando desbordamientos imprevistos al calcular tiempos con `millis()`).

## Decisión
1. Se adopta una arquitectura modular en capas: Hardware/HAL, Drivers de Periféricos, Filtros de Señal, Controlador de Dominio (FSM) y Capa de Presentación.
2. Se exige el uso estricto del estándar C99/C++11 `<stdint.h>`:
   - `uint8_t` para pines, contadores de muestra y porcentajes (0-100).
   - `uint16_t` para lecturas de conversión analógica-digital (ADC 0-1023 o 0-4095).
   - `uint32_t` para marcas de tiempo provenientes de `millis()`.
3. Se prohíbe el uso de la clase dinámica `String` en la lógica de control; se emplean cadenas literales en memoria Flash mediante el macro `F()` y buffers estáticos de caracteres (`char[]`).

## Consecuencias
### Positivas:
- **Portabilidad:** El código es 100% portable entre Arduino Nano (AVR), ESP32 (Xtensa), STM32 (ARM Cortex) o Raspberry Pi Pico (RP2040).
- **Estabilidad de Memoria:** Riesgo cero de fragmentación de memoria en el heap; uso determinista y predecible de la memoria estática (SRAM).
- **Mantenibilidad:** Facilidad para realizar pruebas unitarias sobre los módulos de lógica pura.
