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

// 更改原本的 shouldUpdate 名稱，用來區分本機感測與外部 API
bool WeatherManager::shouldUpdateLocal() const {
  return millis() - _lastLocalUpdateMs >= WEATHER_UPDATE_INTERVAL_MS;
}

// 新增：判斷是否該抓 API
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
  
  // 只有在非模擬模式，且 API 還沒抓到資料時，才用 DHT11 決定天氣 Buff
  if (!_mockWeatherMode && _lastApiUpdateMs == 0) {
    _weatherType = classifyWeather(temperature, humidity);
  }
  return true;
}

// 新增：抓取 OpenWeather API 的主邏輯
bool WeatherManager::updateFromAPI() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  // 使用虎尾科大的經緯度
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
      
      _lastApiUpdateMs = millis();
      http.end();
      
      // 印出 Log 方便從 Serial Monitor 確認是否有抓對
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
  _lastLocalUpdateMs = millis(); // 修正變數名稱
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

// 新增：將 OpenWeather 回傳的狀態轉成 SoulBox 的五種天氣 Buff
String WeatherManager::mapApiWeather(const String &apiMain, float currentTemp) const {
  if (apiMain == "Thunderstorm") return "Thunderstorm";
  if (apiMain == "Rain" || apiMain == "Drizzle") return "Rain";
  if (apiMain == "Clear") {
    if (currentTemp >= 30.0) return "Hot"; 
    return "Clear";
  }
  return "Clouds";
}
