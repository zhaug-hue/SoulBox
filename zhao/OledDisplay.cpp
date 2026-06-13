#include "OledDisplay.h"
#include "config.h"
#include <Wire.h>

OledDisplay::OledDisplay()
  : _display(U8G2_R0, U8X8_PIN_NONE) {
}

void OledDisplay::begin() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  _display.begin();
  _display.setFont(u8g2_font_6x10_tf);
}

void OledDisplay::showBootScreen() {
  _display.clearBuffer();
  _display.setFont(u8g2_font_7x13B_tf);
  _display.drawStr(10, 18, "SoulBox");
  _display.setFont(u8g2_font_6x10_tf);
  _display.drawStr(10, 36, "Booting...");
  _display.drawStr(10, 52, "Wi-Fi + RPG IoT");
  _display.sendBuffer();
}

void OledDisplay::update(const GameStatus &status) {
  const String signature = status.weatherType + "|" + String(status.temperature, 1) + "|" +
                           String(status.humidity, 1) + "|" + status.airQuality + "|" +
                           status.oledExpression;
  if (signature == _lastSignature) {
    return;
  }
  _lastSignature = signature;

  _display.clearBuffer();
  _display.setFont(u8g2_font_9x15B_tf);
  _display.drawStr(34, 16, status.oledExpression.c_str());
  _display.setFont(u8g2_font_6x10_tf);
  _display.drawStr(0, 34, ("W:" + status.weatherType).c_str());
  _display.drawStr(0, 48, ("T:" + String(status.temperature, 0) + "C H:" + String(status.humidity, 0) + "%").c_str());
  _display.drawStr(0, 62, ("AQ:" + status.airQuality).c_str());
  _display.sendBuffer();
}

String OledDisplay::faceForStatus(const GameStatus &status) const {
  if (status.gameState == "RESULT" && !status.monsterAlive) {
    return "(^o^)";
  }
  if (status.gameState == "RESULT" && status.playerHp <= 0) {
    return "(>_<)";
  }
  if (status.mood == "happy") {
    return "(^_^)";
  }
  if (status.mood == "hungry") {
    return "(T_T)";
  }
  return "(-_-)";
}

void OledDisplay::drawBar(int x, int y, int width, int height, int value, int maxValue) {
  _display.drawFrame(x, y, width, height);
  if (maxValue <= 0) {
    return;
  }
  const int fillWidth = map(constrain(value, 0, maxValue), 0, maxValue, 0, width - 2);
  _display.drawBox(x + 1, y + 1, fillWidth, height - 2);
}
