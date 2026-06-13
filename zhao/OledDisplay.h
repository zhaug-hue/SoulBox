#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "GameEngine.h"

class OledDisplay {
public:
  OledDisplay();
  void begin();
  void showBootScreen();
  void showNetworkInfo(const String &url, bool qrAvailable);
  void showWeatherAlert(const GameStatus &status);
  void update(const GameStatus &status);

private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C _display;
  String _lastSignature;

  String faceForStatus(const GameStatus &status) const;
  void drawBar(int x, int y, int width, int height, int value, int maxValue);
};

#endif
