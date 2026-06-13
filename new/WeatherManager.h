#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

class WeatherManager {
public:
  WeatherManager();
  void begin();
  bool shouldUpdate() const;
  bool shouldUpdateLocal() const;
  bool shouldUpdateAPI() const;
  bool shouldUpdateAirQuality() const;
  bool updateLocalSensor();
  bool updateFromAPI();
  bool updateAirQualityFromAPI();
  void updateWeatherMock(const String &weather);

  float getTemperature() const;
  float getHumidity() const;
  String getWeatherType() const;
  String getAirQuality() const;
  float getExtTemperature() const;
  float getWindSpeed() const;
  String getWindDir() const;
  int getPressure() const;
  float getVisibility() const;
  float getDewPoint() const;

private:
  DHT _dht;
  float _temperature = 0;
  float _humidity = 0;
  String _weatherType = "Clouds";
  unsigned long _lastUpdateMs = 0;
  unsigned long _lastApiUpdateMs = 0;
  unsigned long _lastAirQualityUpdateMs = 0;
  bool _mockWeatherMode = false;
  String _airQuality = "Good";
  float _extTemperature = 0;
  float _windSpeed = 0;
  String _windDir = "-";
  int _pressure = 0;
  float _visibility = 0;
  float _dewPoint = 0;

  String classifyWeather(float temperature, float humidity) const;
  String mapApiWeather(const String &apiMain, float currentTemp) const;
  String degreesToDirection(int deg) const;
  float calculateDewPoint(float temperature, float humidity) const;
  String mapAirQuality(int aqi) const;
};

#endif
