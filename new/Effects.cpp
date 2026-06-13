#include "Effects.h"
#include "config.h"

void Effects::begin() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  setLedStatus(false);
}

void Effects::playAttackSound() {
  beep(1200, 60);
}

void Effects::playWinSound() {
  beep(900, 80);
  delay(40);
  beep(1400, 100);
}

void Effects::playWarningSound() {
  beep(450, 120);
}

void Effects::setLedStatus(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void Effects::beep(uint16_t frequency, uint16_t durationMs) {
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcAttach(BUZZER_PIN, frequency, 8);
  ledcWrite(BUZZER_PIN, 128);
  delay(durationMs);
  ledcWrite(BUZZER_PIN, 0);
#else
  ledcSetup(1, frequency, 8);
  ledcAttachPin(BUZZER_PIN, 1);
  ledcWrite(1, 128);
  delay(durationMs);
  ledcWrite(1, 0);
#endif
}
