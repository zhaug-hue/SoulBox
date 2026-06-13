#ifndef HARDWARE_INPUT_H
#define HARDWARE_INPUT_H

#include <Arduino.h>

class HardwareInput {
public:
  void begin();
  bool consumeAttackEvent();

private:
  static void IRAM_ATTR onAttackButtonPressed();
  static volatile bool _attackFlag;
  unsigned long _lastAttackMs = 0;
};

#endif
