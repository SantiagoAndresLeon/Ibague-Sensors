#include <WiFi.h>
#include <HTTPClient.h>

// 🔹 WIFI
const char* ssid = "Coffeewet";
const char* password = "Iguana123";

// 🔹 URL DEL APPS SCRIPT
const char* serverName = "https://script.google.com/macros/s/AKfycbyrMMl0y7Sj9ECJRS_mmKMIioQwZnhI1H51ajAmN74aWeCRJstaxF0jfHqXaJIgvaJF0Q/exec";

// 🔹 Pin del sensor
const int pinPH = 32;

// 🔹 Control de tiempo
unsigned long lastTime = 0;
const unsigned long interval = 600000; // 10 minutos

void setup() {

  Serial.begin(115200);
  analogReadResolution(12);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.println(WiFi.localIP());

  // 🔥 ENVÍA UNA VEZ APENAS SE CONECTA
  enviarDatos();
  lastTime = millis();
}

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

void enviarDatos() {

  if (WiFi.status() == WL_CONNECTED) {

    float ph = medirPH();

    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"ph\":" + String(ph, 2) + "}";

    int httpResponseCode = http.POST(jsonData);

    Serial.print("pH enviado: ");
    Serial.println(ph, 2);

    Serial.print("Código HTTP: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.println("Error enviando datos");
    }

    http.end();
  } else {
    Serial.println("WiFi desconectado");
  }
}

void loop() {

  if (millis() - lastTime >= interval) {
    enviarDatos();
    lastTime = millis();
  }
}