#include "max11270.h"

Max11270 maxim(10);

void setup() {
  Serial.begin();
  delay(3000);
  // put your setup code here, to run once:
 
  maxim.setConversionMode(MAX11270_CONTINOUS_CONVERSION_MODE);
  maxim.setGain(MAX11270_PGA_GAIN_32);
  maxim.performSelfCalibration();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(500);
  Serial.println(maxim.readMicroVolts(), 3);
}