#include <ThreeWire.h>
#include <RtcDS1302.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// -------------------------
// Pines RTC DS1302
// -------------------------
#define PIN_CLK 16
#define PIN_DAT 17
#define PIN_RST 4

ThreeWire myWire(PIN_DAT, PIN_CLK, PIN_RST);
RtcDS1302<ThreeWire> rtc(myWire);

// -------------------------
// Pin CS de la microSD
// -------------------------
#define SD_CS 5

// -------------------------
// Variable para evitar repetir cabecera
// -------------------------
bool encabezadoEscrito = false;

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

  // Ajustar hora solo si no es válida
  if (!rtc.IsDateTimeValid()) {
    Serial.println("RTC sin hora valida, ajustando con hora de compilacion...");
    rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
  }

  // Si quieres forzar manualmente una fecha/hora, usa esta linea
  // rtc.SetDateTime(RtcDateTime(2026, 4, 13, 12, 30, 0));

  // -------------------------
  // Inicializar SD
  // -------------------------
  if (!SD.begin(SD_CS)) {
    Serial.println("Error al iniciar SD");
  } else {
    Serial.println("SD iniciada correctamente");

    // Crear archivo con encabezado si no existe
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

    encabezadoEscrito = true;
  }
}

void loop() {
  // -------------------------
  // Leer fecha y hora del RTC
  // -------------------------
  RtcDateTime now = rtc.GetDateTime();

  // -------------------------
  // Leer temperatura interna
  // -------------------------
  float temp = temperatureRead();

  // -------------------------
  // Formatear fecha y hora
  // -------------------------
  char fecha[11];   // YYYY-MM-DD
  char hora[9];     // HH:MM:SS

  snprintf(fecha, sizeof(fecha), "%04u-%02u-%02u",
           now.Year(), now.Month(), now.Day());

  snprintf(hora, sizeof(hora), "%02u:%02u:%02u",
           now.Hour(), now.Minute(), now.Second());

  // -------------------------
  // Mostrar en monitor serial
  // -------------------------
  Serial.print(fecha);
  Serial.print(",");
  Serial.print(hora);
  Serial.print(",");
  Serial.println(temp, 2);

  // -------------------------
  // Guardar en SD
  // -------------------------
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

  delay(5000); // guardar cada 5 segundos
}