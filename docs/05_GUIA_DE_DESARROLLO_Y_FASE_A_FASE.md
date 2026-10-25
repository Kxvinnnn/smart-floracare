# 🛠️ 05. Guía de Desarrollo y Construcción Fase a Fase

Esta guía describe el proceso ordenado para construir, probar y desplegar el regador automático en el hogar de tu abuela de manera segura y confiable.

---

## 📌 Fase 1: Pruebas en Mesa (Prototipo en Protoboard)

**Objetivo:** Validar que el circuito electrónico y la lógica respondan antes de ensamblar el sistema definitivo.

1. **Montaje en protoboard:**
   - Conecta el Arduino Nano, la pantalla OLED y el sensor de humedad en la protoboard según [docs/03_DIAGRAMA_Y_CONEXIONES.md](file:///c:/Users/kevincipher/Desktop/ProyectoRegadorDePlantas/docs/03_DIAGRAMA_Y_CONEXIONES.md).
   - Conecta un LED con una resistencia de 220 Ω en el pin `D3` (sustituyendo temporalmente la bomba) para verificar cuándo se activa la señal de bombeo sin salpicar agua.
2. **Carga del firmware:**
   - Abre Arduino IDE, instala las librerías `Adafruit SSD1306` y `Adafruit GFX`.
   - Carga el firmware [src/arduino_nano_original/regador_nano.ino](file:///c:/Users/kevincipher/Desktop/ProyectoRegadorDePlantas/src/arduino_nano_original/regador_nano.ino).
3. **Verificación visual:**
   - La pantalla OLED debe encender mostrando el porcentaje de humedad.
   - Al tocar el sensor con un paño húmedo o sumergirlo en un vaso con agua, el porcentaje debe subir en tiempo real y el LED debe apagarse.

---

## 📌 Fase 2: Acondicionamiento y Sellado del Sensor

**Objetivo:** Evitar que el agua del riego o la humedad ambiental dañe los circuitos integrados del sensor con el paso de los meses.

1. **Identificación de la zona sumergible:**
   - El sensor tiene una línea blanca serigrafiada en el medio. Solo la parte inferior de dicha línea debe entrar en contacto con la tierra.
2. **Impermeabilización de la parte superior:**
   - Aplica 2 capas de barniz de uñas transparente o laca acrílica sobre los componentes SMD, resistencias y el chip temporizador situados en el mango superior del sensor.
   - Aplica silicona caliente o tubo termorretráctil con resina adhesiva alrededor del conector de 3 cables.
   - Deja secar durante 2 horas antes de introducirlo en la maceta.

---

## 📌 Fase 3: Fontanería e Hidráulica (Prevención de Sifón)

> [!CAUTION] ¡Peligro de Efecto Sifón!
> Si el depósito de agua se encuentra a mayor altura que la maceta, una vez que la bomba arranque, el agua continuará fluyendo por gravedad **incluso después de que la bomba se haya apagado**, provocando el desborde y ahogamiento de la planta.

### Reglas de Instalación Hidráulica:
1. **Posición del depósito:** Coloca el frasco o botella de agua a la **misma altura o ligeramente por debajo** de la superficie de la maceta.
2. **Bucle anti-sifón:** Asegúrate de que el extremo de la manguera que riega la tierra no quede sumergido dentro del agua de la maceta; debe estar suspendido a 2 cm sobre la tierra.
3. **Filtro de succión:** Agrega una pequeña malla o tela porosa en la entrada de la microbomba sumergible para evitar que suciedad o pelusas atasquen las aspas del motor.

---

## 📌 Fase 4: Ensamble Definitivo y Gabinete

Para que el proyecto luzca prolijo y seguro en la sala o jardín de tu abuela:

1. **Caja protectora:**
   - Puedes usar una caja plástica estanca pequeña de conexiones eléctricas (disponible en ferreterías por ~$2 USD) o una caja impresa en 3D.
   - Realiza un corte rectangular para la pantalla OLED y perforaciones para los cables.
2. **Fijación de conectores:**
   - Utiliza prensaestopas de goma o pasacables para que los cables no sufran tirones accidentales si alguien mueve la maceta.
3. **Soporte de manguera:**
   - Usa una pequeña estaca de madera o plástico para fijar el tubo de vinilo apuntando hacia la base del tallo de la planta.

---

## 📌 Fase 5: Protocolo de Validación y Pruebas de Fallo

Antes de dejar el sistema funcionando autónomamente, ejecuta estas 3 pruebas de esfuerzo:

- [ ] **Prueba de Inundación (Watchdog):** Retira el sensor del suelo para simular sequedad extrema (<20%). Comprueba que la bomba se active y que, a los 25 segundos continuos, el sistema corte la bomba de emergencia (`TIMEOUT`) y dé la alarma.
- [ ] **Prueba de Depósito Vacío:** Si instalaste el sensor de flotador, simula nivel bajo. Verifica que el buzzer emita el pitido intermitente y que la bomba se niegue a arrancar.
- [ ] **Prueba de Riego Real:** Entierra el sensor a 5 cm de la raíz. Deja que riegue la planta. Observa cómo el agua se infiltra y cómo, al llegar al 70%, la bomba se detiene limpiamente.
