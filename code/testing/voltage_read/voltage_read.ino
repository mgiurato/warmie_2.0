int sensorPin = A0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
}

void loop() {
  int sensorValue;
  for (int i = 0; i < 20 ; i++) {
    sensorValue = analogRead(sensorPin);
    delay(50);
  }
  
  float voltage = (float)sensorValue * 3.84 / 432;

  Serial.println(voltage);

  delay(500);
}
