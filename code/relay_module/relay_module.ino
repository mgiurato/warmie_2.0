#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define FACTORY_RESET 0
#define MODULE_TYPE "Relay module"

#define BOT_TOKEN_SIZE 47+1
#define USER_ID_SIZE 8+1

#define RELAY_PIN D1

WiFiClientSecure bclient;
UniversalTelegramBot *bot;
char bot_token[BOT_TOKEN_SIZE];
char user_id[USER_ID_SIZE];
int Bot_mtbs = 1000; //mean time between scan messages
long Bot_lasttime = 0;   //last time messages' scan has been done

bool relay_on;

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

          strcpy(bot_token, json["bot_token"]);
          strcpy(user_id, json["user_id"]);
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

  WiFiManagerParameter custom_bot_token("bot_token", "Bot token", bot_token, BOT_TOKEN_SIZE);
  WiFiManagerParameter custom_user_id("user_id", "User ID", user_id, USER_ID_SIZE);

  //WiFiManager
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_bot_token);
  wifiManager.addParameter(&custom_user_id);

  if (FACTORY_RESET) {
    //reset settings - for testing
    wifiManager.resetSettings();
  }

  wifiManager.autoConnect(MODULE_TYPE);

  //read updated parameters
  strcpy(bot_token, custom_bot_token.getValue());
  strcpy(user_id, custom_user_id.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["bot_token"] = bot_token;
    json["user_id"] = user_id;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }
  //end save

  bclient.setInsecure();
  bot = new UniversalTelegramBot(bot_token, bclient);

  Serial.println();
  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  relay_on = false;

  Serial.println("ESP8266 initialised!");
}

void loop() {
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages = bot->getUpdates(bot->last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    }

    Bot_lasttime = millis();
  }
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot->messages[i].chat_id);
    String from_id = String(bot->messages[i].from_id);
    String text = bot->messages[i].text;
    String from_name = bot->messages[i].from_name;
    String message = "";

    if (from_name == "") {
      from_name = "Guest";
    }

    Serial.println("Cname=" + from_name + " - ChatID=" + chat_id + " - FromID=" + from_id + " - Text=" + text);

    if (from_id == user_id) {
      Serial.println(from_id + "<- Allowed! :)");
      if (text == "/status") {
        if (relay_on) {
          message = "The relay is on\n";
        } else {
          message = "The relay is off\n";
        }
      } else if (text == "/on") {
        digitalWrite(RELAY_PIN, HIGH);
        relay_on = true;
        message = "The relay has been turned on\n";
      } else if (text == "/off") {
        digitalWrite(RELAY_PIN, LOW);
        relay_on = false;
        message = "The relay has been turned off\n";
      } else if (text == "/start" || text == "/help") {
        message = "Welcome to the WARMIE 2.0 bot interface, " + from_name + ".\n";
        message += "These are the commands you can use:\n\n";
        message += "/status : to get the status of the relay\n";
        message += "/on : to turn on the relay\n";
        message += "/off : to turn off the relay\n";
      } else {
        message = "I cannot understand what you are saying! 😖";
      }
    } else {
      Serial.println(from_id + "<- NOT allowed! D:");
      message = "Sorry, I do not know who you are! 😜\n";
    }

    bot->sendChatAction(chat_id, "typing");
    delay(100);
    bot->sendMessage(chat_id, message);
  }
}
