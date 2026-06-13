#include "WeatherManager.h"
#include "config.h"

WeatherManager::WeatherManager() : _dht(DHT_PIN, DHT11) {
}

void WeatherManager::begin() {
  _dht.begin();
}

bool WeatherManager::shouldUpdate() const {
  return millis() - _lastUpdateMs >= WEATHER_UPDATE_INTERVAL_MS;
}

bool WeatherManager::updateLocalSensor() {
  _lastUpdateMs = millis();

  const float temperature = _dht.readTemperature();
  const float humidity = _dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    return false;
  }

  _temperature = temperature;
  _humidity = humidity;
  if (!_mockWeatherMode) {
    _weatherType = classifyWeather(temperature, humidity);
  }
  return true;
}

void WeatherManager::updateWeatherMock(const String &weather) {
  _weatherType = weather;
  _mockWeatherMode = true;
  _lastUpdateMs = millis();
}

float WeatherManager::getTemperature() const {
  return _temperature;
}

float WeatherManager::getHumidity() const {
  return _humidity;
}

String WeatherManager::getWeatherType() const {
  return _weatherType;
}

String WeatherManager::classifyWeather(float temperature, float humidity) const {
  if (temperature >= 30) {
    return "Hot";
  }
  if (humidity >= 75) {
    return "Rain";
  }
  if (humidity <= 45) {
    return "Clear";
  }
  return "Clouds";
}
