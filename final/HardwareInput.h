#ifndef HARDWARE_INPUT_H
#define HARDWARE_INPUT_H

#include <Arduino.h>

class HardwareInput {
public:
  void begin();
  bool consumeWeatherAlertEvent();

private:
  static void IRAM_ATTR onWeatherButtonPressed();
  static volatile bool _weatherAlertFlag;
  unsigned long _lastWeatherAlertMs = 0;
};

#endif
