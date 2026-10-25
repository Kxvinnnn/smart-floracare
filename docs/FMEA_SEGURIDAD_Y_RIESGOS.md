# 🛡️ Análisis Modal de Fallos y Efectos (FMEA - Failure Mode and Effects Analysis)

Este documento aplica la metodología internacional **FMEA** (IEC 60812) para evaluar los riesgos funcionales, mecánicos, eléctricos e hidráulicos del sistema de autorriego, asegurando la integridad física de las plantas y la seguridad eléctrica en el hogar de tu abuela.

---

## 📊 Matriz de Evaluación de Riesgos (FMEA Matrix)

| ID | Subsistema / Modo de Fallo | Efecto Potencial del Fallo | Causa Raíz | S | O | D | **RPN** | Acción Preventiva / Mitigación de Diseño |
| :---: | :--- | :--- | :--- | :---: | :---: | :---: | :---: | :--- |
| **F-01** | **Depósito sin agua (Marcha en seco)** | Desgaste térmico de la microbomba sumergible, posible sobrecalentamiento. | Consumo normal del agua sin recarga oportuna. | 6 | 7 | 2 | **84** | **Sensor de Flotador Físico (CS-CO058):** Deshabilita la bomba por hardware/software antes de encender. Alarma acústica suave por buzzer pasivo. |
| **F-02** | **Efecto Sifón Hidráulico** | Vaciado total del depósito en la maceta por gravedad, ahogando las raíces e inundando el suelo. | Extremo de la manguera colocado por debajo del nivel de agua del depósito. | 9 | 5 | 3 | **135** | **Regla de Instalación Hidráulica:** La salida del tubo de riego debe terminar suspendida en el aire sobre la maceta y el depósito debe ubicarse a nivel o por debajo del tiesto. |
| **F-03** | **Fallo / Desconexión del Sensor de Suelo** | Riego continuo infinito o falta de riego total. | Cable suelto, patilla sulfatada o caída a tierra. | 9 | 3 | 2 | **54** | **Watchdog de Tiempo Máximo (25s):** Corte forzoso absoluto si la bomba opera 25s seguidos. Validación de rango válido de ADC (`100 < ADC < 950`). |
| **F-04** | **Pico Inductivo del Motor (Back-EMF)** | Destrucción de la compuerta del MOSFET, reinicios aleatorios del Arduino. | Corte brusco de corriente en la inductancia del motor de la bomba. | 8 | 6 | 2 | **96** | **Diodo Flyback 1N4007** en antiparalelo directo a bornes de la bomba + condensador cerámico de desacoplo de 100 nF. |
| **F-05** | **Compuerta Flotante del MOSFET** | Bomba activada aleatoriamente durante el flasheo o arranque del microcontrolador. | Pin del microcontrolador en alta impedancia (Hi-Z) durante el booteo. | 7 | 5 | 2 | **70** | **Resistencia de Pull-Down externa de 10 kΩ** fijada rígidamente entre Gate y GND. |
| **F-06** | **Contaminación por Agua en la Electrónica** | Corrosión de pistas SMD del sensor capacitivo v1.2. | Riego salpicando los componentes superiores del sensor. | 7 | 6 | 3 | **126** | **Sellado con barniz acrílico/epoxi** de la cabecera del sensor y funda termorretraíble con pegamento interior. |
| **F-07** | **Aviso Sonoro Perturbador para Adulto Mayor** | Estrés o sobresalto en la abuela por pitido agudo o estridente. | Elección de zumbador activo comercial no regulado. | 5 | 6 | 2 | **60** | **Buzzer Pasivo con Tono Grave (850 Hz)** + resistencia física atenuadora de 220 Ω + pulsos cortos y espaciados cada 6 segundos. |

*Escala de 1 a 10:*  
- **S (Severidad):** Impacto en el sistema/planta (1 = Despreciable, 10 = Muerte de planta / Daño electrónico crítico).  
- **O (Ocurrencia):** Probabilidad de ocurrencia (1 = Muy remota, 10 = Inevitable).  
- **D (Detección):** Dificultad para detectarlo antes del fallo (1 = Detección inmediata, 10 = Indetectable).  
- **RPN (Risk Priority Number):** $S \times O \times D$. Los valores superiores a 80 exigen mitigación prioritaria obligatoria.
