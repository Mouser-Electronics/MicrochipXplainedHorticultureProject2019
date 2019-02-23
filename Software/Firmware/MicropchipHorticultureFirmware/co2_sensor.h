#ifndef CO2_SENSOR_H
#define CO2_SENSOR_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif
#include "Wire.h"

class co2_sensor {
 public:
  co2_sensor();
  boolean begin(int co2pin);  // by default go highres
  float readCO2level();
  
 private:
  int co2analogPin;
};


#endif
