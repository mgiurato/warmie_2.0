#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <UniversalTelegramBot.h>

#define BOT_TOKEN_LENGTH 46

char botToken[BOT_TOKEN_LENGTH] = "";

WiFiClientSecure client;
UniversalTelegramBot *bot;

int Bot_mtbs = 1000; //mean time between scan messages
long Bot_lasttime;   //last time messages' scan has been done
int ledStatus;
int ledPin = LED_BUILTIN;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void readBotTokenFromEeprom(int offset) {
  for (int i = offset; i < BOT_TOKEN_LENGTH; i++ ) {
    botToken[i] = EEPROM.read(i);
  }
  EEPROM.commit();
}

void writeBotTokenToEeprom(int offset) {
  for (int i = offset; i < BOT_TOKEN_LENGTH; i++ ) {
    EEPROM.write(i, botToken[i]);
  }
  EEPROM.commit();
}

void handleNewMessages(int numNewMessages) {
  Serial.println("Handle new messages.");

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot->messages[i].chat_id);
    Serial.print("Chat ID:  ");
    Serial.println(chat_id);

    String text = bot->messages[i].text;
    Serial.print("Text:  ");
    Serial.println(text);

    if (text == "/ledon") {
      ledStatus = 1;
      digitalWrite(ledPin, LOW);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is active low on the ESP-01)
      bot->sendMessage(chat_id, "Led is ON", "");
    }
    if (text == "/ledoff") {
      ledStatus = 0;
      digitalWrite(ledPin, HIGH);  // Turn the LED off by making the voltage HIGH
      bot->sendMessage(chat_id, "Led is OFF", "");
    }
    if (text == "/status") {
      if (ledStatus) {
        bot->sendMessage(chat_id, "Led is ON", "");
      } else {
        bot->sendMessage(chat_id, "Led is OFF", "");
      }
    }
    if (text == "/start") {
      String welcome = "Welcome from FlashLedBot, your personal Bot on ESP8266\n";
      welcome = welcome + "/ledon : to switch the Led ON\n";
      welcome = welcome + "/ledoff : to switch the Led OFF\n";
      welcome = welcome + "/status : Returns current status of LED\n";
      bot->sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for the Serial monitor to be opened

  EEPROM.begin(BOT_TOKEN_LENGTH);
  pinMode(ledPin, OUTPUT); // initialize digital ledPin as an output.
  delay(10);
  digitalWrite(ledPin, HIGH); // initialize pin as off
  ledStatus = 0;

  Serial.println("read bot token");
  readBotTokenFromEeprom(0);
  Serial.println(botToken);

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  //Adding an additional config on the WIFI manager webpage for the bot token
  WiFiManagerParameter custom_bot_id("botid", "Bot Token", botToken, 50);
  wifiManager.addParameter(&custom_bot_id);
  wifiManager.autoConnect();

  strcpy(botToken, custom_bot_id.getValue());
  if (shouldSaveConfig) {
    writeBotTokenToEeprom(0);
  }

  bot = new UniversalTelegramBot(botToken, client);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  client.setInsecure();
  Serial.println("ESP8266 initialised");
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
