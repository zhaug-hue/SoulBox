#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "GameEngine.h"

typedef void (*WebCommandCallback)(const String &command);

class WebManager {
public:
  WebManager();
  bool begin();
  void loop();
  void setCommandCallback(WebCommandCallback callback);
  void broadcastStatus(const GameStatus &status);

private:
  AsyncWebServer _server;
  AsyncWebSocket _socket;
  WebCommandCallback _commandCallback = nullptr;

  static WebManager *_instance;
  static void onSocketEvent(AsyncWebSocket *server,
                            AsyncWebSocketClient *client,
                            AwsEventType type,
                            void *arg,
                            uint8_t *data,
                            size_t len);

  void handleSocketData(uint8_t *data, size_t len);
  size_t statusToJson(const GameStatus &status, char *buffer, size_t bufferSize);
  void connectWiFi();
};

#endif
