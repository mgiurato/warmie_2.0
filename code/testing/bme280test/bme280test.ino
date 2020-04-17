/***************************************************************************
  This is a library for the BME280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BME280 Breakout
  ----> http://www.adafruit.com/products/2650

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface. The device's I2C address is either 0x76 or 0x77.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
  See the LICENSE file for details.
 ***************************************************************************/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C

void setup() {
  Serial.begin(115200);
  while (!Serial);   // time to get serial running

  bme.begin(0x76);

  Serial.println("-- Default Test --");

  Serial.println();
}


void loop() {
  float t = bme.readTemperature();
  float p = bme.readPressure() / 100.0F;
  float h = bme.readAltitude(SEALEVELPRESSURE_HPA);
  float rh = bme.readHumidity();
  float dp = dewPoint(t, rh);
  float hum = humidex(t, dp);
  float ah = absoluteHumidity(t, rh);

  Serial.print("Temperature = ");
  Serial.print(t);
  Serial.println(" °C");

  Serial.print("Pressure = ");
  Serial.print(p);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(h);
  Serial.println(" m");

  Serial.print("Relative humidity = ");
  Serial.print(rh);
  Serial.println(" %");

  Serial.print("Dew point = ");
  Serial.print(dp);
  Serial.println(" %");

  Serial.print("Humidex = ");
  Serial.print(hum);
  Serial.println(" °C");

  Serial.print("Absolute humidity = ");
  Serial.print(hum);
  Serial.println(" g/m^3");


  Serial.println();

  delay(1000);
}

float dewPoint(float T, float RH) {
  // reference: https://en.wikipedia.org/wiki/Dew_point
  // These valuations provide a maximum error of 0.1%, for −30 °C ≤ T ≤ 35°C and 1% < RH < 100%
  float b = 17.67;
  float c = 243.5;
  
  float gamma = (b * T) / (c + T) + log(RH / 100.0);
  float Tdp = (c * gamma) / (b - gamma);
  
  return Tdp;
}

float humidex(float T, float Tdp) {
  // reference: https://en.wikipedia.org/wiki/Humidex
  float e = 6.11 * exp(5417.7530 * (1.0 / 273.16 - 1.0 / (273.155 + Tdp)));
  float H = T + 5.0/9.0 * ( e - 10.0);
  
  return H;
}

float absoluteHumidity(float T, float RH) {
  // reference: https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/
  // This formula is accurate to within 0.1% over the temperature range –30°C to +35°C
  float mw = 18.01528;    // molar mass of water g/mol
  float R = 8.314462618;  // Universal gas constant J/mol/K

  float temp = exp((17.67 * T) / (T + 243.5));
  float AH = (6.112 * temp * RH * mw) / ((273.15 + T) * R);
  
  return AH;
}
