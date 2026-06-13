#include "config.h"
#include "GameEngine.h"
#include "WebManager.h"
#include "OledDisplay.h"
#include "HardwareInput.h"
#include "WeatherManager.h"
#include "Effects.h"

// 宣告所有核心功能模組物件
GameEngine     gameEngine;
WebManager     webManager;
OledDisplay    oledDisplay;
HardwareInput  hardwareInput;
WeatherManager weatherManager;
Effects        effects;

// 紀錄上一次的遊戲狀態，用來偵測狀態切換並觸發對應的實體音效
String lastGameState = "BOOT";

// 宣告 WebSocket 指令回調函式
void handleWebCommand(const String &command);

void setup() {
  // 1. 初始化序列埠監控
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== SoulBox RPG IoT 系統啟動中 ===");

  // 2. 初始化實體週邊（LED、蜂鳴器、中斷按鈕、DHT11 感測器）
  effects.begin();
  hardwareInput.begin();
  weatherManager.begin();

  // 3. 啟動 OLED 螢幕並顯示開機畫面
  oledDisplay.begin();
  oledDisplay.showBootScreen();
  delay(1000); // 讓開機畫面短暫停留展示

  // 4. 啟動網路與通訊服務 (連線 Wi-Fi、掛載 SPIFFS、建立 WebSocket 伺服器)
  if (!webManager.begin()) {
    Serial.println("致命錯誤: WebManager 初始化失敗，請檢查 SPIFFS 是否已上傳。");
    while (true) { delay(1000); } // 發生致命錯誤時停機防呆
  }

  // 5. 初始化 RPG 核心遊戲引擎數據
  gameEngine.initGame();

  // 6. 綁定網頁端 WebSocket 指令監聽器
  webManager.setCommandCallback(handleWebCommand);

  Serial.println("=== SoulBox 系統初始化完成，開始運作 ===");
}

void loop() {
  // 1. 處理 WebSocket 底層客戶端清理與內部維護
  webManager.loop();

  // 2. 驅動遊戲引擎內部的時間輪詢計時器（例如處理 RESULT 畫面超時自動切回 IDLE）
  gameEngine.tick();

  // 3. 檢查實體中斷按鈕事件 (GPIO 32)
  if (hardwareInput.consumeAttackEvent()) {
    effects.playAttackSound();         // 播放實體砍擊音效回饋
    gameEngine.playerAttack("button"); // 觸發實體按鈕攻擊邏輯 (基礎傷害 20)
  }

  // 4. 定期讀取本地實體 DHT11 感測器 (環境溫濕度)
  if (weatherManager.shouldUpdateLocal()) {
    if (weatherManager.updateLocalSensor()) {
      gameEngine.setEnvironment(
        weatherManager.getTemperature(), 
        weatherManager.getHumidity(), 
        weatherManager.getWeatherType()
      );
    }
  }

  // 5. 定期向 OpenWeather API 請求虎尾科大的真實世界天氣 (預設每 10 分鐘)
  if (weatherManager.shouldUpdateAPI()) {
    if (weatherManager.updateFromAPI()) {
      gameEngine.setEnvironment(
        weatherManager.getTemperature(), 
        weatherManager.getHumidity(), 
        weatherManager.getWeatherType()
      );
    }
  }

  // 6. 核心即時同步機制：若遊戲數據或狀態發生改變，立刻更新實體週邊並廣播至網頁
  GameStatus currentStatus = gameEngine.getStatus();
  
  if (gameEngine.hasStatusChanged()) {
    // A. 將最新遊戲與環境數據廣播至網頁 Dashboard
    webManager.broadcastStatus(currentStatus);
    gameEngine.clearStatusChanged();

    // B. 根據遊戲狀態的轉變，觸發對應的實體蜂鳴器音效
    if (currentStatus.gameState != lastGameState) {
      if (currentStatus.gameState == "RESULT" && !currentStatus.monsterAlive) {
        effects.playWinSound(); // 戰勝怪物/Boss 音效
      } else if (currentStatus.gameState == "WARNING") {
        effects.playWarningSound(); // 飽食度過低飢餓警告音效
      }
      lastGameState = currentStatus.gameState;
    }
  }

  // 7. 定時保底廣播機制（確保網頁端的「更新時間計時」如 5s ago、12s ago 能維持正常跳動）
  static unsigned long lastBroadcastMs = 0;
  if (millis() - lastBroadcastMs >= STATUS_BROADCAST_INTERVAL_MS) {
    lastBroadcastMs = millis();
    webManager.broadcastStatus(gameEngine.getStatus());
  }

  // 8. 即時刷新實體 OLED 顯示螢幕
  oledDisplay.update(currentStatus);

  // 9. 系統效能與記憶體監控：每 5 秒在 Serial 印出 Free Heap，防範長時間展示發生記憶體洩漏
  static unsigned long lastHeapReportMs = 0;
  if (millis() - lastHeapReportMs >= 5000) {
    lastHeapReportMs = millis();
    Serial.printf("[System] Free Heap: %u bytes\n", ESP.getFreeHeap());
  }
}

// 核心控制中樞：處理從網頁 Dashboard 經由 WebSocket 傳輸過來的控制與測試指令
void handleWebCommand(const String &command) {
  Serial.println("[WebSocket 遠端指令] 收到: " + command);

  // === 戰鬥與遊戲核心操作指令 ===
  if (command == "web_attack") {
    gameEngine.normalAttack(); // 網頁普通攻擊 (基礎傷害 15)
  } 
  else if (command == "skill_1") {
    gameEngine.useSkill(0); // 使用技能欄位 1
  } 
  else if (command == "skill_2") {
    gameEngine.useSkill(1); // 使用技能欄位 2
  } 
  else if (command == "skill_3") {
    gameEngine.useSkill(2); // 使用技能欄位 3
  } 
  else if (command == "call_boss") {
    gameEngine.callBoss(); // 召喚雙倍血量（180 HP）的 Boss 進場
  } 
  else if (command == "feed") {
    gameEngine.feedPet(); // 執行休息與餵食，恢復飽食度與玩家 HP
  } 
  
  // === 系統、彈窗與真實環境交互指令 ===
  else if (command == "new_day") {
    gameEngine.initDailyStatus(); // 重置並初始化新的一天遊戲數據
  } 
  else if (command == "check_environment") {
    gameEngine.checkEnvironment(); // 檢查環境並觸發網頁端跳出環境建議彈窗
  } 
  else if (command == "weather_api_update") {
    gameEngine.simulateWeatherApiUpdate(); // 手動觸發天氣計時更新（原本的模擬 Tick）
  } 
  else if (command == "sync_real_weather") {
    // 💡 處理網頁端點擊「Sync Real Weather」時的即時網路抓取請求
    Serial.println("執行手動即時同步真實世界氣象...");
    if (weatherManager.updateFromAPI()) {
      gameEngine.setEnvironment(
        weatherManager.getTemperature(), 
        weatherManager.getHumidity(), 
        weatherManager.getWeatherType()
      );
    }
  }
  else if (command == "clear_modals") {
    gameEngine.clearModalFlags(); // 清除網頁端的彈窗旗標，關閉 Overlay UI
  } 
  
  // === 離線展示或除錯專用的天氣模擬強制切換指令 ===
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
