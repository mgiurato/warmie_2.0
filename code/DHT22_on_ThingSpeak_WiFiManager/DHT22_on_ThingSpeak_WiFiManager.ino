//#include "arduino_secrets.h"
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

// Wifi connection to the router
//const char* ssid  = SECRET_SSID;
//const char* password = SECRET_PASS;

// ThingSpeak
#define SECRET_THINGSPEAK_API_KEY_LENGTH 16
char apiKey[SECRET_THINGSPEAK_API_KEY_LENGTH] = "";
//String apiKey = SECRET_THINGSPEAK_API_KEY;
const char* server = "api.thingspeak.com";

// DHT22
#define DHTPOWER D1
#define DHTPIN D2
#define DHTTYPE DHT22

// Connect pin 1 (on the left) of the sensor to D0
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND

DHT dht(DHTPIN, DHTTYPE);

//flag for saving data
bool shouldSaveConfig = false;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for the Serial monitor to be opened
  
  EEPROM.begin(SECRET_THINGSPEAK_API_KEY_LENGTH);
  Serial.println("read API key");
  readApiKeyFromEeprom(0);
  Serial.println(apiKey);

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
  
  WiFiManagerParameter custom_api_key("apikey", "ThingSpeak API Key", apiKey, 20);
  wifiManager.addParameter(&custom_api_key);
  wifiManager.autoConnect();

  strcpy(apiKey, custom_api_key.getValue());
  if (shouldSaveConfig) {
    writeApiKeyToEeprom(0);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(DHTPOWER, OUTPUT);
  delay(500);
  dht.begin();
  Serial.println("ESP8266 initialised");
}

void loop(){
  digitalWrite(DHTPOWER, HIGH);
  delay(1000);
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t))  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  if (client.connect(server, 80)){
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(t);
    postStr += "&field2=";
    postStr += String(h);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: ");
    client.println(apiKey);
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" degrees Celcius, Humidity: ");
    Serial.print(h);
    Serial.println("%. Send to Thingspeak.");
  }
  client.stop();

  Serial.println("Waiting...");

  digitalWrite(DHTPOWER, LOW);
  
  // thingspeak needs minimum 15 sec delay between updates
  delay(60000);
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void readApiKeyFromEeprom(int offset) {
  for (int i = offset; i < SECRET_THINGSPEAK_API_KEY_LENGTH; i++ ) {
    apiKey[i] = EEPROM.read(i);
  }
  EEPROM.commit();
}

void writeApiKeyToEeprom(int offset) {
  for (int i = offset; i < SECRET_THINGSPEAK_API_KEY_LENGTH; i++ ) {
    EEPROM.write(i, apiKey[i]);
  }
  EEPROM.commit();
}
