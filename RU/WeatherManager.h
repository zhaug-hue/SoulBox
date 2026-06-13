#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

class WeatherManager {
public:
  WeatherManager();
  void begin();
  bool shouldUpdateLocal() const;    // 判斷是否該讀取 DHT11
  bool shouldUpdateAPI() const;      // 判斷是否該抓取 OpenWeather API
  bool updateLocalSensor();
  bool updateFromAPI();              // 透過經緯度抓取真實天氣封包
  void updateWeatherMock(const String &weather);

  float getTemperature() const;
  float getHumidity() const;
  String getWeatherType() const;
  float getWindSpeed() const;
  String getWindDir() const;
  int getPressure() const;
  float getVisibility() const;
  float getDewPoint() const;
  float getExtTemperature() const;

private:
  DHT _dht;
  float _temperature = 0;
  float _humidity = 0;
  String _weatherType = "Clouds";
  unsigned long _lastLocalUpdateMs = 0; // 本地 DHT11 上次更新時間
  unsigned long _lastApiUpdateMs = 0;   // 外部 API 上次更新時間
  bool _mockWeatherMode = false;
  float _windSpeed = 0;
  String _windDir = "-";
  int _pressure = 0;
  float _visibility = 0;
  float _dewPoint = 0;
  float _extTemperature = 0;

  String degreesToDirection(int deg) const;
  float calculateDewPoint(float temp, float hum) const;

  String classifyWeather(float temperature, float humidity) const;
  String mapApiWeather(const String &apiMain, float currentTemp) const; // 將 API 天氣映射至遊戲 Buff
};

#endif
