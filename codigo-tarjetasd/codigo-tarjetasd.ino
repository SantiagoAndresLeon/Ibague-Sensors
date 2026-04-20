#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// =========================
// RTC DS1302
// =========================
#define PIN_CLK 16
#define PIN_DAT 17
#define PIN_RST 4

ThreeWire myWire(PIN_DAT, PIN_CLK, PIN_RST);
RtcDS1302<ThreeWire> rtc(myWire);

// =========================
// SD
// =========================
#define SD_CS 5

// =========================
// Control de tiempo
// =========================
unsigned long ultimoGuardado = 0;
const unsigned long intervalo = 600000UL; // 10 min = 600000 ms

void guardarDato() {
  RtcDateTime now = rtc.GetDateTime();
  float temp = temperatureRead();

  char fecha[11];
  char hora[9];

  snprintf(fecha, sizeof(fecha), "%04u-%02u-%02u",
           now.Year(), now.Month(), now.Day());

  snprintf(hora, sizeof(hora), "%02u:%02u:%02u",
           now.Hour(), now.Minute(), now.Second());

  Serial.print("Guardando dato: ");
  Serial.print(fecha);
  Serial.print(",");
  Serial.print(hora);
  Serial.print(",");
  Serial.println(temp, 2);

  File file = SD.open("/datos.csv", FILE_APPEND);
  if (file) {
    file.print(fecha);
    file.print(",");
    file.print(hora);
    file.print(",");
    file.println(temp, 2);
    file.close();
    Serial.println("Dato guardado en SD");
  } else {
    Serial.println("No se pudo abrir datos.csv para escribir");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Iniciando sistema...");

  // -------------------------
  // Inicializar RTC
  // -------------------------
  rtc.Begin();
  rtc.SetIsWriteProtected(false);
  rtc.SetIsRunning(true);

  // Ajusta la hora al iniciar
  rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
  Serial.println("RTC calibrado");

  // -------------------------
  // Inicializar SD
  // -------------------------
  if (!SD.begin(SD_CS)) {
    Serial.println("Error al iniciar SD");
    while (1);
  }

  Serial.println("SD iniciada correctamente");

  // Crear archivo si no existe
  if (!SD.exists("/datos.csv")) {
    File file = SD.open("/datos.csv", FILE_WRITE);
    if (file) {
      file.println("Fecha,Hora,Temp_ESP32");
      file.close();
      Serial.println("Archivo datos.csv creado");
    } else {
      Serial.println("No se pudo crear datos.csv");
    }
  } else {
    Serial.println("Archivo datos.csv ya existe");
  }

  // -------------------------
  // Guardar primer dato apenas enciende
  // -------------------------
  guardarDato();

  // Guardar referencia de tiempo
  ultimoGuardado = millis();
}

void loop() {
  unsigned long ahora = millis();

  if (ahora - ultimoGuardado >= intervalo) {
    guardarDato();
    ultimoGuardado = ahora;
  }

  // Mostrar hora actual en serial
  RtcDateTime now = rtc.GetDateTime();

  char fecha[11];
  char hora[9];

  snprintf(fecha, sizeof(fecha), "%04u-%02u-%02u",
           now.Year(), now.Month(), now.Day());

  snprintf(hora, sizeof(hora), "%02u:%02u:%02u",
           now.Hour(), now.Minute(), now.Second());

  Serial.print("Hora actual: ");
  Serial.print(fecha);
  Serial.print(" ");
  Serial.println(hora);

  delay(1000);
}