# ADR-002: Implementación de Máquina de Estados Finita No Bloqueante

## Estado
Aceptado

## Contexto
En proyectos comunes de aficionados y prototipos rápidos con Arduino, es habitual el empleo de la función bloqueante `delay()`. 

En un sistema de riego para plantas residenciales, el uso de retardos bloqueantes presenta graves riesgos:
1. Durante un `delay(5000)`, el microcontrolador queda ciego: no puede responder a pulsaciones del botón manual, no puede leer el sensor de flotador para detener la bomba si se agota el agua, y no puede refrescar la interfaz de usuario en la pantalla OLED.
2. Si un usuario presiona el botón de emergencia o corte, el sistema no respondería hasta que expire el temporizador bloqueante.

## Decisión
Se prohíbe el uso de `delay()` en el lazo principal de ejecución (`loop()`). Se adopta una **Máquina de Estados Finita (FSM)** basada en diferencias de tiempo con `millis()` (*Non-blocking Cooperative Scheduling*).

## Consecuencias
### Positivas:
- **Respuesta en Tiempo Real:** El sistema responde instantáneamente (< 5 ms) a la pulsación del botón manual o a la activación del flotador de nivel de agua.
- **Seguridad Multitarea:** El watchdog de corte por sobretiempo (25 segundos) se evalúa continuamente en cada ciclo de reloj.
- **Actualización Fluida:** La pantalla OLED y los avisos acústicos operan sin interrupciones ni tirones visuales.

### Negativas / Mitigaciones:
- Se requiere llevar un control explícito de marcas de tiempo (`uint32_t ultimoMuestreo`, `uint32_t tiempoInicioBombeo`). Se resuelve encapsulando las temporizaciones en estructuras modulares.
