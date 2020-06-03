#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define FACTORY_RESET 0
#define MODULE_TYPE "SM_%08X"

#define CHANNEL_ID_SIZE 7+1
#define WRITE_API_KEY_SIZE 16+1

#define BME_POWER_PIN D4
#define BME_GND_PIN D3
#define BME_I2C_ADDRESS 0x76

#define VOLTAGE_PIN A0
#define VOLTAGE_GAIN 4.2/1023.0
#define VOLTAGE_THRESHOLD 3.5
#define VOLTAGE_MEASURE_ITERATIONS 5

#define SERIAL_BAUD 115200
#define TIMEOUT_CONFIG_PORTAL 300 //seconds
#define REGULAR_TIMEOUT 300 //seconds
#define LOW_BATTERY_TIMEOUT 600 //seconds

char channel_id[CHANNEL_ID_SIZE];
char write_api_key[WRITE_API_KEY_SIZE];
const int temperature_field = 1;
const int humidity_field = 2;
const int pressure_field = 3;
const int battery_voltage_field = 4;
int statusCode;

bool shouldSaveConfig = false;

WiFiClient client;

Adafruit_BME280 bme;

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(SERIAL_BAUD);
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

  //set timeout for wifi connection trials
  wifiManager.setConfigPortalTimeout(TIMEOUT_CONFIG_PORTAL);

  //add all your parameters here
  wifiManager.addParameter(&custom_channel_id);
  wifiManager.addParameter(&custom_write_api_key);

  if (FACTORY_RESET) {
    //reset settings - for testing
    wifiManager.resetSettings();
  }

  char APssid[25];
  snprintf(APssid, 25, MODULE_TYPE, ESP.getChipId());

  if (!wifiManager.autoConnect(APssid)) {
    Serial.println("Failed to connect and hit timeout");
    ESP.deepSleep(REGULAR_TIMEOUT * 1000000);
  } else {
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

    ThingSpeak.begin(client);  // Initialize ThingSpeak

    pinMode(BME_POWER_PIN, OUTPUT);
    digitalWrite(BME_POWER_PIN, HIGH);
    pinMode(BME_GND_PIN, OUTPUT);
    digitalWrite(BME_GND_PIN, LOW);
    delay(500);

    bme.begin(BME_I2C_ADDRESS);

    float t = bme.readTemperature();
    float h = bme.readHumidity();
    float p = bme.readPressure() / 100.0F;

    digitalWrite(BME_POWER_PIN, LOW);
    delay(500);

    float v = 0.0;
    for (int i = 0; i < VOLTAGE_MEASURE_ITERATIONS; i++) {
      v = (float)analogRead(VOLTAGE_PIN) * VOLTAGE_GAIN + v;
      delay(100);
    }
    v = v / VOLTAGE_MEASURE_ITERATIONS;

    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" degrees Celsius, Humidity: ");
    Serial.print(h);
    Serial.print(" %, Pressure: ");
    Serial.print(p);
    Serial.print(" hPa, Battery voltage: ");
    Serial.print(v);
    Serial.println(" V. Send to Thingspeak.");

    ThingSpeak.setField(temperature_field, t);
    ThingSpeak.setField(humidity_field, h);
    ThingSpeak.setField(pressure_field, p);
    ThingSpeak.setField(battery_voltage_field, v);

    if ( v <= VOLTAGE_THRESHOLD )  {
      ThingSpeak.setStatus("Low battery!");
    } else {
      ThingSpeak.setStatus("Operative and running.");
    }

    statusCode = ThingSpeak.writeFields(atoi(channel_id), write_api_key);
    if (statusCode == 200) {
      Serial.println("Channel update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(statusCode));
    }

    if ( v <= VOLTAGE_THRESHOLD )  {
      ESP.deepSleep(LOW_BATTERY_TIMEOUT * 1000000);
    } else {
      ESP.deepSleep(REGULAR_TIMEOUT * 1000000);
    }
  }
}

void loop() {

}
