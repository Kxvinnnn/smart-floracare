# 🌿 Smart FloraCare: Robust Automated Plant Irrigation System

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Multi--Target-orange?logo=platformio)](https://platformio.org/)
[![CI](https://github.com/Kxvinnnn/smart-floracare/actions/workflows/ci.yml/badge.svg)](https://github.com/Kxvinnnn/smart-floracare/actions/workflows/ci.yml)
[![C++14/17](https://img.shields.io/badge/Standard-C%2B%2B14%2F17-blue?logo=c%2B%2B)](https://en.cppreference.com/)
[![MISRA Subset](https://img.shields.io/badge/MISRA--C%2B%2B-Safe%20Subset-green)](#-estándares-de-codificación-y-calidad)
[![Architecture: Layered](https://img.shields.io/badge/Architecture-5--Layer%20Embedded-brightgreen)](#-arquitectura-del-sistema)
[![FMEA Validated](https://img.shields.io/badge/Safety-IEC%2060812%20FMEA-blueviolet)](#-análisis-de-seguridad-y-riesgos-fmea)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Sistema automatizado de autorriego por goteo con sensado capacitivo, conmutación por MOSFET de nivel lógico, interlocks electromecánicos contra marcha en seco, ergonomía acústica adaptada a adultos mayores y arquitectura de software embebido modular desacoplada.

Diseñado mediante **ingeniería inversa** del diseño de referencia de **Wirecora** (*"This Plant Waters Itself When the Soil Dries"*), optimizado para eliminar caídas de tensión parásitas, desgaste térmico y fallos por cebado en vacío.

---

## 📑 Tabla de Contenidos

- [1. Visión General y Diferenciadores de Ingeniería](#1-visión-general-y-diferenciadores-de-ingeniería)
- [2. Estructura del Repositorio](#2-estructura-del-repositorio)
- [3. Arquitectura del Sistema (5 Capas Embebidas)](#3-arquitectura-del-sistema-5-capas-embebidas)
- [4. Comparativa Técnica de Hardware](#4-comparativa-técnica-de-hardware)
- [5. Análisis de Modos de Fallo y Efectos (FMEA)](#5-análisis-de-modos-de-fallo-y-efectos-fmea)
- [6. Instrucciones de Compilación y Flasheo (PlatformIO / Arduino IDE)](#6-instrucciones-de-compilación-y-flasheo-platformio--arduino-ide)
- [7. Registros de Decisiones de Arquitectura (ADRs)](#7-registros-de-decisiones-de-arquitectura-adrs)
- [8. Licencia y Autor](#8-licencia-y-autor)

---

## 1. Visión General y Diferenciadores de Ingeniería

La mayoría de los proyectos residenciales de riego utilizan transistores bipolares obsoletos, sensores resistivos que se corroen en semanas y bucles de control bloqueantes con `delay()`. **Smart FloraCare** aborda estos problemas con estándares de ingeniería rigurosos:

1. **Eficiencia Eléctrica:** Reemplazo de transistores Darlington (caída típica de 1.2V y calentamiento) por etapas de conmutación de baja pérdida: MOSFET de nivel lógico **IRLZ44N** accionado a 5V en Arduino Nano y **Driver No Inversor Bipolar NPN+PNP** para excitar la compuerta a +4.85V plenos en ESP32.
2. **Protección Física de Actuadores:** Interruptor de flotador electromecánico hermético que bloquea la bomba antes de que pueda operar en seco (prevención de cavitación).
3. **Ergonomía Centrada en Adultos Mayores:** 
   - Pantalla OLED SSD1306 con tipografía de alto contraste.
   - Avisos auditivos discretos de baja frecuencia (850 Hz) mediante zumbador piezoeléctrico pasivo con resistencia atenuadora de 220 Ω (evita sobresaltos).
   - Pulsador táctil físico para disparo de riego manual de 3 segundos bajo demanda.
4. **Software Embebido Determinista:** Cero asignaciones dinámicas en runtime (`malloc`/`String`), variables de ancho explícito (`uint8_t`, `uint16_t`, `uint32_t`) y máquina de estados finita no bloqueante.

---

## 2. Estructura del Repositorio

```text
.
├── README.md                                # Documento de inducción y especificación principal
├── PROJECT_CONTEXT.md                       # Memoria viva de estado y arquitectura del proyecto
├── CHANGELOG.md                             # Historial de versiones (SemVer 2.0.0 / Keep a Changelog)
├── platformio.ini                           # Configuración multi-target (Nano AVR y ESP32)
├── LICENSE                                  # Licencia MIT de código abierto
│
├── include/                                 # Dominio desacoplado de hardware (C++ puro)
│   ├── IrrigationController.h               # FSM centralizada de riego, reglas y watchdogs
│   └── SensorValidator.h                    # Validación de calibración y límites físicos ADC
│
├── docs/                                    # Documentación técnica formal
│   ├── 01_ARQUITECTURA_DEL_SISTEMA.md       # Arquitectura en capas y máquina de estados finita
│   ├── 02_LISTA_DE_MATERIALES_Y_COSTOS.md   # BOM detallado, precios y proveedores homologados
│   ├── 03_DIAGRAMA_Y_CONEXIONES.md          # Diagramas esquemáticos y pinout completo
│   ├── 04_CALIBRACION_Y_CONFIGURACION.md    # Protocolo de calibración matemática del sensor
│   ├── 05_GUIA_DE_DESARROLLO_Y_FASE_A_FASE.md # Protocolo de ensamble, hidráulica y anti-sifón
│   ├── CODING_STANDARDS.md                  # Estándares MISRA C++ y Google C++ para embebidos
│   ├── FMEA_SEGURIDAD_Y_RIESGOS.md          # Matriz de riesgos y modos de fallo (IEC 60812)
│   └── adr/                                 # Architecture Decision Records
│       ├── ADR-001-MOSFET-SELECTION.md      # Selección de MOSFET de canal N
│       ├── ADR-002-NON-BLOCKING-FSM.md      # Máquina de estados cooperativa no bloqueante
│       └── ADR-003-MODULAR-FIRMWARE-ARCHITECTURE.md # Tipado estricto y desacoplamiento
│
├── src/                                     # Código fuente (Firmwares específicos de placa)
│   ├── arduino_nano_original/
│   │   └── regador_nano.ino                 # Firmware de referencia (réplica Wirecora)
│   ├── arduino_nano_mejorado/
│   │   └── regador_nano_pro.cpp             # Firmware industrial PRO (Arduino Nano V3)
│   └── esp32_mejorado_wifi/
│       └── regador_esp32.cpp                # Firmware IoT con Web Dashboard y WiFi (ESP32)
│
└── tests/                                   # Suite de pruebas automatizadas (37 tests reproducibles)
    ├── test_irrigation_suite.py             # 30 pruebas unitarias de dominio, FSM e interlocks
    ├── test_firmware_security.py            # 12 pruebas estáticas de seguridad y robustez
    └── test_http_auth_security.py           # 33 pruebas de autenticación HTTP, CSRF, transporte y controlador
```

---

## 3. Arquitectura del Sistema (5 Capas Embebidas)

```text
+--------------------------------------------------------------------------+
| CAPA 5: PRESENTACIÓN Y HMI (Human-Machine Interface)                     |
|  - DisplayView: Renderizado gráfico en OLED SSD1306 con telemetría clara |
|  - AcousticAlert: Avisos auditivos de baja frecuencia con buzzer pasivo  |
+--------------------------------------------------------------------------+
                                    ▲
+--------------------------------------------------------------------------+
| CAPA 4: DOMINIO Y APLICACIÓN                                             |
|  - Finite State Machine (FSM): MONITORING, WATERING_AUTO, MANUAL, ALERT  |
|  - Safety Interlocks: Watchdog anti-inundación y corte por vaciado       |
+--------------------------------------------------------------------------+
                                    ▲
+--------------------------------------------------------------------------+
| CAPA 3: SERVICIOS Y FILTRADO DE SEÑAL                                    |
|  - SignalFilter: Sobremuestreo analógico (16x) y promediado móvil        |
|  - CalibrationMapper: Conversión inversa no lineal ADC -> Humedad %      |
+--------------------------------------------------------------------------+
                                    ▲
+--------------------------------------------------------------------------+
| CAPA 2: CONTROLADORES DE DISPOSITIVO (DRIVERS)                           |
|  - PumpDriver: Control de potencia MOSFET (IRLZ44N 5V / Driver NPN+PNP)  |
|  - CapacitiveSensorDriver: Interfaz analógica libre de corrosión         |
|  - FloatSwitchDriver: Interfaz de contacto seco magnético                |
+--------------------------------------------------------------------------+
                                    ▲
+--------------------------------------------------------------------------+
| CAPA 1: ABSTRACCIÓN DE HARDWARE (HAL / TARGET)                           |
|  - Arduino AVR (ATmega328P @ 16 MHz) / Espressif ESP32 (Xtensa Dual-Core)|
+--------------------------------------------------------------------------+
```

---

## 4. Comparativa Técnica de Hardware

| Parámetro | Diseño Viral de Referencia | Smart FloraCare (Este Proyecto) |
| :--- | :--- | :--- |
| **Elemento Conmutador** | BJT Darlington TIP120 ($V_{CE(sat)} \approx 1.2V$) | MOSFET Logic-Level IRLZ44N ($R_{DS(on)} < 0.03\,\Omega$) |
| **Pérdida de Tensión en Bomba** | ~24% de caída de tensión | **< 1% de caída de tensión (0 calor)** |
| **Detección de Depósito Vacío** | Solo timeout por software (30s en seco) | **Interruptor de flotador electromecánico hermético** |
| **Notificación a Usuario** | Texto pequeño de 2 mm en pantalla OLED | **OLED + Alarma acústica suave (850 Hz) atenuada** |
| **Control Manual** | No disponible (requiere reinicio o código) | **Pulsador táctil con debouncing e interlock** |
| **Arquitectura de Software** | Archivo monolítico con `delay()` | **Arquitectura en capas, FSM no bloqueante, tipos C++11** |

---

## 5. API REST y Seguridad IoT (Firmware ESP32)

El firmware ESP32 (`src/esp32_mejorado_wifi/regador_esp32.cpp`) incorpora un servidor web embebido y una API REST protegida para monitoreo y control remoto defensivo del riego:

### Rutas HTTP y Métodos Admitidos
| Ruta | Método | Autenticación | Descripción y Respuestas |
| :--- | :---: | :---: | :--- |
| `/` | `GET` | Pública (Lectura) | Dashboard HTML responsivo con telemetría de humedad, nivel de agua, estado de bomba y formulario manual. Responde `200 OK`. |
| `/api/water` | `POST` | **Obligatoria** | Disparo de pulso manual de 3 segundos gobernado por el controlador. Rechaza `GET` con `405 Method Not Allowed`. |

### Mecanismos de Autenticación Admitidos
El endpoint `/api/water` admite dos mecanismos normalizados:
1. **Encabezado HTTP:** `Authorization: Bearer <token>` (Recomendado para integraciones M2M, Home Assistant, scripts cURL).
2. **Cuerpo de Formulario POST:** Parámetro `token=<token>` con `application/x-www-form-urlencoded` (Utilizado por el dashboard web).
*Nota de Seguridad:* Si se envían simultáneamente ambos mecanismos con valores dispares, la petición se rechaza con `400 Bad Request` por ambigüedad.

### Gestión Segura de Credenciales
- **Cero Secretos en Repositorio:** El archivo `src/esp32_mejorado_wifi/Credentials.h` almacena las claves personales y está excluido de Git mediante `.gitignore`. Se provee una plantilla segura en `Credentials.h.example`.
- **Modo Local Autónomo por Defecto:** Si `Credentials.h` no existe o el SSID está vacío, el firmware desactiva el radio Wi-Fi (`WiFi.mode(WIFI_OFF)`), no inicializa el servidor web y opera con seguridad física autónoma.
- **Bloqueo Activo de Tokens Inseguros:** Se rechazan en tiempo de compilación y ejecución (`403 Forbidden`) tokens menores a 16 caracteres o marcadores de posición (`GENERA_UN_TOKEN...`, `CAMBIA_ESTE...`).
- **Defensas en Transporte y Memoria:**
  - Comparación en tiempo constante (`safeConstantTimeEquals`) para mitigar ataques de temporización (*timing attacks*).
  - Límite de longitud de entrada (`MAX_TOKEN_LEN = 128` bytes) para prevenir agotamiento de memoria dinámica (*Heap*).
  - Mitigación de CSRF mediante validación de cabecera `Origin` vinculada a la IP/Host local del ESP32.
  - Retardo defensivo de 200 ms ante fallos de autenticación para mitigar ataques de fuerza bruta en red.

### Códigos de Respuesta HTTP
- `200 OK`: Telemetría HTML entregada satisfactoriamente.
- `303 See Other`: Riego remoto aceptado; redirección al dashboard principal.
- `400 Bad Request`: Longitud de token excedida (> 128 B), esquema inválido o credenciales contradictorias.
- `401 Unauthorized`: Token ausente, incorrecto o cabecera `Authorization` malformada.
- `403 Forbidden`: Token inseguro/placeholder configurado o petición de origen cruzado (CSRF) bloqueada.
- `405 Method Not Allowed`: Uso de métodos no permitidos (ej. `GET` en `/api/water`).
- `409 Conflict`: Solicitud rechazada por el controlador de seguridad (depósito vacío, ciclo activo o cooldown de 5 s).

### Ejemplos de Petición Legítima
```bash
# Vía cURL con cabecera Bearer
curl -X POST http://<IP_ESP32>/api/water \
     -H "Authorization: Bearer MI_TOKEN_SECRETO_SEGURO_32_CHARS"

# Vía cURL con formulario URL-encoded
curl -X POST http://<IP_ESP32>/api/water \
     -d "token=MI_TOKEN_SECRETO_SEGURO_32_CHARS"
```

### Limitaciones de Transporte y Entorno de Red Permitido
> [!CAUTION] Restricción de Despliegue de Red
> El ESP32 opera actualmente con transporte **HTTP en texto plano (puerto 80)**.
> - **Entorno Autorizado:** Red local privada protegida (VLAN de IoT con cifrado WPA2/WPA3).
> - **Prohibición Estricta:** **Queda terminantemente prohibido exponer el puerto 80 del ESP32 a Internet público mediante reenvío de puertos (Port Forwarding / DMZ)**.
> - **Acceso Remoto Seguro:** Para acceder desde el exterior, se debe utilizar una VPN de acceso local (WireGuard / Tailscale) o un proxy inverso de confianza que termine TLS/HTTPS (Nginx / Caddy / Home Assistant Cloud).

---

## 6. Análisis de Seguridad y Riesgos (FMEA)

El proyecto cuenta con una matriz exhaustiva de modos de fallo según la norma **IEC 60812** ([docs/FMEA_SEGURIDAD_Y_RIESGOS.md](docs/FMEA_SEGURIDAD_Y_RIESGOS.md)) que cubre 22 modos de riesgo funcional, mecánico, eléctrico, hidráulico y de ciberseguridad IoT.

---

## 7. Instrucciones de Compilación y Flasheo

### Vía PlatformIO (Recomendado)

```bash
# Clonar el repositorio
git clone https://github.com/Kxvinnnn/smart-floracare.git
cd smart-floracare

# Compilar para Arduino Nano V3 (ATmega328P)
pio run -e nanoatmega328

# Compilar para ESP32 DevKit V1
pio run -e esp32dev

# Subir firmware al microcontrolador conectado por USB
pio run -e nanoatmega328 -t upload  # o -e esp32dev -t upload

# Abrir monitor serial
pio device monitor -b 115200
```

### Vía Arduino IDE (2.x)

1. Abre el archivo `src/arduino_nano_mejorado/regador_nano_pro.cpp` (o crea un `.ino` importándolo).
2. Instala desde el Gestor de Bibliotecas:
   - `Adafruit SSD1306`
   - `Adafruit GFX Library`
3. Selecciona la placa correspondiente (**Arduino Nano** o **ESP32 Dev Module**).
4. Compila y carga el sketch.

### Ejecución de Pruebas Automatizadas

El proyecto cuenta con una suite integral de **75 pruebas automatizadas**, reproducibles y con cero dependencias externas:

```bash
# Ejecutar suite completa con Python 3 (75 pruebas: unitarias, integración y seguridad)
python -m unittest discover -v -s tests
```

- **30 Pruebas:** Máquina de estados (FSM), calibración matemática, temporización no bloqueante e interlocks (`test_irrigation_suite.py`).
- **12 Pruebas:** Seguridad estática, mitigación de fallbacks hardcodeados, tiempo constante y cabeceras (`test_firmware_security.py`).
- **33 Pruebas:** Autenticación HTTP, transporte, mitigación CSRF, límite de entradas e integridad del controlador (`test_http_auth_security.py`).

> [!IMPORTANT] Estado de Validación Física
> La suite de pruebas de software valida la lógica de la FSM, la resiliencia a fallos y la seguridad de API. La validación física del prototipo en banco permanece **PENDIENTE** hasta ejecutar los Cold Checks, mediciones de $V_{GS}$, caída $V_{DS}$ y pruebas de bombeo instrumental detalladas en [`docs/05_GUIA_DE_DESARROLLO_Y_FASE_A_FASE.md`](docs/05_GUIA_DE_DESARROLLO_Y_FASE_A_FASE.md).

---

## 8. Registros de Decisiones de Arquitectura (ADRs)

- [ADR-001: Selección y Especificación de la Etapa de Conmutación de Bomba](docs/adr/ADR-001-MOSFET-SELECTION.md)
- [ADR-002: Implementación de Máquina de Estados Finita No Bloqueante](docs/adr/ADR-002-NON-BLOCKING-FSM.md)
- [ADR-003: Arquitectura Modular de Firmware y Tipado Explícito](docs/adr/ADR-003-MODULAR-FIRMWARE-ARCHITECTURE.md)

---

## 9. Licencia y Autor

Desarrollado con altos estándares de ingeniería por **Kevin ([@Kxvinnnn](https://github.com/Kxvinnnn))**.  
Distribuido bajo la **[Licencia MIT](LICENSE)**. Siéntete libre de construirlo, mejorarlo o desplegarlo en tu hogar.
