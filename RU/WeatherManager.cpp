#include "WeatherManager.h"
#include "config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

WeatherManager::WeatherManager() : _dht(DHT_PIN, DHT11) {
}

void WeatherManager::begin() {
  _dht.begin();
}

bool WeatherManager::shouldUpdateLocal() const {
  return millis() - _lastLocalUpdateMs >= WEATHER_UPDATE_INTERVAL_MS;
}

bool WeatherManager::shouldUpdateAPI() const {
  return _lastApiUpdateMs == 0 || millis() - _lastApiUpdateMs >= API_UPDATE_INTERVAL_MS;
}

bool WeatherManager::updateLocalSensor() {
  _lastLocalUpdateMs = millis();

  const float temperature = _dht.readTemperature();
  const float humidity = _dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    return false;
  }

  _temperature = temperature;
  _humidity = humidity;
  
  if (!_mockWeatherMode && _lastApiUpdateMs == 0) {
    _weatherType = classifyWeather(temperature, humidity);
  }
  return true;
}

bool WeatherManager::updateFromAPI() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(OPENWEATHER_LAT) + "&lon=" + String(OPENWEATHER_LON) + "&appid=" + String(OPENWEATHER_API_KEY) + "&units=metric";
  
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      String apiMain = doc["weather"][0]["main"].as<String>();
      String locationName = doc["name"].as<String>();
      
      if (!_mockWeatherMode) {
        _weatherType = mapApiWeather(apiMain, _temperature);
      }
      
      // 擷取擴充氣象資料
      _extTemperature = doc["main"]["temp"] | _temperature; // 儲存外部溫度
      _windSpeed = doc["wind"]["speed"] | 0.0f;
      int deg = doc["wind"]["deg"] | 0;
      _windDir = degreesToDirection(deg);
      _pressure = doc["main"]["pressure"] | 0;
      _visibility = (doc["visibility"] | 0) / 1000.0f;
      
      float extHum = doc["main"]["humidity"] | _humidity;
      _dewPoint = calculateDewPoint(_extTemperature, extHum); // 使用外部溫度計算露點
      
      _lastApiUpdateMs = millis();
      http.end();
      
      Serial.println("成功取得天氣 API!");
      Serial.println("地點定位: " + locationName);
      Serial.println("API原始天氣: " + apiMain + " -> 轉換為遊戲Buff: " + _weatherType);
      
      return true;
    } else {
      Serial.println("Weather JSON 解析失敗");
    }
  } else {
    Serial.println("Weather API 連線失敗，HTTP 狀態碼: " + String(httpCode));
  }
  
  http.end();
  return false;
}

void WeatherManager::updateWeatherMock(const String &weather) {
  _weatherType = weather;
  _mockWeatherMode = true;
  _lastLocalUpdateMs = millis();
}

float WeatherManager::getTemperature() const { return _temperature; }
float WeatherManager::getHumidity() const { return _humidity; }
String WeatherManager::getWeatherType() const { return _weatherType; }

float WeatherManager::getExtTemperature() const { return _extTemperature; }
float WeatherManager::getWindSpeed() const { return _windSpeed; }
String WeatherManager::getWindDir() const { return _windDir; }
int WeatherManager::getPressure() const { return _pressure; }
float WeatherManager::getVisibility() const { return _visibility; }
float WeatherManager::getDewPoint() const { return _dewPoint; }

String WeatherManager::classifyWeather(float temperature, float humidity) const {
  if (temperature >= 30) return "Hot";
  if (humidity >= 75) return "Rain";
  if (humidity <= 45) return "Clear";
  return "Clouds";
}

String WeatherManager::mapApiWeather(const String &apiMain, float currentTemp) const {
  if (apiMain == "Thunderstorm") return "Thunderstorm";
  if (apiMain == "Rain" || apiMain == "Drizzle") return "Rain";
  if (apiMain == "Clear") {
    if (currentTemp >= 30.0) return "Hot"; 
    return "Clear";
  }
  return "Clouds";
}

String WeatherManager::degreesToDirection(int deg) const {
  const char* directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  int val = (int)((deg / 22.5) + 0.5);
  return directions[val % 16];
}

float WeatherManager::calculateDewPoint(float temp, float hum) const {
  float a = 17.27;
  float b = 237.7;
  float alpha = ((a * temp) / (b + temp)) + log(hum / 100.0);
  return (b * alpha) / (a - alpha);
}
