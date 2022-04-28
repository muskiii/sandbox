/**
   ESP8266 + DHT11 + DS18B20 + Deep-sleep mode
*/
#include <string>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>


// Sensors

//    DS18B20
OneWire ourWire(4);                      //Se establece el pin 2  como bus OneWire
DallasTemperature sensors(&ourWire);     //Se declara una variable u objeto para nuestro sensor

//    DHT11
const byte dhtPin = 5;
#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);

//    KY-018
#define PIN_ANALOG A0

// WiFi credentials.
const char* WIFI_SSID = "ba-sing-se 2.4 GHZ";
const char* WIFI_PASS = "LotoBlanco";
const char* serverName = "http://192.168.0.33:9000/sensor";

// Client
WiFiClient client;
HTTPClient http;


void connect() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {

    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WiFi. Please verify credentials: ");
      delay(10000);
    }

    delay(500);
    Serial.println("...");
    // Only try for 15 seconds.
    if (millis() - wifiConnectStart > 15000) {
      Serial.println("Failed to connect to WiFi");
      return;
    }
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void notify(char* message) {
  int httpResponseCode = http.POST(message);
  Serial.print("Posted :");
  Serial.println(message);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
}

void dht11Reading(char* buffer) {
  String sensor = "dht11";
  float dht11_temp, dht11_hum;

  dht.begin();
  dht11_hum = dht.readHumidity();
  dht11_temp = dht.readTemperature();

  if (!isnan(dht11_hum) && !isnan(dht11_temp)) {
    sprintf_P(buffer, "{\"%s\":{\"temp\":%d.%02d,\"hum\":%d.%02d}}", (String)sensor, (int)dht11_temp, (int)(dht11_temp * 100) % 100, (int)dht11_hum, (int)(dht11_hum * 100) % 100);

  } else {
    Serial.println("Failed to read from DHT11 sensor!");
    sprintf_P(buffer, "{\"sensorFailure\":{\"name\":%s}}", (String)sensor);
  }
}

void ky018Reading(char* buffer) {
  String sensor = "ky018";

  int measureMean = 0;
  int measureCont = 0;
  int successfulMeasures = 30;

  for (int i = 0; i < 30; ++i) {
    int measure = analogRead(PIN_ANALOG);

    if (isnan(measure)) {
      successfulMeasures --;
    }else {
      measureCont += measure;
    }
  }

  if (successfulMeasures > 25) {
    measureMean = measureCont / successfulMeasures;
    sprintf_P(buffer, "{\"%s\":{\"light\":%d}}", sensor, (int)measureMean);

  } else {
    Serial.println("Failed to read from KY-018 sensor!");
    sprintf_P(buffer, "{\"sensorFailure\":{\"name\":%s}}", (String)sensor);
  }
}

void ds18b20Reading(char* buffer) {
  String sensor = "ds18b20";
  float ds18b20_temp;

  sensors.begin();
  sensors.requestTemperatures();
  ds18b20_temp = sensors.getTempCByIndex(0);

  if (!isnan(ds18b20_temp)) {
    sprintf_P(buffer, "{\"ds18b20\":{\"temp\":%d.%02d}}", (int)ds18b20_temp, (int)(ds18b20_temp * 100) % 100);

  } else {
    Serial.println("Failed to read from DS18B20 sensor!");
    sprintf_P(buffer, "{\"sensorFailure\":{\"name\":%s}}", (String)sensor);
  }
}

void serialInitialization() {
  Serial.begin(9600);
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while (!Serial) {}
  Serial.println("Device Started");
  Serial.println("-------------------------------------");
}

void deepSleep() {
  Serial.println("Running Deep Sleep Firmware!");
  Serial.println("Going into deep sleep for 60 seconds");
  Serial.println("-------------------------------------");
  Serial.println("");
  ESP.deepSleep(60e6);  // 60e6 is 60 microseconds
}


void setup() {
  http.begin(client, serverName);
  http.addHeader("Content-Type", "application/json");

  serialInitialization();
  connect();

  char dht11Payload[100] = "";
  dht11Reading(dht11Payload);
  notify(dht11Payload);

  char ky018Payload[100] = "";
  ky018Reading(ky018Payload);
  notify(ky018Payload);

  char ds18b20Payload[100] = "";
  ds18b20Reading(ds18b20Payload);
  notify(ds18b20Payload);

  // Free up resources
  http.end();
  deepSleep();
}

void loop() {
}
