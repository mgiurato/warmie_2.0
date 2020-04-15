#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

#define FACTORY_RESET 0
#define MODULE_TYPE "Sensor module"
#define CHANNEL_ID_SIZE 7+1
#define WRITE_API_KEY_SIZE 16+1

char channel_id[CHANNEL_ID_SIZE];
char write_api_key[WRITE_API_KEY_SIZE];

bool shouldSaveConfig = false;

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println();

  if (FACTORY_RESET) {
    //clean FS, for testing
    SPIFFS.format();
  }

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(channel_id, json["channel_id"]);
          strcpy(write_api_key, json["write_api_key"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  WiFiManagerParameter custom_channel_id("channel_id", "Channel ID", channel_id, CHANNEL_ID_SIZE);
  WiFiManagerParameter custom_write_api_key("write_api_key", "Write API key", write_api_key, WRITE_API_KEY_SIZE);

  //WiFiManager
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_channel_id);
  wifiManager.addParameter(&custom_write_api_key);

  if (FACTORY_RESET) {
    //reset settings - for testing
    wifiManager.resetSettings();
  }

  wifiManager.autoConnect(MODULE_TYPE);

  //read updated parameters
  strcpy(channel_id, custom_channel_id.getValue());
  strcpy(write_api_key, custom_write_api_key.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["channel_id"] = channel_id;
    json["write_api_key"] = write_api_key;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }
  //end save
  
  Serial.println();
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}

void loop() {
}
