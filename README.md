# warmie_2.0
This repository contains all the code developed for a custom DIY wi-fi thermostat, codename: Warmie 2.0
The repository includes also sample code developed for the tesing of the single functionalities.
The code is developed using the Arduino IDE v1.8.12

Needed hardware
---------------
to do

How to install ESP8266 drivers on Arduino IDE
---------------------------------------------
Download and install v1.8.12 of the Arduino IDE by following the instructions at:
https://www.arduino.cc/en/Main/Software

The microcontroller boards requires the USB to UART driver to be installed in order to program it. 
Download and install the driver for your platform by following the instructions at:
https://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx

Launch the Arduino IDE.
In order for the board to show up as a board in Arduino, you must add the following URL to the “Additional Boards Manager URLs” field in the preferences (Arduino -> Preferences).
http://arduino.esp8266.com/stable/package_esp8266com_index.json

Restart the Arduino IDE.
Open the Board Manager at Tools -> Board -> Boards Manager. 
Change the Type field to Contributed and enter esp8266 in the Search field. 
Select the ESP8266 entry in the list, change the version to 2.6.3, and click the Install button.

Restart the Arduino IDE again. 
We can now configure the Arduino IDE to use the board we just installed.

Open the Arduino IDE, select the Tools menu, and change the Board to "NodeMCU 1.0 (ESP-12E Module)" or "LOLIN(WEMOS) D1 mini lite" according to the used board.

Installed libraries
-------------------
The code in this repository requires a few dependencies to be installed. 
The libraries can be installed using Arduino’s Library Manager. 
Open the manager from the Sketch -> Include Library -> Manage Libraries menu.

-	ArduinoJson library v5.13.5
-	UniversalTelegramBot v1.1.0
-	WiFiManager v0.15.0
-	Adafruit ADXL343 v1.2.0
-	Adafruit Unified Sensor v1.1.2
-	DHT sensor library v1.3.8
