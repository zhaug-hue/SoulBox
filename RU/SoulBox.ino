#include "config.h"
#include "GameEngine.h"
#include "WebManager.h"
#include "OledDisplay.h"
#include "HardwareInput.h"
#include "WeatherManager.h"
#include "Effects.h"

GameEngine     gameEngine;
WebManager     webManager;
OledDisplay    oledDisplay;
HardwareInput  hardwareInput;
WeatherManager weatherManager;
Effects        effects;

String lastGameState = "BOOT";

void handleWebCommand(const String &command);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== SoulBox RPG IoT 系統啟動中 ===");

  effects.begin();
  hardwareInput.begin();
  weatherManager.begin();

  oledDisplay.begin();
  oledDisplay.showBootScreen();
  delay(1000); 

  if (!webManager.begin()) {
    Serial.println("致命錯誤: WebManager 初始化失敗。");
    while (true) { delay(1000); } 
  }

  gameEngine.initGame();
  webManager.setCommandCallback(handleWebCommand);
  Serial.println("=== SoulBox 系統初始化完成 ===");
}

void loop() {
  webManager.loop();
  gameEngine.tick();

  if (hardwareInput.consumeAttackEvent()) {
    effects.playAttackSound();         
    gameEngine.playerAttack("button"); 
  }

  if (weatherManager.shouldUpdateLocal()) {
    if (weatherManager.updateLocalSensor()) {
      gameEngine.setEnvironment(
        weatherManager.getTemperature(), 
        weatherManager.getHumidity(), 
        weatherManager.getWeatherType()
      );
    }
  }

  if (weatherManager.shouldUpdateAPI()) {
    if (weatherManager.updateFromAPI()) {
      gameEngine.setEnvironment(
        weatherManager.getTemperature(), 
        weatherManager.getHumidity(), 
        weatherManager.getWeatherType()
      );
      // 將擴充氣象寫入引擎
      gameEngine.setExtendedWeather(
        weatherManager.getWindSpeed(),
        weatherManager.getWindDir(),
        weatherManager.getPressure(),
        weatherManager.getVisibility(),
        weatherManager.getDewPoint()
      );
    }
  }

  GameStatus currentStatus = gameEngine.getStatus();
  
  if (gameEngine.hasStatusChanged()) {
    webManager.broadcastStatus(currentStatus);
    gameEngine.clearStatusChanged();

    if (currentStatus.gameState != lastGameState) {
      if (currentStatus.gameState == "RESULT" && !currentStatus.monsterAlive) {
        effects.playWinSound(); 
      } else if (currentStatus.gameState == "WARNING") {
        effects.playWarningSound(); 
      }
      lastGameState = currentStatus.gameState;
    }
  }

  static unsigned long lastBroadcastMs = 0;
  if (millis() - lastBroadcastMs >= STATUS_BROADCAST_INTERVAL_MS) {
    lastBroadcastMs = millis();
    webManager.broadcastStatus(gameEngine.getStatus());
  }

  oledDisplay.update(currentStatus);

  static unsigned long lastHeapReportMs = 0;
  if (millis() - lastHeapReportMs >= 5000) {
    lastHeapReportMs = millis();
    Serial.printf("[System] Free Heap: %u bytes\n", ESP.getFreeHeap());
  }
}

void handleWebCommand(const String &command) {
  Serial.println("[WebSocket] 收到: " + command);

  if (command == "web_attack") { gameEngine.normalAttack(); } 
  else if (command == "skill_1") { gameEngine.useSkill(0); } 
  else if (command == "skill_2") { gameEngine.useSkill(1); } 
  else if (command == "skill_3") { gameEngine.useSkill(2); } 
  else if (command == "call_boss") { gameEngine.callBoss(); } 
  else if (command == "feed") { gameEngine.feedPet(); } 
  else if (command == "new_day") { gameEngine.initDailyStatus(); } 
  else if (command == "check_environment") { gameEngine.checkEnvironment(); } 
  else if (command == "weather_api_update") { gameEngine.simulateWeatherApiUpdate(); } 
  else if (command == "sync_real_weather") {
    Serial.println("執行手動即時同步真實世界氣象...");
    if (weatherManager.updateFromAPI()) {
      gameEngine.setEnvironment(
        weatherManager.getTemperature(), 
        weatherManager.getHumidity(), 
        weatherManager.getWeatherType()
      );
      // 將擴充氣象寫入引擎
      gameEngine.setExtendedWeather(
        weatherManager.getWindSpeed(),
        weatherManager.getWindDir(),
        weatherManager.getPressure(),
        weatherManager.getVisibility(),
        weatherManager.getDewPoint()
      );
    }
  }
  else if (command == "clear_modals") { gameEngine.clearModalFlags(); } 
  else if (command == "weather_rain") {
    weatherManager.updateWeatherMock("Rain");
    gameEngine.setEnvironment(weatherManager.getTemperature(), weatherManager.getHumidity(), "Rain");
  } 
  else if (command == "weather_clear") {
    weatherManager.updateWeatherMock("Clear");
    gameEngine.setEnvironment(weatherManager.getTemperature(), weatherManager.getHumidity(), "Clear");
  } 
  else if (command == "weather_clouds") {
    weatherManager.updateWeatherMock("Clouds");
    gameEngine.setEnvironment(weatherManager.getTemperature(), weatherManager.getHumidity(), "Clouds");
  } 
  else if (command == "weather_hot") {
    weatherManager.updateWeatherMock("Hot");
    gameEngine.setEnvironment(weatherManager.getTemperature(), weatherManager.getHumidity(), "Hot");
  } 
  else if (command == "weather_thunder") {
    weatherManager.updateWeatherMock("Thunderstorm");
    gameEngine.setEnvironment(weatherManager.getTemperature(), weatherManager.getHumidity(), "Thunderstorm");
  }
}
