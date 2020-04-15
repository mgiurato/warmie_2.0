#include "arduino_secrets.h"
#include <ESP8266WiFi.h>
#include <DHT.h>

// Wifi connection to the router
const char* ssid  = SECRET_SSID;
const char* password = SECRET_PASS;

// ThingSpeak
String apiKey = SECRET_THINGSPEAK_API_KEY;
const char* server = "api.thingspeak.com";

// DHT22
#define DHTPOWER D1
#define DHTPIN D2
#define DHTTYPE DHT22

// Connect pin 1 (on the left) of the sensor to D0
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND

DHT dht(DHTPIN, DHTTYPE);

WiFiClient client;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for the Serial monitor to be opened

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(DHTPOWER, OUTPUT);
  delay(500);
  digitalWrite(DHTPOWER, HIGH);
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
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
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
