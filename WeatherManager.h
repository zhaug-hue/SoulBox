#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

class WeatherManager {
public:
  WeatherManager();
  void begin();
  bool shouldUpdate() const;
  bool updateLocalSensor();
  void updateWeatherMock(const String &weather);

  float getTemperature() const;
  float getHumidity() const;
  String getWeatherType() const;

private:
  DHT _dht;
  float _temperature = 0;
  float _humidity = 0;
  String _weatherType = "Clouds";
  unsigned long _lastUpdateMs = 0;
  bool _mockWeatherMode = false;

  String classifyWeather(float temperature, float humidity) const;
};

#endif
