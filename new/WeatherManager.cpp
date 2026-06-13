#include "WeatherManager.h"
#include "config.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <math.h>

WeatherManager::WeatherManager() : _dht(DHT_PIN, DHT11) {
}

void WeatherManager::begin() {
  _dht.begin();
}

bool WeatherManager::shouldUpdate() const {
  return shouldUpdateLocal();
}

bool WeatherManager::shouldUpdateLocal() const {
  return millis() - _lastUpdateMs >= WEATHER_UPDATE_INTERVAL_MS;
}

bool WeatherManager::shouldUpdateAPI() const {
  return _lastApiUpdateMs == 0 || millis() - _lastApiUpdateMs >= API_UPDATE_INTERVAL_MS;
}

bool WeatherManager::shouldUpdateAirQuality() const {
  return _lastAirQualityUpdateMs == 0 || millis() - _lastAirQualityUpdateMs >= AIR_QUALITY_UPDATE_INTERVAL_MS;
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

bool WeatherManager::updateFromAPI() {
  _lastApiUpdateMs = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Weather API skipped: Wi-Fi is not connected.");
    return false;
  }

  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(OPENWEATHER_LAT) +
               "&lon=" + String(OPENWEATHER_LON) +
               "&appid=" + String(OPENWEATHER_API_KEY) +
               "&units=metric";

  http.begin(url);
  const int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.println("Weather API failed. HTTP code: " + String(httpCode));
    http.end();
    return false;
  }

  const String payload = http.getString();
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, payload);
  http.end();

  if (error) {
    Serial.println("Weather API JSON parse failed.");
    return false;
  }

  const String apiMain = doc["weather"][0]["main"].as<String>();
  const float apiTemp = doc["main"]["temp"] | _temperature;
  const float apiHumidity = doc["main"]["humidity"] | _humidity;
  const int windDeg = doc["wind"]["deg"] | 0;
  const String locationName = doc["name"].as<String>();

  _temperature = apiTemp;
  _humidity = apiHumidity;
  _extTemperature = apiTemp;
  _windSpeed = doc["wind"]["speed"] | 0.0f;
  _windDir = degreesToDirection(windDeg);
  _pressure = doc["main"]["pressure"] | 0;
  _visibility = (doc["visibility"] | 0) / 1000.0f;
  _dewPoint = calculateDewPoint(apiTemp, apiHumidity);
  _weatherType = mapApiWeather(apiMain, apiTemp);
  _mockWeatherMode = false;

  Serial.println("Weather API updated.");
  Serial.println("Location: " + locationName);
  Serial.println("OpenWeather: " + apiMain + " -> SoulBox: " + _weatherType);
  return true;
}

bool WeatherManager::updateAirQualityFromAPI() {
  _lastAirQualityUpdateMs = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Air quality API skipped: Wi-Fi is not connected.");
    return false;
  }

  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/air_pollution?lat=" + String(OPENWEATHER_LAT) +
               "&lon=" + String(OPENWEATHER_LON) +
               "&appid=" + String(OPENWEATHER_API_KEY);

  http.begin(url);
  const int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.println("Air quality API failed. HTTP code: " + String(httpCode));
    http.end();
    return false;
  }

  const String payload = http.getString();
  StaticJsonDocument<768> doc;
  DeserializationError error = deserializeJson(doc, payload);
  http.end();

  if (error) {
    Serial.println("Air quality API JSON parse failed.");
    return false;
  }

  const int aqi = doc["list"][0]["main"]["aqi"] | 1;
  _airQuality = mapAirQuality(aqi);
  Serial.println("Air quality API updated: " + _airQuality);
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

String WeatherManager::getAirQuality() const {
  return _airQuality;
}

float WeatherManager::getExtTemperature() const {
  return _extTemperature;
}

float WeatherManager::getWindSpeed() const {
  return _windSpeed;
}

String WeatherManager::getWindDir() const {
  return _windDir;
}

int WeatherManager::getPressure() const {
  return _pressure;
}

float WeatherManager::getVisibility() const {
  return _visibility;
}

float WeatherManager::getDewPoint() const {
  return _dewPoint;
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

String WeatherManager::mapApiWeather(const String &apiMain, float currentTemp) const {
  if (apiMain == "Thunderstorm") {
    return "Thunderstorm";
  }
  if (apiMain == "Rain" || apiMain == "Drizzle") {
    return "Rain";
  }
  if (apiMain == "Clear") {
    return currentTemp >= 30.0 ? "Hot" : "Clear";
  }
  return "Clouds";
}

String WeatherManager::degreesToDirection(int deg) const {
  const char *directions[] = {
    "N", "NNE", "NE", "ENE",
    "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW",
    "W", "WNW", "NW", "NNW"
  };
  const int index = (int)((deg / 22.5f) + 0.5f) % 16;
  return directions[index];
}

float WeatherManager::calculateDewPoint(float temperature, float humidity) const {
  if (humidity <= 0) {
    return 0;
  }

  const float a = 17.27f;
  const float b = 237.7f;
  const float alpha = ((a * temperature) / (b + temperature)) + log(humidity / 100.0f);
  return (b * alpha) / (a - alpha);
}

String WeatherManager::mapAirQuality(int aqi) const {
  if (aqi <= 2) {
    return "Good";
  }
  if (aqi == 3) {
    return "Moderate";
  }
  return "Poor";
}
