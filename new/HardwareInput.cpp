#include "HardwareInput.h"
#include "config.h"

volatile bool HardwareInput::_weatherAlertFlag = false;

void HardwareInput::begin() {
  pinMode(WEATHER_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WEATHER_BUTTON_PIN), onWeatherButtonPressed, FALLING);
}

bool HardwareInput::consumeWeatherAlertEvent() {
  if (!_weatherAlertFlag) {
    return false;
  }

  noInterrupts();
  _weatherAlertFlag = false;
  interrupts();

  const unsigned long now = millis();
  if (now - _lastWeatherAlertMs < BUTTON_DEBOUNCE_MS) {
    return false;
  }

  _lastWeatherAlertMs = now;
  return true;
}

void IRAM_ATTR HardwareInput::onWeatherButtonPressed() {
  _weatherAlertFlag = true;
}
