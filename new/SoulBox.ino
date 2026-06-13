#include "config.h"
#include "GameEngine.h"
#include "HardwareInput.h"
#include "WeatherManager.h"
#include "OledDisplay.h"
#include "WebManager.h"
#include "Effects.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <time.h>

GameEngine game;
HardwareInput hardware;
WeatherManager weather;
OledDisplay oled;
WebManager web;
Effects effects;

unsigned long lastBroadcastMs = 0;
int lastNtpYear = -1;
int lastNtpYDay = -1;
unsigned long lastNtpCheckMs = 0;
bool weatherSlotsDone[4] = {false, false, false, false};
bool bossSlotsDone[3] = {false, false, false};

void publishStatus(bool force);
void handleWebCommand(const String &command);
void syncExtendedWeatherToGame();
void applyManualScenario(const String &weatherType, float temperature, float humidity, const String &airQuality);
void setupNetworkServices();
void setupTimeSync();
void checkNtpDayRollover();
void checkScheduledWeatherUpdates();
void checkAirQualityUpdate();
void checkMealBossSchedule();
void resetDailyScheduleFlags();
void handleWeatherAlertButton();

void syncExtendedWeatherToGame() {
  game.setExtendedWeather(
    weather.getExtTemperature(),
    weather.getWindSpeed(),
    weather.getWindDir(),
    weather.getPressure(),
    weather.getVisibility(),
    weather.getDewPoint()
  );
}

void applyManualScenario(const String &weatherType, float temperature, float humidity, const String &airQuality) {
  game.setManualScenario(temperature, humidity, weatherType, airQuality);
}

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
  } else if (command == "show_external_environment") {
    game.showExternalEnvironmentDetail();
  } else if (command == "weather_api_update") {
    if (weather.updateFromAPI()) {
      game.setEnvironment(weather.getTemperature(), weather.getHumidity(), weather.getWeatherType());
      syncExtendedWeatherToGame();
      if (weather.updateAirQualityFromAPI()) {
        game.setAirQuality(weather.getAirQuality());
      }
      game.setSystemEvent("Weather API tick updated real weather data.");
    } else {
      game.setSystemEvent("Weather API tick failed. Check Wi-Fi or OpenWeather settings.");
    }
  } else if (command == "sync_real_weather") {
    if (weather.updateFromAPI()) {
      game.setEnvironment(weather.getTemperature(), weather.getHumidity(), weather.getWeatherType());
      syncExtendedWeatherToGame();
      if (weather.updateAirQualityFromAPI()) {
        game.setAirQuality(weather.getAirQuality());
      }
      game.setSystemEvent("Real weather synced successfully.");
    } else {
      game.setSystemEvent("Real weather sync failed. Check Wi-Fi or OpenWeather settings.");
    }
  } else if (command == "get_status") {
    // Only publish the current status.
  } else if (command == "replace_skill_1") {
    game.resolvePendingSkill(0);
  } else if (command == "replace_skill_2") {
    game.resolvePendingSkill(1);
  } else if (command == "replace_skill_3") {
    game.resolvePendingSkill(2);
  } else if (command == "discard_new_skill") {
    game.discardPendingSkill();
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
  } else if (command == "manual_weather_Rain") {
    weather.updateWeatherMock("Rain");
    applyManualScenario("Rain", weather.getTemperature(), weather.getHumidity(), "Humid");
  } else if (command == "manual_weather_Clear") {
    weather.updateWeatherMock("Clear");
    applyManualScenario("Clear", weather.getTemperature(), weather.getHumidity(), "Good");
  } else if (command == "manual_weather_Clouds") {
    weather.updateWeatherMock("Clouds");
    applyManualScenario("Clouds", weather.getTemperature(), weather.getHumidity(), "Good");
  } else if (command == "manual_weather_Hot") {
    weather.updateWeatherMock("Hot");
    applyManualScenario("Hot", max(weather.getTemperature(), 34.0f), weather.getHumidity(), "Dry");
  } else if (command == "manual_weather_Thunderstorm") {
    weather.updateWeatherMock("Thunderstorm");
    applyManualScenario("Thunderstorm", weather.getTemperature(), max(weather.getHumidity(), 80.0f), "Electric");
  } else if (command == "manual_temp_cold") {
    applyManualScenario(weather.getWeatherType(), 12.0f, weather.getHumidity(), "Good");
  } else if (command == "manual_temp_comfort") {
    applyManualScenario(weather.getWeatherType(), 24.0f, weather.getHumidity(), "Good");
  } else if (command == "manual_temp_hot") {
    applyManualScenario("Hot", 34.0f, weather.getHumidity(), "Dry");
  } else if (command == "manual_humidity_dry") {
    applyManualScenario(weather.getWeatherType(), weather.getTemperature(), 35.0f, "Dry");
  } else if (command == "manual_humidity_comfort") {
    applyManualScenario(weather.getWeatherType(), weather.getTemperature(), 60.0f, "Good");
  } else if (command == "manual_humidity_humid") {
    applyManualScenario("Rain", weather.getTemperature(), 85.0f, "Humid");
  } else if (command == "manual_air_good") {
    game.setAirQuality("Good");
  } else if (command == "manual_air_poor") {
    game.setAirQuality("Poor");
  } else if (command == "manual_air_dry") {
    game.setAirQuality("Dry");
  } else if (command == "manual_air_humid") {
    game.setAirQuality("Humid");
  } else if (command == "manual_air_electric") {
    game.setAirQuality("Electric");
  }

  publishStatus(true);
}

void setupNetworkServices() {
  if (MDNS.begin(DEVICE_HOSTNAME)) {
    MDNS.addService("http", "tcp", WEB_PORT);
    Serial.print("mDNS ready: http://");
    Serial.print(DEVICE_HOSTNAME);
    Serial.println(".local");
  } else {
    Serial.println("mDNS setup failed.");
  }

  ArduinoOTA.setHostname(DEVICE_HOSTNAME);
  ArduinoOTA
    .onStart([]() {
      Serial.println("OTA update started.");
    })
    .onEnd([]() {
      Serial.println("OTA update finished.");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("OTA progress: %u%%\n", (progress * 100) / total);
    })
    .onError([](ota_error_t error) {
      Serial.printf("OTA error[%u]\n", error);
    });
  ArduinoOTA.begin();
  Serial.print("OTA ready: ");
  Serial.println(DEVICE_HOSTNAME);
}

void setupTimeSync() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2);
  Serial.println("NTP time sync requested.");
}

void checkNtpDayRollover() {
  const unsigned long now = millis();
  if (now - lastNtpCheckMs < NTP_CHECK_INTERVAL_MS) {
    return;
  }
  lastNtpCheckMs = now;

  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, 100)) {
    Serial.println("NTP time not ready yet.");
    return;
  }

  if (lastNtpYear < 0) {
    lastNtpYear = timeInfo.tm_year;
    lastNtpYDay = timeInfo.tm_yday;
    Serial.printf("NTP day baseline set: year=%d yday=%d\n", lastNtpYear, lastNtpYDay);
    return;
  }

  if (timeInfo.tm_year != lastNtpYear || timeInfo.tm_yday != lastNtpYDay) {
    lastNtpYear = timeInfo.tm_year;
    lastNtpYDay = timeInfo.tm_yday;
    resetDailyScheduleFlags();
    game.advanceDayFromNtp();
    publishStatus(true);
  }
}

void resetDailyScheduleFlags() {
  for (int i = 0; i < 4; i++) {
    weatherSlotsDone[i] = false;
  }
  for (int i = 0; i < 3; i++) {
    bossSlotsDone[i] = false;
  }
}

void checkScheduledWeatherUpdates() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, 50)) {
    return;
  }

  const int weatherHours[4] = {5, 12, 17, 21};
  for (int i = 0; i < 4; i++) {
    if (timeInfo.tm_hour == weatherHours[i] && !weatherSlotsDone[i]) {
      weatherSlotsDone[i] = true;
      if (weather.updateFromAPI()) {
        game.setEnvironment(weather.getTemperature(), weather.getHumidity(), weather.getWeatherType());
        syncExtendedWeatherToGame();
        game.setSystemEvent("Scheduled OpenWeather update completed.");
        publishStatus(true);
      }
    }
  }
}

void checkAirQualityUpdate() {
  if (!weather.shouldUpdateAirQuality()) {
    return;
  }

  if (weather.updateAirQualityFromAPI()) {
    game.setAirQuality(weather.getAirQuality());
    publishStatus(true);
  }
}

void checkMealBossSchedule() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, 50)) {
    return;
  }

  const int bossHours[3] = {7, 12, 18};
  for (int i = 0; i < 3; i++) {
    if (timeInfo.tm_hour == bossHours[i] && !bossSlotsDone[i]) {
      bossSlotsDone[i] = true;
      game.callScheduledBoss();
      publishStatus(true);
    }
  }
}

void handleWeatherAlertButton() {
  if (!hardware.consumeWeatherAlertEvent()) {
    return;
  }

  if (weather.updateLocalSensor()) {
    game.setEnvironment(weather.getTemperature(), weather.getHumidity(), weather.getWeatherType());
  }

  game.checkEnvironment();
  game.showExternalEnvironmentDetail();
  game.setSystemEvent(
    String("Weather button: ") + weather.getWeatherType() +
    ", " + String(weather.getTemperature(), 1) + "C, " +
    String(weather.getHumidity(), 0) + "% humidity."
  );
  oled.showWeatherAlert(game.getStatus());
  effects.playWarningSound();
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
  setupNetworkServices();
  setupTimeSync();

  publishStatus(true);
  oled.showNetworkInfo("http://soul-box.local", true);
}

void loop() {
  web.loop();
  ArduinoOTA.handle();
  game.tick();

  handleWeatherAlertButton();
  checkNtpDayRollover();
  checkScheduledWeatherUpdates();
  checkAirQualityUpdate();
  checkMealBossSchedule();

  if (weather.shouldUpdateLocal()) {
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

  static unsigned long lastMemoryGuardMs = 0;
  if (millis() - lastMemoryGuardMs > 60000) {
    lastMemoryGuardMs = millis();
    game.optimizeMemory();
  }

  delay(10);
}
