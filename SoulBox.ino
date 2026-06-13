#include "config.h"
#include "GameEngine.h"
#include "HardwareInput.h"
#include "WeatherManager.h"
#include "OledDisplay.h"
#include "WebManager.h"
#include "Effects.h"

GameEngine game;
HardwareInput hardware;
WeatherManager weather;
OledDisplay oled;
WebManager web;
Effects effects;

unsigned long lastBroadcastMs = 0;

void publishStatus(bool force);
void handleWebCommand(const String &command);

void handleWebCommand(const String &command) {
  if (command == "web_attack") {
    game.normalAttack();
    effects.playAttackSound();
  } else if (command == "skill_1") {
    game.useSkill(0);
    effects.playAttackSound();
  } else if (command == "skill_2") {
    game.useSkill(1);
    effects.playAttackSound();
  } else if (command == "skill_3") {
    game.useSkill(2);
    effects.playAttackSound();
  } else if (command == "feed") {
    game.feedPet();
  } else if (command == "call_boss") {
    game.callBoss();
  } else if (command == "new_day") {
    game.initDailyStatus();
  } else if (command == "check_environment") {
    game.checkEnvironment();
  } else if (command == "weather_api_update") {
    game.simulateWeatherApiUpdate();
  } else if (command == "clear_modals") {
    game.clearModalFlags();
  } else if (command == "weather_rain") {
    weather.updateWeatherMock("Rain");
    game.applyWeatherBuff("Rain");
  } else if (command == "weather_clear") {
    weather.updateWeatherMock("Clear");
    game.applyWeatherBuff("Clear");
  } else if (command == "weather_clouds") {
    weather.updateWeatherMock("Clouds");
    game.applyWeatherBuff("Clouds");
  } else if (command == "weather_hot") {
    weather.updateWeatherMock("Hot");
    game.applyWeatherBuff("Hot");
  } else if (command == "weather_thunder") {
    weather.updateWeatherMock("Thunderstorm");
    game.applyWeatherBuff("Thunderstorm");
  }

  publishStatus(true);
}

void publishStatus(bool force) {
  const unsigned long now = millis();
  if (!force && !game.hasStatusChanged() && now - lastBroadcastMs < STATUS_BROADCAST_INTERVAL_MS) {
    return;
  }

  GameStatus status = game.getStatus();
  oled.update(status);
  web.broadcastStatus(status);
  effects.setLedStatus(status.gameState == "WARNING" || status.inBattle);

  if (status.gameState == "WARNING") {
    effects.playWarningSound();
  } else if (status.gameState == "RESULT" && !status.monsterAlive) {
    effects.playWinSound();
  }

  lastBroadcastMs = now;
  game.clearStatusChanged();
}

void setup() {
  Serial.begin(115200);

  effects.begin();
  hardware.begin();
  weather.begin();
  oled.begin();
  oled.showBootScreen();
  game.initGame();

  web.setCommandCallback(handleWebCommand);
  web.begin();

  publishStatus(true);
}

void loop() {
  web.loop();
  game.tick();

  if (hardware.consumeAttackEvent()) {
    game.playerAttack("button");
    effects.playAttackSound();
    publishStatus(true);
  }

  if (weather.shouldUpdate()) {
    if (weather.updateLocalSensor()) {
      game.setEnvironment(weather.getTemperature(), weather.getHumidity(), weather.getWeatherType());
    }
  }

  publishStatus(false);

  static unsigned long lastHeapPrint = 0;
  if (millis() - lastHeapPrint > 5000) {
    lastHeapPrint = millis();
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Min free heap: ");
    Serial.println(ESP.getMinFreeHeap());
    Serial.print("Max alloc heap: ");
    Serial.println(ESP.getMaxAllocHeap());
  }

  delay(10);
}
