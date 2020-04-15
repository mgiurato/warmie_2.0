#include <ESP8266WiFi.h>
#include <DHT.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

// ThingSpeak
#define READ_API_KEY_LENGTH 16
char apiKey[READ_API_KEY_LENGTH] = "";
const char* server = "api.thingspeak.com";

//flag for saving data
bool shouldSaveConfig = false;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for the Serial monitor to be opened

  EEPROM.begin(READ_API_KEY_LENGTH);
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

  digitalWrite(DHTPOWER, HIGH);
  delay(1000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float hic = dht.computeHeatIndex(t, h, false);

  if (isnan(h) || isnan(t) || isnan(hic))  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(t);
    postStr += "&field2=";
    postStr += String(h);
    postStr += "&field3=";
    postStr += String(hic);
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
    Serial.print("%, Apparent temperature: ");
    Serial.print(hic);
    Serial.println(" degrees Celcius. Send to Thingspeak.");
  }
  client.stop();

  digitalWrite(DHTPOWER, LOW);

  ESP.deepSleep(60e6);
}

void loop() {
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void readApiKeyFromEeprom(int offset) {
  for (int i = offset; i < READ_API_KEY_LENGTH; i++ ) {
    apiKey[i] = EEPROM.read(i);
  }
  EEPROM.commit();
}

void writeApiKeyToEeprom(int offset) {
  for (int i = offset; i < READ_API_KEY_LENGTH; i++ ) {
    EEPROM.write(i, apiKey[i]);
  }
  EEPROM.commit();
}
