#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "splashscreen.h"

#define FACTORY_RESET 0
#define MODULE_TYPE "Display module"

#define CHANNEL_ID_SIZE 7+1
#define READ_API_KEY_SIZE 16+1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

char channel_id[CHANNEL_ID_SIZE];
char read_api_key[READ_API_KEY_SIZE];
const int temperature_field = 1;
const int humidity_field = 2;
const int pressure_field = 3;
const int battery_voltage_field = 4;
int statusCode;

bool shouldSaveConfig = false;

WiFiClient client;

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.drawBitmap(0,0, splashscreen, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
  display.setCursor(1, 1);
  display.print("ESP8266 set-up...");
  display.display();

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
          strcpy(read_api_key, json["read_api_key"]);
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
  WiFiManagerParameter custom_read_api_key("read_api_key", "Read API key", read_api_key, READ_API_KEY_SIZE);

  //WiFiManager
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_channel_id);
  wifiManager.addParameter(&custom_read_api_key);

  if (FACTORY_RESET) {
    //reset settings - for testing
    wifiManager.resetSettings();
  }

  wifiManager.autoConnect(MODULE_TYPE);

  //read updated parameters
  strcpy(channel_id, custom_channel_id.getValue());
  strcpy(read_api_key, custom_read_api_key.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["channel_id"] = channel_id;
    json["read_api_key"] = read_api_key;

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

  ThingSpeak.begin(client);  // Initialize ThingSpeak

  Serial.println("ESP8266 initialised!");
  
  display.clearDisplay();
  display.drawBitmap(0,0, splashscreen, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  display.setCursor(1, 1);
  display.print("ESP8266 initialised!");
  display.display();
}

void loop() {
  float t = ThingSpeak.readFloatField(atoi(channel_id), temperature_field, read_api_key);
  statusCode = ThingSpeak.getLastReadStatus();
  if (statusCode == 200) {
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" degrees Celcius.");
  } else {
    Serial.println("Unable to read channel / No internet connection");
  }
  delay(100);

  float h = ThingSpeak.readFloatField(atoi(channel_id), humidity_field, read_api_key);
  statusCode = ThingSpeak.getLastReadStatus();
  if (statusCode == 200) {
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.println("%.");
  } else {
    Serial.println("Unable to read channel / No internet connection");
  }
  delay(100);
  
  displayTemperature(t);
  delay(3000);
  displayHumidity(h);
  delay(3000);
  
  delay(60000);
}

void displayTemperature(float temperature) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(29, 10);
  display.print(temperature);
  display.setCursor(90, 3);
  display.setTextSize(1);
  display.print("o");
  display.setCursor(97, 10);
  display.setTextSize(2);
  display.print("C");
  display.display();
}

void displayHumidity(float humidity) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(29, 10);
  display.print(humidity);
  display.setCursor(92, 10);
  display.print("%");
  display.display();
}
