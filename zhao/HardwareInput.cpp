#include "HardwareInput.h"
#include "config.h"

volatile bool HardwareInput::_attackFlag = false;

void HardwareInput::begin() {
  pinMode(ATTACK_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ATTACK_BUTTON_PIN), onAttackButtonPressed, FALLING);
}

bool HardwareInput::consumeAttackEvent() {
  if (!_attackFlag) {
    return false;
  }

  noInterrupts();
  _attackFlag = false;
  interrupts();

  const unsigned long now = millis();
  if (now - _lastAttackMs < BUTTON_DEBOUNCE_MS) {
    return false;
  }

  _lastAttackMs = now;
  return true;
}

void IRAM_ATTR HardwareInput::onAttackButtonPressed() {
  _attackFlag = true;
}
