#include "co2_sensor.h"

co2_sensor::co2_sensor() {
}


boolean co2_sensor::begin(int co2pin) {
  co2analogPin = co2pin;
  pinMode(co2analogPin, INPUT);
  return true;
}


float co2_sensor::readCO2level() {
  float voltageDiference = 0.0;
  float concentration = 0.0;
  float voltage = 0.0;
  
  int adcVal = analogRead(co2analogPin);
  voltage = adcVal*(3.3/4095.0);

  if(voltage == 0.0) {
    Serial.println("A problem has occurred with the sensor.");
  }
  else if(voltage < 0.4) {
    Serial.println("Pre-heating the sensor...");
  }
  else {
    float voltageDiference=voltage-0.4;
    float concentration=(voltageDiference*5000.0)/1.6;

    Serial.print("voltage:");
    Serial.print(voltage);
    Serial.println("V");
    Serial.print(concentration);
    Serial.println("ppm");
  }

  delay(2000);
  return concentration;
}
