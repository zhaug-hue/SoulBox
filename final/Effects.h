#ifndef EFFECTS_H
#define EFFECTS_H

#include <Arduino.h>

class Effects {
public:
  void begin();
  void playAttackSound();
  void playWinSound();
  void playWarningSound();
  void setLedStatus(bool on);

private:
  void beep(uint16_t frequency, uint16_t durationMs);
};

#endif
