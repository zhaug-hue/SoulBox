  #include "WebManager.h"
#include "config.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <string.h>

WebManager *WebManager::_instance = nullptr;

WebManager::WebManager()
  : _server(WEB_PORT), _socket("/ws") {
  _instance = this;
}

bool WebManager::begin() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed.");
    return false;
  }

  connectWiFi();

  _server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
  _server.serveStatic("/favicon.ico", SPIFFS, "/www/favicon.ico");
  _server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  _socket.onEvent(onSocketEvent);
  _server.addHandler(&_socket);
  _server.begin();

  Serial.println("SoulBox dashboard ready.");
  Serial.print("Open http://");
  Serial.println(WiFi.localIP());
  return true;
}

void WebManager::loop() {
  _socket.cleanupClients();
}

void WebManager::setCommandCallback(WebCommandCallback callback) {
  _commandCallback = callback;
}

void WebManager::broadcastStatus(const GameStatus &status) {
  char buffer[1536];
  memset(buffer, 0, sizeof(buffer));
  const size_t length = statusToJson(status, buffer, sizeof(buffer));
  if (length > 0) {
    _socket.textAll(buffer);
  }
}

void WebManager::onSocketEvent(AsyncWebSocket *server,
                               AsyncWebSocketClient *client,
                               AwsEventType type,
                               void *arg,
                               uint8_t *data,
                               size_t len) {
  if (!_instance) {
    return;
  }

  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client %u connected.\n", client->id());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client %u disconnected.\n", client->id());
      break;
    case WS_EVT_DATA:
      _instance->handleSocketData(data, len);
      break;
    default:
      break;
  }
}

void WebManager::handleSocketData(uint8_t *data, size_t len) {
  StaticJsonDocument<160> doc;
  DeserializationError error = deserializeJson(doc, data, len);
  if (error) {
    Serial.println("Invalid WebSocket JSON.");
    return;
  }

  const char *command = doc["cmd"] | "";
  if (_commandCallback && strlen(command) > 0) {
    _commandCallback(String(command));
  }
}

size_t WebManager::statusToJson(const GameStatus &status, char *buffer, size_t bufferSize) {
  StaticJsonDocument<1280> doc;
  doc["playerHp"] = status.playerHp;
  doc["playerMaxHp"] = status.playerMaxHp;
  doc["monsterHp"] = status.monsterHp;
  doc["monsterMaxHp"] = status.monsterMaxHp;
  doc["monsterName"] = status.monsterName;
  doc["satiety"] = status.satiety;
  doc["dayCount"] = status.dayCount;
  doc["expressionCounter"] = status.expressionCounter;
  doc["role"] = status.role;
  doc["mood"] = status.mood;
  doc["moodFace"] = status.moodFace;
  doc["webExpression"] = status.webExpression;
  doc["oledExpression"] = status.oledExpression;
  doc["state"] = status.gameState;
  doc["gameState"] = status.gameState;
  doc["weather"] = status.weatherType;
  doc["weatherType"] = status.weatherType;
  doc["buff"] = status.buffName;
  doc["weatherBuffName"] = status.buffName;
  doc["weatherMultiplier"] = status.weatherMultiplier;
  doc["satietyBuffName"] = status.satietyBuffName;
  doc["airQuality"] = status.airQuality;
  doc["environmentAdvice"] = status.environmentAdvice;
  doc["story"] = status.story;
  doc["lastWeatherApiUpdate"] = status.lastWeatherApiUpdate;
  doc["lastAirQualityUpdate"] = status.lastAirQualityUpdate;
  doc["showTutorial"] = status.showTutorial;
  doc["showWeatherBuff"] = status.showWeatherBuff;
  doc["showEnvironmentAdvice"] = status.showEnvironmentAdvice;
  doc["showFirstStory"] = status.showFirstStory;
  doc["showSatietyWarning"] = status.showSatietyWarning;
  doc["weatherBuffModalPending"] = status.weatherBuffModalPending;
  doc["temperature"] = status.temperature;
  doc["humidity"] = status.humidity;
  doc["inBattle"] = status.inBattle;
  doc["monsterAlive"] = status.monsterAlive;
  doc["bossBattle"] = status.bossBattle;
  doc["isBoss"] = status.bossBattle;
  doc["event"] = status.lastEvent;
  doc["lastEvent"] = status.lastEvent;

  JsonArray skills = doc.createNestedArray("skills");
  for (int i = 0; i < status.skillCount; i++) {
    skills.add(status.skills[i]);
  }

  return serializeJson(doc, buffer, bufferSize);
}

void WebManager::connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
