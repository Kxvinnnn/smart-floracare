/*
  =============================================================================
  PROYECTO: Regador Automático Inteligente (Versión Arduino Nano Fiel al Video)
  AUTOR BASE: Wirecora (Ingeniería Inversa) / Adaptación y Robustez
  PLACA: Arduino Nano (ATmega328P)
  =============================================================================
  COMPONENTES:
    - Sensor Capacitivo de Humedad v1.2 -> Pin A0
    - Pantalla OLED I2C SSD1306 (128x64) -> SDA (A4), SCL (A5)
    - Conmutador Bomba (TIP120 / MOSFET) -> Pin D3
  =============================================================================
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIGURACION DE PANTALLA OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C // Dirección I2C típica (0x3C o 0x3D)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- ASIGNACION DE PINES ---
const int PIN_SENSOR_HUMEDAD = A0;
const int PIN_DRIVER_BOMBA   = 3; // Salida digital / PWM al Gate del MOSFET o Base TIP120

// --- PARAMETROS DE CALIBRACION (Ajustar según docs/04_CALIBRACION) ---
const int VALOR_CALIBRACION_AIRE = 580; // Lectura analógica en seco total (0%)
const int VALOR_CALIBRACION_AGUA = 270; // Lectura analógica sumergido en agua (100%)

// --- UMBRALES DE CONTROL (%) ---
const int UMBRAL_RIEGO_INICIO = 20; // Si la humedad cae por debajo de este %, arranca la bomba
const int UMBRAL_RIEGO_FIN    = 70; // Cuando la humedad alcanza este %, apaga la bomba

// --- TIEMPOS Y PROTECCIONES (Milisegundos) ---
const unsigned long INTERVALO_LECTURA_SENSOR = 1000;      // Frecuencia de muestreo (1 segundo)
const unsigned long TIEMPO_MAXIMO_BOMBEO     = 25000;     // 25 segundos máx para evitar inundación / vaciado

// --- VARIABLES DE ESTADO ---
enum EstadoSistema {
  ESTADO_MONITOREO,
  ESTADO_REGANDO,
  ESTADO_ERROR_REFILL
};

EstadoSistema estadoActual = ESTADO_MONITOREO;
unsigned long tiempoInicioBombeo = 0;
unsigned long ultimoMuestreo = 0;
int humedadPorcentajeActual = 0;
int humedadInicialAlRegar = 0;

// =============================================================================
// FUNCION: LECTURA PROMEDIADA DEL SENSOR
// =============================================================================
int leerPorcentajeHumedad() {
  long sumaLecturas = 0;
  const int NUM_MUESTRAS = 16;
  
  for (int i = 0; i < NUM_MUESTRAS; i++) {
    sumaLecturas += analogRead(PIN_SENSOR_HUMEDAD);
    delay(5);
  }
  int promedioRaw = sumaLecturas / NUM_MUESTRAS;

  // Mapear escala inversa (mayor voltaje = menor humedad)
  int porcentaje = map(promedioRaw, VALOR_CALIBRACION_AIRE, VALOR_CALIBRACION_AGUA, 0, 100);
  porcentaje = constrain(porcentaje, 0, 100);
  return porcentaje;
}

// =============================================================================
// FUNCION: ACTUALIZAR PANTALLA OLED
// =============================================================================
void actualizarPantalla(int humedad, const char* estadoTexto) {
  display.clearDisplay();

  // Título superior
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 2);
  display.print(F("AUTO-REGADOR v1.0"));
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

  // Porcentaje Grande de Humedad
  display.setTextSize(3);
  display.setCursor(18, 18);
  display.print(humedad);
  display.print(F("%"));

  // Barra de progreso horizontal de humedad
  int anchoBarra = map(humedad, 0, 100, 0, 120);
  display.drawRect(4, 46, 120, 6, SSD1306_WHITE);
  display.fillRect(4, 46, anchoBarra, 6, SSD1306_WHITE);

  // Mensaje de Estado inferior
  display.setTextSize(1);
  display.setCursor(6, 55);
  display.print(F("ESTADO: "));
  display.print(estadoTexto);

  display.display();
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(9600);
  pinMode(PIN_DRIVER_BOMBA, OUTPUT);
  digitalWrite(PIN_DRIVER_BOMBA, LOW); // Bomba apagada por seguridad al arrancar

  // Inicialización de OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Error: No se encontro pantalla OLED SSD1306"));
    while (true); // Bloquear si no hay pantalla
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 20);
  display.println(F("INICIALIZANDO..."));
  display.setCursor(20, 35);
  display.println(F("SISTEMA DE RIEGO"));
  display.display();
  delay(1500);

  humedadPorcentajeActual = leerPorcentajeHumedad();
}

// =============================================================================
// LOOP PRINCIPAL (NO BLOQUEANTE)
// =============================================================================
void loop() {
  unsigned long tiempoActual = millis();

  // 1. Muestreo periódico del sensor
  if (tiempoActual - ultimoMuestreo >= INTERVALO_LECTURA_SENSOR) {
    ultimoMuestreo = tiempoActual;
    humedadPorcentajeActual = leerPorcentajeHumedad();
  }

  // 2. Máquina de Estados
  switch (estadoActual) {
    case ESTADO_MONITOREO:
      digitalWrite(PIN_DRIVER_BOMBA, LOW);
      actualizarPantalla(humedadPorcentajeActual, (humedadPorcentajeActual < UMBRAL_RIEGO_INICIO) ? "SECO" : "OPTIMO");

      // Disparo automático cuando se seca la tierra
      if (humedadPorcentajeActual <= UMBRAL_RIEGO_INICIO) {
        humedadInicialAlRegar = humedadPorcentajeActual;
        tiempoInicioBombeo = millis();
        estadoActual = ESTADO_REGANDO;
      }
      break;

    case ESTADO_REGANDO:
      digitalWrite(PIN_DRIVER_BOMBA, HIGH); // Encender bomba
      actualizarPantalla(humedadPorcentajeActual, "REGANDO...");

      // Condición de éxito: Al llegar al umbral de satisfacción
      if (humedadPorcentajeActual >= UMBRAL_RIEGO_FIN) {
        digitalWrite(PIN_DRIVER_BOMBA, LOW);
        estadoActual = ESTADO_MONITOREO;
      }

      // Condición de seguridad Failsafe: Tiempo límite excedido
      if (tiempoActual - tiempoInicioBombeo >= TIEMPO_MAXIMO_BOMBEO) {
        digitalWrite(PIN_DRIVER_BOMBA, LOW); // Cortar bomba de inmediato

        // Si después de 25s no subió al menos un 5%, el depósito está vacío o la bomba no chupa agua
        if (humedadPorcentajeActual - humedadInicialAlRegar < 5) {
          estadoActual = ESTADO_ERROR_REFILL;
        } else {
          estadoActual = ESTADO_MONITOREO; // Si subió un poco pero acabó el tiempo, vuelve a monitoreo
        }
      }
      break;

    case ESTADO_ERROR_REFILL:
      digitalWrite(PIN_DRIVER_BOMBA, LOW); // Bomba siempre apagada
      actualizarPantalla(humedadPorcentajeActual, "SIN AGUA! (REFILL)");

      // Permite desbloquear el error una vez que el usuario humedezca o reinicie
      if (humedadPorcentajeActual > UMBRAL_RIEGO_INICIO + 10) {
        estadoActual = ESTADO_MONITOREO;
      }
      break;
  }
}
