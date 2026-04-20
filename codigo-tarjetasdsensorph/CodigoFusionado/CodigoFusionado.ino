#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>

// =========================
// WIFI
// =========================
const char* ssid = "Coffeewet";
const char* password = "Iguana123";

// =========================
// URL DEL APPS SCRIPT
// =========================
const char* serverName = "https://script.google.com/macros/s/AKfycbyrMMl0y7Sj9ECJRS_mmKMIioQwZnhI1H51ajAmN74aWeCRJstaxF0jfHqXaJIgvaJF0Q/exec";

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
// SENSOR pH
// =========================
const int pinPH = 32;

// =========================
// CONTROL DE TIEMPO
// =========================
unsigned long ultimoGuardado = 0;
const unsigned long intervalo = 600000UL; // 10 min

// =========================
// MEDIR pH
// =========================
float medirPH() {
  long suma = 0;

  for (int i = 0; i < 20; i++) {
    suma += analogRead(pinPH);
    delay(10);
  }

  float adcPromedio = suma / 20.0;
  float millivolts = (adcPromedio * 3.3 / 4095.0) * 1000.0;
  float phValue = (0.002237 * millivolts) + 1.05;

  return phValue;
}

// =========================
// ENVIAR DATOS A WEB
// =========================
void enviarDatosWeb(String fecha, String hora, float ph) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"fecha\":\"" + fecha + "\",\"hora\":\"" + hora + "\",\"ph\":" + String(ph, 2) + "}";

    int httpResponseCode = http.POST(jsonData);

    Serial.print("Datos enviados -> ");
    Serial.print(fecha);
    Serial.print(" ");
    Serial.print(hora);
    Serial.print(" | pH: ");
    Serial.println(ph, 2);

    Serial.print("Codigo HTTP: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respuesta servidor:");
      Serial.println(response);
    } else {
      Serial.println("Error enviando datos");
    }

    http.end();
  } else {
    Serial.println("WiFi desconectado, no se pudo enviar a la web");
  }
}

// =========================
// GUARDAR EN SD Y ENVIAR
// =========================
void guardarYEnviarDato() {
  RtcDateTime now = rtc.GetDateTime();
  float ph = medirPH();

  char fechaChar[11];
  char horaChar[9];

  snprintf(fechaChar, sizeof(fechaChar), "%04u-%02u-%02u",
           now.Year(), now.Month(), now.Day());

  snprintf(horaChar, sizeof(horaChar), "%02u:%02u:%02u",
           now.Hour(), now.Minute(), now.Second());

  String fecha = String(fechaChar);
  String hora  = String(horaChar);

  Serial.print("Guardando dato: ");
  Serial.print(fecha);
  Serial.print(",");
  Serial.print(hora);
  Serial.print(",");
  Serial.println(ph, 2);

  // FILE_APPEND evita sobreescribir
  File file = SD.open("/datos_ph.csv", FILE_APPEND);
  if (file) {
    file.print(fecha);
    file.print(",");
    file.print(hora);
    file.print(",");
    file.println(ph, 2);
    file.close();
    Serial.println("Dato guardado en SD");
  } else {
    Serial.println("No se pudo abrir datos_ph.csv para escribir");
  }

  // Luego de guardar en SD, enviar a la web
  enviarDatosWeb(fecha, hora, ph);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Iniciando sistema...");
  analogReadResolution(12);

  // =========================
  // INICIALIZAR RTC
  // =========================
  rtc.Begin();
  rtc.SetIsWriteProtected(false);
  rtc.SetIsRunning(true);

  // IMPORTANTE:
  // Como ya calibraste el reloj, NO vuelvas a usar SetDateTime aquí,
  // porque si no reinicia la hora cada vez que encienda.
  // rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));

  // =========================
  // INICIALIZAR SD
  // =========================
  if (!SD.begin(SD_CS)) {
    Serial.println("Error al iniciar SD");
    while (1);
  }

  Serial.println("SD iniciada correctamente");

  // Crear archivo si no existe
  if (!SD.exists("/datos_ph.csv")) {
    File file = SD.open("/datos_ph.csv", FILE_WRITE);
    if (file) {
      file.println("Fecha,Hora,pH");
      file.close();
      Serial.println("Archivo datos_ph.csv creado");
    } else {
      Serial.println("No se pudo crear datos_ph.csv");
    }
  } else {
    Serial.println("Archivo datos_ph.csv ya existe");
  }

  // =========================
  // CONECTAR WIFI
  // =========================
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado a WiFi");
  Serial.println(WiFi.localIP());

  // =========================
  // PRIMER DATO APENAS ENCIENDE
  // =========================
  guardarYEnviarDato();
  ultimoGuardado = millis();
}

void loop() {
  unsigned long ahora = millis();

  if (ahora - ultimoGuardado >= intervalo) {
    guardarYEnviarDato();
    ultimoGuardado = ahora;
  }

  // Mostrar hora actual del RTC
  RtcDateTime now = rtc.GetDateTime();

  char fecha[11];
  char hora[9];

  snprintf(fecha, sizeof(fecha), "%04u-%02u-%02u",
           now.Year(), now.Month(), now.Day());

  snprintf(hora, sizeof(hora), "%02u:%02u:%02u",
           now.Hour(), now.Minute(), now.Second());

  Serial.print("Hora RTC: ");
  Serial.print(fecha);
  Serial.print(" ");
  Serial.println(hora);

  delay(1000);
}