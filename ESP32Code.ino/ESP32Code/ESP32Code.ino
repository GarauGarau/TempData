#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

// Configurazione Wi-Fi
const char* ssid = "VodafoneCVGA";
const char* password = "modernbolt125";

// Sensore BME680
Adafruit_BME680 bme;

// URL del Google Apps Script
const char* serverName = "https://script.google.com/macros/s/AKfycbwtm4dpGCTzrnTVLz0WJSYQA-hbAtcXpSUnuj3gnWg8yunMbFHSkKeK9rtdRxv51C1q-Q/exec";

// Variabili per gestire il tempo
unsigned long previousMillis = 0;
const long interval = 60000;  // 1 minuto
const int numSamples = 5;  // Numero di campioni per il calcolo della media

void setup() {
  Serial.begin(115200);

  // Disabilita il risparmio energetico del Wi-Fi
  WiFi.setSleep(false);

  // Connessione al Wi-Fi
  WiFi.begin(ssid, password);
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    retryCount++;
    if (retryCount >= 30) {  // 30 secondi di tentativi
      Serial.println("WiFi connection failed. Restarting...");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");

  // Inizializza BME680
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    delay(5000);  // Ritardo prima di riprovare
    ESP.restart();
  }

  // Configura il sensore BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // 320°C per 150ms
}

void loop() {
  // Verifica lo stato del Wi-Fi e riconnessione in caso di disconnessione
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected, attempting reconnection...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Reconnecting to WiFi...");
      retryCount++;
      if (retryCount >= 30) {
        Serial.println("Reconnection failed. Restarting...");
        ESP.restart();
      }
    }
    Serial.println("Reconnected to WiFi");
  }

  // Gestione del tempo per misurare i dati ogni minuto
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Leggi più campioni dal sensore BME680 e calcola la media
    float temperatureSum = 0;
    float pressureSum = 0;
    float humiditySum = 0;
    float gasSum = 0;

    for (int i = 0; i < numSamples; i++) {
      if (bme.performReading()) {
        temperatureSum += bme.temperature;
        pressureSum += bme.pressure / 100.0;  // Converti in hPa
        humiditySum += bme.humidity;
        gasSum += bme.gas_resistance / 1000.0;  // Converti in kOhms
      } else {
        Serial.println("Errore nella lettura dei dati dal sensore BME680.");
      }
      delay(500);  // Ritardo tra le letture per consentire al sensore di stabilizzarsi
    }

    // Calcola la media dei campioni
    float temperature = temperatureSum / numSamples;
    float pressure = pressureSum / numSamples;
    float humidity = humiditySum / numSamples;
    float gas = gasSum / numSamples;

    // Formatta i dati con due decimali
    String temperatureString = String(temperature, 2);
    String pressureString = String(pressure, 2);
    String humidityString = String(humidity, 2);
    String gasString = String(gas, 2);

    // Invia i dati al Google Sheets
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String httpRequestData = "temperature=" + temperatureString + "&pressure=" + pressureString + "&humidity=" + humidityString + "&gas=" + gasString;
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
      } else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    } else {
      Serial.println("WiFi Disconnected");
    }
  }
}
