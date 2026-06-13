#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Fill these in before uploading to the ESP32.
const char WIFI_SSID[] = "wifi名稱";
const char WIFI_PASSWORD[] = "密碼";

const uint8_t OLED_SDA_PIN = 21;
const uint8_t OLED_SCL_PIN = 22;
const uint8_t DHT_PIN = 27;
const uint8_t ATTACK_BUTTON_PIN = 32;
const uint8_t BUZZER_PIN = 25;
const uint8_t LED_PIN = 26;

const uint16_t WEB_PORT = 80;
const unsigned long WEATHER_UPDATE_INTERVAL_MS = 5000;
const unsigned long STATUS_BROADCAST_INTERVAL_MS = 1000;
const unsigned long BUTTON_DEBOUNCE_MS = 80;

const int PLAYER_MAX_HP = 100;
const int MONSTER_NORMAL_HP = 100;
const int MONSTER_BOSS_HP = 180;
const int SATIETY_MAX = 10;
const int SATIETY_MIN = 0;

#endif
