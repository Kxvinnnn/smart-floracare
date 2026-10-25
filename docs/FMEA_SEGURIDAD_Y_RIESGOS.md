# 🛡️ Análisis Modal de Fallos y Efectos (FMEA - Failure Mode and Effects Analysis)

Este documento aplica la metodología internacional **FMEA** (norma **IEC 60812**) para evaluar los riesgos funcionales, mecánicos, eléctricos e hidráulicos del sistema **Smart FloraCare**.

> [!IMPORTANT]
> **Criterio de Aceptación de Seguridad:** El RPN ($S \times O \times D$) no es el único factor de decisión. Cualquier riesgo con **Severidad $\ge 8$** (daño catastrófico a la planta, inundación del hogar o daño eléctrico irreversible) exige medidas de mitigación obligatorias y explícitas, independientemente de su detectabilidad.

---

## 📊 Matriz de Evaluación de Riesgos (FMEA Matrix Auditada)

| ID | Modo de Fallo | Efecto Potencial | Causa Raíz | S | O | D | **RPN** | Mitigación Técnica y Estado de Verificación |
| :---: | :--- | :--- | :--- | :---: | :---: | :---: | :---: | :--- |
| **F-01** | **Marcha en seco de la bomba (Depósito vacío)** | Desgaste acelerado, calentamiento y destrucción de la microbomba sumergible. | Evaporación o agotamiento del agua del reservorio. | 6 | 7 | 2 | **84** | **Software:** Enclavamiento inmediato en `IrrigationController` que bloquea la activación y corta la bomba en $<50\,\text{ms}$.<br>**Estado:** `[IMPLEMENTADO Y PROBADO - Test 03]`.<br>**Limitación:** Si el flotador se atasca físicamente abajo o se cortocircuita a GND, el microcontrolador creerá que hay agua. Para redundancia crítica industrial se requeriría un segundo sensor o fusible térmico. |
| **F-02** | **Efecto Sifón Hidráulico Continuo** | Vaciado por gravedad del depósito en la maceta; ahogamiento radicular e inundación en el suelo del hogar. | Extremo del tubo colocado por debajo del nivel hidrostático máximo del depósito. | 9 | 6 | 4 | **216** | **Mitigación Física Obligatoria:** El firmware **NO puede detener el sifonamiento** una vez cebado. Requiere: 1) Reservorio posicionado a nivel o inferior a la maceta; 2) Salida de descarga suspendida en el aire (Air Gap $\ge 2\,\text{cm}$); 3) Perforación sifón anti-vacío.<br>**Estado:** `[REGLA FÍSICA EXTERNA - NO MITIGABLE POR SOFTWARE]`. |
| **F-03** | **Sensor de Suelo Desconectado / Flotante** | Lectura analógica saturada a nivel alto, interpretada erróneamente como sequedad total; bombeo cíclico repetido. | Cable suelto, vibración o conector desconectado. | 8 | 4 | 2 | **64** | **Software:** `SensorValidator::evaluateHealth()` detecta ADC $> \text{maxPhysLimit}$ (desconexión) y transiciona el sistema a `FAULT(SENSOR_MALFUNCTION)`, bloqueando el riego automático.<br>**Estado:** `[IMPLEMENTADO Y PROBADO - Tests 02 y 09]`. |
| **F-04** | **Sensor de Suelo en Cortocircuito a GND** | Lectura analógica saturada a 0V, interpretada erróneamente como saturación acuosa (100%); planta muere por falta de riego. | Humedad corrosiva en cabecera SMD o cruce de terminales. | 8 | 4 | 2 | **64** | **Software:** `SensorValidator` valida ADC $< \text{minPhysLimit}$, bloquea el cálculo y reporta `SHORT_CIRCUIT_OR_LOW`.<br>**Estado:** `[IMPLEMENTADO Y PROBADO - Tests 02 y 09]`. |
| **F-05** | **Bomba Bloqueada o Fuga en Circuito de Riego** | Bomba funcionando sin elevar la humedad del tiesto (manguera suelta o fuera de la maceta). | Desprendimiento mecánico de la manguera de vinilo. | 7 | 5 | 3 | **105** | **Software:** Watchdog temporal estricto (25 segundos máx) gobernado por `IrrigationController`. Corte obligatorio inmutable.<br>**Estado:** `[IMPLEMENTADO Y PROBADO - Test 05]`. |
| **F-06** | **Fuerza Contraelectromotriz (Back-EMF Inductivo)** | Transitorio de alta tensión ($> 40\,\text{V}$) destruye el MOSFET y reinicia el microcontrolador. | Colapso del campo magnético del rotor de la bomba al conmutar a OFF. | 8 | 7 | 2 | **112** | **Hardware:** Diodo Flyback 1N4007 colocado en antiparalelo directo a bornes del motor + condensador cerámico de 100 nF.<br>**Estado:** `[IMPLEMENTADO EN CIRCUITO - REQUIERE VERIFICACIÓN CON OSCILOSCOPIO]`. |
| **F-07** | **Compuerta Flotante del MOSFET en Arranque** | Activación espuria de la bomba durante el reset o flasheo del microcontrolador. | Pines en alta impedancia (Hi-Z) durante la fase de bootloader. | 7 | 5 | 2 | **70** | **Hardware:** Resistencia externa de pull-down de $10\,\text{k}\Omega$ conectada fijamente entre Gate y GND.<br>**Estado:** `[IMPLEMENTADO EN ESQUEMÁTICO - REQUIERE COMPROBACIÓN FÍSICA]`. |
| **F-08** | **Accionamiento Web Indebido o Ataque CSRF (ESP32)** | Vaciado malicioso del depósito o inundación mediante peticiones automáticas de terceros en la LAN. | Acceso no autorizado o precarga de navegador (prefetch GET). | 8 | 5 | 2 | **80** | **Software:** Eliminación de GET `/regar`. Implementación de POST `/api/water` con verificación estricta de token secreto (`API_AUTH_TOKEN`) y rate-limiting de 5s vía `IrrigationController`.<br>**Estado:** `[IMPLEMENTADO Y PROBADO - Test 11]`. |
| **F-09** | **Desbordamiento de Temporizador (`millis()` Rollover)** | Bloqueo o activación infinita tras 49.71 días continuos de operación del microcontrolador. | Desbordamiento del registro `uint32_t` a cero. | 9 | 3 | 2 | **54** | **Software:** Aritmética de diferencias sin signo `(uint32_t)(nowMs - startTimeMs) >= durationMs`. Totalmente inmune al desbordamiento.<br>**Estado:** `[IMPLEMENTADO Y PROBADO - Test 10]`. |

---

## 🔬 Estado y Clasificación de Mitigaciones

1. **Mitigaciones de Software Implementadas y Probadas Automatizadamente:**
   - [x] Enclavamiento de corte por flotador (`Test 03`).
   - [x] Límite máximo de duración y watchdog inmutable de 25s (`Test 05`).
   - [x] Detección de calibraciones matemáticas inválidas (`Test 02`).
   - [x] Detección de fallos de sensor por desconexión / corto (`Test 09`).
   - [x] Aislamiento de método HTTP POST y autorización por token (`Test 11`).
   - [x] Idempotencia de parada y rechazo de solapamientos (`Tests 04 y 06`).
   - [x] Aritmética inmune al rollover de `millis()` (`Test 10`).

2. **Mitigaciones de Hardware Pendientes de Verificación en Banco de Ensayos:**
   - [ ] Inspección con osciloscopio del transitorio Back-EMF con diodo 1N4007.
   - [ ] Medición de tensión de compuerta en Gate del MOSFET durante encendido y apagado en frío.
   - [ ] Comprobación física de estanqueidad de la junta tórica del sensor de flotador CS-CO058.

3. **Limitaciones Críticas No Resolubles por Software (Medidas Físicas Exigidas):**
   - **Sifonamiento:** Ninguna línea de código puede evitar que el agua fluya si el nivel del reservorio queda por encima de la salida de riego. Se exige separación de aire física (*Air Gap*).
   - **Atasco Mecánico del Flotador:** Si el anillo magnético del flotador queda bloqueado mecánicamente en la posición superior por sedimentos o sarro, el microcontrolador no puede inferir que el tanque se vació sin un sensor redundante de respaldo.
