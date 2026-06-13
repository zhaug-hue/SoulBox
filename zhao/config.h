#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Fill these in before uploading to the ESP32.
const char WIFI_SSID[] = "TTT";
const char WIFI_PASSWORD[] = "0o0o0o0o0o";

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

// OpenWeather API settings. Replace the API key with your own key before demoing publicly.
const char OPENWEATHER_API_KEY[] = "5be66be8579cd1d6623cf13e0cded2df";
const char OPENWEATHER_LAT[] = "23.702613";
const char OPENWEATHER_LON[] = "120.429490";
const unsigned long API_UPDATE_INTERVAL_MS = 600000;

#endif
