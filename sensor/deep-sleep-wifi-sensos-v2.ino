/**
   ESP8266 + DHT11 + DS18B20 + Deep-sleep mode
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <OneWire.h>                
#include <DallasTemperature.h>

// WiFi credentials.
const char* WIFI_SSID = "ba-sing-se 2.4 GHZ";
const char* WIFI_PASS = "LotoBlanco";
const char* serverName = "http://192.168.0.33:9000/sensor";

// Client
  WiFiClient client;
  HTTPClient http;

// Sensors
//    DS18B20
OneWire ourWire(4);                      //Se establece el pin 2  como bus OneWire
DallasTemperature sensors(&ourWire);     //Se declara una variable u objeto para nuestro sensor
//     sensor DHT11
const byte dhtPin = 5;
#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);


// Connect to Wifi.
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

void notify(char message[]) {
  http.begin(client, serverName);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(message);
  Serial.print("Posted :");
  Serial.println(message);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  // Free resources
  http.end();
}

void dht11Notify(){
   float dht11_temp, dht11_hum;

   dht.begin();
   dht11_hum = dht.readHumidity();
   dht11_temp = dht.readTemperature();

   if (isnan(dht11_hum) || isnan(dht11_temp)) {
   Serial.println(F("Failed to read from DHT sensor!"));
   return;
}

   char buffer[100] = "";
   sprintf_P(buffer, "{\"dht11\":{\"temp\":%d.%02d,\"hum\":%d.%02d}}", (int)dht11_temp, (int)(dht11_temp*100)%100, (int)dht11_hum, (int)(dht11_hum*100)%100);
   notify(buffer);

}

void ds18b20Notify(){
   float ds18b20_temp;

   sensors.begin();
   sensors.requestTemperatures();
   ds18b20_temp= sensors.getTempCByIndex(0);

   if (isnan(ds18b20_temp)) {
      Serial.println(F("Failed to read from DS18B20 sensor!"));
      return;
   }

   char buffer[50] = "";
   sprintf_P(buffer, "{\"ds18b20\":{\"temp\":%d.%02d}}", (int)ds18b20_temp, (int)(ds18b20_temp*100)%100);
   
   notify(buffer);
}

void serialInitialization(){
   Serial.begin(9600);
   Serial.setTimeout(2000);

   // Wait for serial to initialize.
   while (!Serial) {}

   Serial.println("Device Started");
   Serial.println("-------------------------------------");
}

void deepSleep(){
   Serial.println("Running Deep Sleep Firmware!");
   Serial.println("Going into deep sleep for 60 seconds");
   Serial.println("-------------------------------------");
   Serial.println("");
   ESP.deepSleep(60e6);  // 60e6 is 60 microseconds
}

void setup() {
   serialInitialization();
   connect();
   ds18b20Notify();
   dht11Notify();
   deepSleep();
}

void loop() {
}
