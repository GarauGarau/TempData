/*********** Inclusione delle Librerie Necessarie ***********/
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include "bsec.h"

/*********** Configurazione della Libreria BSEC ***********/
const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

/*********** Credenziali Wi-Fi ***********/
const char* ssid = "VodafoneCVGA";        
const char* password = "modernbolt125";

/*********** URL dell'App Script ***********/
const char* serverName = "https://script.google.com/macros/s/AKfycby7rKIv4MhSYAFa_GI-E-YdLyZnFWbjzIK3Mg67JL-ZiRrmcN1O1EQ0xWgeQjAEbGyuqg/exec";

/*********** Indirizzo I2C del BME680 ***********/
#define BME68X_I2C_ADDR BME68X_I2C_ADDR_HIGH // Sostituisci con l'indirizzo corretto dopo aver eseguito lo scanner I2C

/*********** Definizione dei Pin I2C per l'ESP32 ***********/
#define I2C_SDA 21
#define I2C_SCL 22

/*********** Definizione del LED Integrato ***********/
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

/*********** Altre Variabili ***********/
#define STATE_SAVE_PERIOD  UINT32_C(360 * 60 * 1000) // 360 minuti - 4 volte al giorno
Bsec iaqSensor;
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
uint16_t stateUpdateCounter = 0;
String output;
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 60000; // 60 secondi (1 minuto)

/*********** Prototipi delle Funzioni ***********/
void checkIaqSensorStatus(void);
void errLeds(void);
void loadState(void);
void updateState(void);
void sendDataToGoogleSheets();

/*********** Funzione Setup ***********/
void setup() {
  Serial.begin(115200);
  delay(100);

  // Connetti al Wi-Fi
  Serial.print("Connessione a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connesso.");
  Serial.print("Indirizzo IP: ");
  Serial.println(WiFi.localIP());

  // Inizializzazione del Sensore e delle Componenti
  EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1);
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("I2C inizializzato");
  iaqSensor.begin(BME68X_I2C_ADDR, Wire);
  Serial.println("Sensore BME680 inizializzato");
  checkIaqSensorStatus();

  output = "\nVersione libreria BSEC " + String(iaqSensor.version.major) + "." +
           String(iaqSensor.version.minor) + "." +
           String(iaqSensor.version.major_bugfix) + "." +
           String(iaqSensor.version.minor_bugfix);
  Serial.println(output);

  iaqSensor.setConfig(bsec_config_iaq);
  checkIaqSensorStatus();

  loadState();

  bsec_virtual_sensor_t sensorList[13] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_GAS_PERCENTAGE
  };

  iaqSensor.updateSubscription(sensorList, 13, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();
}

/*********** Funzione Loop ***********/
void loop() {
  if (iaqSensor.run()) {
    digitalWrite(LED_BUILTIN, HIGH);

    if (millis() - lastPrintTime >= printInterval) {
      lastPrintTime = millis();

      // Costruisci l'output da stampare
      output = "Timestamp [ms]: " + String(lastPrintTime) + "\n";
      output += "Temperatura [°C]: " + String(iaqSensor.temperature) + "\n";
      output += "Umidità [%]: " + String(iaqSensor.humidity) + "\n";
      output += "Pressione [hPa]: " + String(iaqSensor.pressure / 100) + "\n";
      output += "Gas [kΩ]: " + String(iaqSensor.gasResistance / 1000) + "\n";
      output += "IAQ: " + String(iaqSensor.iaq) + "\n";
      output += "IAQ Accuracy: " + String(iaqSensor.iaqAccuracy) + "\n";
      // Aggiungi altri parametri se desideri
      Serial.println(output);

      // Invia i dati al Google Sheet
      sendDataToGoogleSheets();

      // Aggiorna lo stato se necessario
      updateState();
    }

    digitalWrite(LED_BUILTIN, LOW);
  } else {
    checkIaqSensorStatus();
  }
}

/*********** Funzione per Inviare i Dati al Google Sheet ***********/
void sendDataToGoogleSheets() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Costruisci i dati da inviare
    String httpRequestData = "temperature=" + String(iaqSensor.temperature)
                           + "&humidity=" + String(iaqSensor.humidity)
                           + "&pressure=" + String(iaqSensor.pressure / 100)
                           + "&gas=" + String(iaqSensor.gasResistance / 1000)
                           + "&rawTemperature=" + String(iaqSensor.rawTemperature)
                           + "&rawHumidity=" + String(iaqSensor.rawHumidity)
                           + "&gasResistance=" + String(iaqSensor.gasResistance)
                           + "&iaq=" + String(iaqSensor.iaq)
                           + "&iaqAccuracy=" + String(iaqSensor.iaqAccuracy)
                           + "&staticIaq=" + String(iaqSensor.staticIaq)
                           + "&co2Equivalent=" + String(iaqSensor.co2Equivalent)
                           + "&breathVocEquivalent=" + String(iaqSensor.breathVocEquivalent)
                           + "&gasPercentage=" + String(iaqSensor.gasPercentage);

    // Invia la richiesta POST
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Errore nell'invio della POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnesso");
  }
}

/*********** Funzione per Verificare lo Stato del Sensore ***********/
void checkIaqSensorStatus(void) {
  if (iaqSensor.bsecStatus != BSEC_OK) {
    if (iaqSensor.bsecStatus < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.bsecStatus);
      Serial.println(output);
      // Gestisci l'errore secondo le tue esigenze
    } else {
      output = "BSEC warning code : " + String(iaqSensor.bsecStatus);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme68xStatus != BME68X_OK) {
    if (iaqSensor.bme68xStatus < BME68X_OK) {
      output = "BME68X error code : " + String(iaqSensor.bme68xStatus);
      Serial.println(output);
      // Gestisci l'errore secondo le tue esigenze
    } else {
      output = "BME68X warning code : " + String(iaqSensor.bme68xStatus);
      Serial.println(output);
    }
  }
}

/*********** Funzione per Gestire gli Errori con il LED ***********/
void errLeds(void) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}

/*********** Funzione per Caricare lo Stato Salvato ***********/
void loadState(void) {
  if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE) {
    // Stato esistente in EEPROM
    Serial.println("Lettura dello stato dall'EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
      bsecState[i] = EEPROM.read(i + 1);
    }

    iaqSensor.setState(bsecState);
    checkIaqSensorStatus();
  } else {
    // Cancella l'EEPROM
    Serial.println("Cancellazione dell'EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE + 1; i++)
      EEPROM.write(i, 0);

    EEPROM.commit();
  }
}

/*********** Funzione per Aggiornare lo Stato ***********/
void updateState(void) {
  bool update = false;
  if (stateUpdateCounter == 0) {
    /* Primo aggiornamento dello stato quando l'IAQ accuracy è >= 3 */
    if (iaqSensor.iaqAccuracy >= 3) {
      update = true;
      stateUpdateCounter++;
    }
  } else {
    /* Aggiorna ogni STATE_SAVE_PERIOD */
    if ((stateUpdateCounter * STATE_SAVE_PERIOD) < millis()) {
      update = true;
      stateUpdateCounter++;
    }
  }

  if (update) {
    iaqSensor.getState(bsecState);
    checkIaqSensorStatus();

    Serial.println("Scrittura dello stato nell'EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE ; i++) {
      EEPROM.write(i + 1, bsecState[i]);
    }

    EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
    EEPROM.commit();
  }
}
