// simplestesp8266clock.ino
//
// Libraries needed:
//  Time.h & TimeLib.h:  https://github.com/PaulStoffregen/Time
//  Timezone.h: https://github.com/JChristensen/Timezone
//  SSD1306.h & SSD1306Wire.h:  https://github.com/squix78/esp8266-oled-ssd1306
//  NTPClient.h: https://github.com/arduino-libraries/NTPClient
//  ESP8266WiFi.h & WifiUDP.h: https://github.com/ekstrand/ESP8266wifi
//
// 128x64 OLED pinout:
// GND goes to ground
// Vin goes to 3.3V
// Data to I2C SDA (GPIO 0)
// Clk to I2C SCL (GPIO 2)

#include <ESP8266WiFi.h>
#include <WifiUDP.h>
#include <String.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>

// Define NTP properties
#define NTP_OFFSET   0                  // In seconds 
#define NTP_INTERVAL 0                  // In milliseconds
#define NTP_ADDRESS  "it.pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)
// Both offset and interval are zero in order to get the UTC time

// Set up the NTP UDP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Jarvis";            // insert your own ssid
const char* password = "M91n93_2405!";  // and password
String date;
String t;
const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"} ;

void setup () {
  Serial.begin(115200); // most ESP-01's use 115200 but this could vary
  delay(100);
  Serial.println("UTC");

  timeClient.begin();   // Start the NTP UDP client
  delay(1000);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  //display.setRotation(2);
  //display.display();
  //delay(2000);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(1, 11);
  display.print("Hello");
  display.setTextSize(1);
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(0, 3);
  display.print("Hi handsome!");
  display.display();
  delay(4000);
  display.clearDisplay();

  // Connect to wifi
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.print(ssid);
  display.setCursor(0, 3);
  display.print("Connecting to Wifi...");
  display.display();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi at ");
  Serial.print(WiFi.localIP());
  Serial.println("");
  display.clearDisplay();
  display.setCursor(0, 3);
  display.print("Connected.");
  display.display();
  delay(1000);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    date = "";  // clear the variables
    t = "";

    // update the NTP client and get the UNIX UTC timestamp
    timeClient.update();
    unsigned long epochTime =  timeClient.getEpochTime();

    // convert received time stamp to time_t object
    time_t local, utc;
    utc = epochTime;

    // Then convert the UTC UNIX timestamp to local time
    TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, +2 * 60};   // Central European Summer Time
    TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, +1 * 60};    // Central European Standard Time
    Timezone CE(CEST, CET);
    local = CE.toLocal(utc);

    // now format the Time variables into strings with proper names for month, day etc
    date += days[weekday(local) - 1];
    date += ", ";
    date += months[month(local) - 1];
    date += " ";
    date += day(local);
    date += ", ";
    date += year(local);

    // format the time to 12-hour format with AM/PM and no seconds
    t += hourFormat12(local);
    t += ":";
    if (minute(local) < 10) // add a zero if minute is under 10
      t += "0";
    t += minute(local);
    t += " ";
    t += ampm[isPM(local)];

    // Display the date and time
    Serial.println("");
    Serial.print("Local date: ");
    Serial.print(date);
    Serial.println("");
    Serial.print("Local time: ");
    Serial.print(t);

    // print the date and time on the OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(1, 1);
    display.print(date);
    display.setTextSize(2);
    display.setCursor(20, 25);
    display.print(t);
    display.display();
  } else { // attempt to connect to wifi again if disconnected
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(15, 10);
    display.print("Connecting to Wifi...");
    display.display();
    WiFi.begin(ssid, password);
    display.setTextSize(1);
    display.setCursor(15, 10);
    display.print("Connected.");
    display.display();
    delay(1000);
  }

  delay(10000);    //Send a request to update every 10 sec (= 10,000 ms)
}
