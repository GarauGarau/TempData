#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "bsec_integration.h"  // Include the BSEC library
#include "commMux.h"  // Include the communication multiplexer library

// Configurazione Wi-Fi
const char* ssid = "VodafoneCVGA";
const char* password = "modernbolt125";

// URL del Google Apps Script
const char* serverName = "https://script.google.com/macros/s/AKfycbwtm4dpGCTzrnTVLz0WJSYQA-hbAtcXpSUnuj3gnWg8yunMbFHSkKeK9rtdRxv51C1q-Q/exec";

// Variabili per gestire il tempo
unsigned long previousMillis = 0;
const long interval = 60000;  // 1 minuto
const int numSamples = 10;  // Numero di campioni per il calcolo della media

// Variabili BSEC
String output;
uint32_t overflowCounter;
uint32_t lastTimeMS;
commMux communicationSetup[NUM_OF_SENS];

// Funzione per inviare i dati via HTTP
void sendData(float temperature, float pressure, float humidity, float gas) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String httpRequestData = "temperature=" + String(temperature, 2) + "&pressure=" + String(pressure, 2) + "&humidity=" + String(humidity, 2) + "&gas=" + String(gas, 2);
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

void setup() {
  Serial.begin(115200);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    retryCount++;
    if (retryCount >= 30) {
      Serial.println("WiFi connection failed. Restarting...");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");

  // Inizializza BSEC e il sensore BME680
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  commMuxBegin(Wire, SPI);c:\Users\aless\Downloads\bsec2-6-1-0_generic_release\examples\BSEC_Integration_Examples\examples\bsec_iot_example\bsec_iot_example.ino
  
  // Impostazioni BSEC
  struct bme68x_dev bme_dev[NUM_OF_SENS];
  return_values_init ret;
  
  for (uint8_t i = 0; i < NUM_OF_SENS; i++) {
    communicationSetup[i] = commMuxSetConfig(Wire, SPI, i, communicationSetup[i]);
    memset(&bme_dev[i], 0, sizeof(bme_dev[i]));
    bme_dev[i].intf = BME68X_SPI_INTF;
    bme_dev[i].read = commMuxRead;
    bme_dev[i].write = commMuxWrite;
    bme_dev[i].delay_us = commMuxDelay;
    bme_dev[i].intf_ptr = &communicationSetup[i];
    bme_dev[i].amb_temp = 25;

    allocateMemory(bsec_mem_block[i], i);
    ret = bsec_iot_init(SAMPLE_RATE, 0.0f, bus_write, bus_read, sleep_n, state_load, config_load, bme_dev[i], i);

    if (ret.bme68x_status) {
      Serial.println("ERROR while initializing BME68x:" + String(ret.bme68x_status));
      return;
    } else if (ret.bsec_status < BSEC_OK) {
      Serial.println("ERROR while initializing BSEC library: " + String(ret.bsec_status));
      return;
    } else if (ret.bsec_status > BSEC_OK) {
      Serial.println("WARNING while initializing BSEC library: " + String(ret.bsec_status));
    }
  }

  Serial.println("BSEC library initialized successfully.");
  bsec_iot_loop(sleep_n, get_timestamp_us, output_ready, state_save, 10000);  // Start the BSEC loop
}

void loop() {
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

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Codice di lettura BSEC sarà chiamato automaticamente nella loop principale bsec_iot_loop
  }
}

// Funzione di output ready per la gestione dell'output BSEC
void output_ready(output_t *outputs, bsec_library_return_t bsec_status) {
  float temperature = outputs->temperature;
  float pressure = outputs->raw_pressure / 100.0;  // Convert to hPa
  float humidity = outputs->humidity;
  float gas = outputs->gas_estimate_1;  // Example gas value

  // Invia i dati al server
  sendData(temperature, pressure, humidity, gas);
}
