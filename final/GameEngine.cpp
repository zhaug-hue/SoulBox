#include "GameEngine.h"
#include "config.h"

void GameEngine::initGame() {
  initNewPlayer();
}

void GameEngine::initNewPlayer() {
  _status.pendingSkill = "";
  _status.carriedBossSkill = "";
  _status.hasPendingSkillChoice = false;
  _status.playerHp = PLAYER_MAX_HP;
  _status.playerMaxHp = PLAYER_MAX_HP;
  _status.monsterName = "普通怪物";
  _status.monsterHp = 45;
  _status.monsterMaxHp = 45;
  _status.satiety = 10;
  _status.dayCount = 1;
  _status.expressionCounter = 0;
  _status.role = "靈魂守護者";
  _status.mood = "好奇";
  _status.moodFace = getCurrentExpression();
  _status.webExpression = getCurrentExpression();
  _status.oledExpression = getCurrentOledExpression();
  _status.weatherType = "多雲";
  _status.buffName = "普通攻擊 x1.0";
  _status.satietyBuffName = "飽足靈魂：傷害 +25%";
  _status.airQuality = "良好";
  _status.environmentAdvice = "雲層穩定。觀察怪物並準備技能。";
  _status.story = "第 1 天：SoulBox 甦醒了。天氣將形塑它的第一種魔法。";
  _status.skills[0] = "基礎斬擊";
  _status.skills[1] = "";
  _status.skills[2] = "";
  _status.skillCount = 1;
  _status.weatherMultiplier = 1.0;
  _status.lastWeatherApiUpdate = 0;
  _status.lastAirQualityUpdate = 0;
  _status.showTutorial = true;
  _status.showWeatherBuff = false;
  _status.showEnvironmentAdvice = false;
  _status.showFirstStory = true;
  _status.showSatietyWarning = false;
  _status.weatherBuffModalPending = false;
  _status.temperature = 0;
  _status.humidity = 0;
  _status.extTemperature = 0;
  _status.extWindSpeed = 0;
  _status.extWindDir = "-";
  _status.extPressure = 0;
  _status.extVisibility = 0;
  _status.extDewPoint = 0;
  _status.healthAdvice = "等待真實氣象數據中。";
  _status.inBattle = false;
  _status.monsterAlive = true;
  _status.bossBattle = false;
  _firstDayMonsterDefeated = false;
  _tutorialMonsterActive = true;
  setState(IDLE);
  setEvent("SoulBox 在第 1 天甦醒。");
  startFirstDayTutorial();
}

void GameEngine::initDailyStatus() {
  _status.dayCount += 1;
  _status.satiety = SATIETY_MAX;
  _status.playerHp = PLAYER_MAX_HP;
  _status.monsterHp = 0;
  _status.monsterMaxHp = MONSTER_NORMAL_HP;
  _status.monsterName = "普通怪物";
  _status.monsterHp = MONSTER_NORMAL_HP;
  _status.monsterAlive = true;
  _status.inBattle = false;
  _status.bossBattle = false;
  _status.expressionCounter = 0;
  _status.mood = "好奇";
  _status.moodFace = getCurrentExpression();
  _status.webExpression = getCurrentExpression();
  _status.oledExpression = getCurrentOledExpression();
  _status.showTutorial = true;
  _status.showFirstStory = true;
  _status.showEnvironmentAdvice = true;
  _firstDayMonsterDefeated = true;
  _tutorialMonsterActive = false;
  _status.story = "新的一天開始了。SoulBox 準備好向天氣學習。";
  checkSatietyBuffDebuff();
  setState(IDLE);
  setEvent("每日狀態已初始化。");
}

void GameEngine::advanceDayFromNtp() {
  _status.dayCount += 1;
  reduceSatiety(DAILY_SATIETY_DECAY);
  _status.monsterName = "普通怪物";
  _status.monsterMaxHp = MONSTER_NORMAL_HP;
  _status.monsterHp = MONSTER_NORMAL_HP;
  _status.monsterAlive = true;
  _status.inBattle = false;
  _status.bossBattle = false;
  _status.expressionCounter = 0;
  _status.showEnvironmentAdvice = true;
  _firstDayMonsterDefeated = true;
  _tutorialMonsterActive = false;
  _status.story = "真實的一天過去了。SoulBox 在夜間消耗了飽食度，新的怪物出現了。";
  if (_status.satiety <= 3) {
    setState(WARNING);
  } else {
    setState(IDLE);
  }
  setEvent("NTP 每日重置：飽食度下降且怪物已重置。");
}

void GameEngine::startFirstDayTutorial() {
  setEvent("教學：使用三次普通攻擊擊敗第一隻怪物，學習今日的天氣技能。");
}

void GameEngine::startBattle(bool isBoss, const String &bossName) {
  _status.bossBattle = isBoss;
  if (isBoss) {
    _status.monsterName = bossName != "" ? bossName : "魔王";
    _status.monsterMaxHp = MONSTER_NORMAL_HP * 2;
    reduceSatiety(2);
  } else if (_status.dayCount == 1 && !_firstDayMonsterDefeated) {
    _status.monsterName = "普通怪物";
    _status.monsterMaxHp = 45;
    _tutorialMonsterActive = true;
  } else {
    _status.monsterName = "普通怪物";
    _status.monsterMaxHp = MONSTER_NORMAL_HP;
    reduceSatiety(1);
    _tutorialMonsterActive = false;
  }
  _status.monsterHp = _status.monsterMaxHp;
  _status.monsterAlive = true;
  _status.inBattle = true;
  setState(BATTLE);
  setEvent(isBoss ? (_status.monsterName + " 出現了。使用你收集的技能！") : "怪物出現了。");
}

void GameEngine::startNormalMonster() {
  startBattle(false);
}

void GameEngine::callBoss() {
  startBattle(true);
}

void GameEngine::callBossWithType(int type) {
  String name = "魔王";
  if (type == 0) name = "早餐魔王";
  else if (type == 1) name = "午餐魔王";
  else if (type == 2) name = "晚餐魔王";
  startBattle(true, name);
}

void GameEngine::callScheduledBoss() {
  struct tm timeInfo;
  int type = 1; // Default to Lunch
  if (getLocalTime(&timeInfo, 10)) {
    if (timeInfo.tm_hour < 10) type = 0;
    else if (timeInfo.tm_hour >= 15) type = 2;
  }

  if (_status.inBattle && _status.monsterAlive && !_status.bossBattle) {
    _status.inBattle = false;
    _status.monsterAlive = false;
    reduceSatiety(1);
    setEvent("未完成的怪物在用餐時間魔王出現前逃走了。飽食度下降。");
  } else if (_status.inBattle && _status.monsterAlive && _status.bossBattle) {
    handleMissedBoss();
  }
  
  callBossWithType(type);
  setEvent("排定的用餐時間 " + _status.monsterName + " 出現了。");
}

void GameEngine::handleMissedBoss() {
  if (_status.skillCount > 1) {
    const int index = _status.skillCount - 1;
    _status.carriedBossSkill = _status.skills[index];
    _status.skills[index] = "";
    _status.skillCount -= 1;
    setEvent("錯過了魔王。一個技能被封印，直到下一個魔王獎勵。");
  } else {
    reduceSatiety(1);
    setEvent("錯過了魔王。基礎攻擊仍安全，但飽食度下降。");
  }
}

void GameEngine::playerAttack(const String &source) {
  if (!_status.inBattle || !_status.monsterAlive) {
    if (_status.bossBattle && !_status.monsterAlive) {
      setEvent("魔王已被擊敗。請再次召喚魔王或開始新的一天。");
      return;
    }
    startBattle(false);
  }

  const int damage = calculateDamage(source == "button" ? 20 : 15);
  _status.monsterHp = clampInt(_status.monsterHp - damage, 0, _status.monsterMaxHp);
  reduceSatiety(1);
  updateExpressionPositive();

  setEvent(source + " 攻擊造成了 " + String(damage) + " 點傷害。");
  checkBattleResult();

  if (_status.inBattle && _status.monsterAlive) {
    monsterAttack();
  }
}

void GameEngine::normalAttack() {
  playerAttack("普通");
}

void GameEngine::useSkill(int skillIndex) {
  if (skillIndex < 0 || skillIndex >= _status.skillCount) {
    setEvent("該技能欄位是空的。");
    return;
  }

  if (!_status.inBattle || !_status.monsterAlive) {
    if (_status.bossBattle && !_status.monsterAlive) {
      setEvent("魔王已被擊敗。請再次召喚魔王或開始新的一天。");
      return;
    }
    startBattle(false);
  }

  const int baseDamage = 24 + skillIndex * 8;
  const int damage = calculateDamage(baseDamage);
  _status.monsterHp = clampInt(_status.monsterHp - damage, 0, _status.monsterMaxHp);
  reduceSatiety(_status.bossBattle ? 2 : 1);
  updateExpressionPositive();
  setEvent(_status.skills[skillIndex] + " 造成了 " + String(damage) + " 點傷害。");
  checkBattleResult();

  if (_status.inBattle && _status.monsterAlive) {
    monsterAttack();
  }
}

void GameEngine::webSkillAttack() {
  useSkill(0);
}

void GameEngine::monsterAttack() {
  if (!_status.inBattle || !_status.monsterAlive) {
    return;
  }

  updateExpressionNegative();
  setEvent(_status.monsterName + " 反擊了。SoulBox 的表情發生了變化。");
}

void GameEngine::feedPet() {
  addSatiety(2);
  _status.playerHp = clampInt(_status.playerHp + 8, 0, _status.playerMaxHp);
  checkMood();
  updateExpressionPositive();
  setEvent("SoulBox 休息了。飽食度和血量已恢復。");
}

void GameEngine::addSatiety(int amount) {
  updateSatiety(amount);
}

void GameEngine::reduceSatiety(int amount) {
  updateSatiety(-amount);
}

void GameEngine::applyWeatherBuff(const String &weatherType) {
  _status.weatherType = weatherType;

  if (weatherType == "Rain" || weatherType == "下雨") {
    _status.weatherType = "下雨";
    _weatherMultiplier = 1.5;
    _status.buffName = "水魔法 x1.5";
    _status.airQuality = "潮濕";
    _status.environmentAdvice = "雨水增強了水魔法。在挑戰魔王前使用下雨技能。";
    addSkill("雨之波紋");
    _status.showWeatherBuff = true;
    _status.weatherBuffModalPending = true;
  } else if (weatherType == "Clear" || weatherType == "晴天") {
    _status.weatherType = "晴天";
    _weatherMultiplier = 1.2;
    _status.buffName = "光之劍 x1.2";
    _status.airQuality = "明亮";
    _status.environmentAdvice = "晴朗的天氣提供穩定的光屬性傷害。";
    addSkill("光之劍");
    _status.showWeatherBuff = true;
    _status.weatherBuffModalPending = true;
  } else if (weatherType == "Thunderstorm" || weatherType == "雷陣雨") {
    _status.weatherType = "雷陣雨";
    _weatherMultiplier = 1.7;
    _status.buffName = "雷電火花 x1.7";
    _status.airQuality = "帶電";
    _status.environmentAdvice = "雷陣雨雖然危險但非常強大。";
    addSkill("雷電火花");
    _status.showWeatherBuff = true;
    _status.weatherBuffModalPending = true;
  } else if (weatherType == "Hot" || weatherType == "炎熱") {
    _status.weatherType = "炎熱";
    _weatherMultiplier = 1.3;
    _status.buffName = "火焰之刃 x1.3";
    _status.airQuality = "乾燥";
    _status.environmentAdvice = "熱空氣增強了火焰傷害，但也消耗能量。";
    addSkill("火焰之刃");
    _status.showWeatherBuff = true;
    _status.weatherBuffModalPending = true;
  } else {
    _status.weatherType = "多雲";
    _weatherMultiplier = 1.0;
    _status.buffName = "普通攻擊 x1.0";
    _status.airQuality = "良好";
    _status.environmentAdvice = "雲層很均衡。為魔王戰保留飽食度。";
    addSkill("雲之守護");
    _status.showWeatherBuff = true;
    _status.weatherBuffModalPending = true;
  }

  _status.weatherMultiplier = _weatherMultiplier;
  _status.lastWeatherApiUpdate = millis();
  grantFirstDayWeatherSkill();
  _statusChanged = true;
}

void GameEngine::setEnvironment(float temperature, float humidity, const String &weatherType) {
  _status.temperature = temperature;
  _status.humidity = humidity;
  applyWeatherBuff(weatherType);
  updateHealthAdvice();
}

void GameEngine::setExtendedWeather(float temperature, float windSpeed, const String &windDir, int pressure, float visibility, float dewPoint) {
  _status.extTemperature = temperature;
  _status.extWindSpeed = windSpeed;
  _status.extWindDir = windDir;
  _status.extPressure = pressure;
  _status.extVisibility = visibility;
  _status.extDewPoint = dewPoint;
  updateHealthAdvice();
  _statusChanged = true;
}

void GameEngine::setManualScenario(float temperature, float humidity, const String &weatherType, const String &airQuality) {
  _status.temperature = temperature;
  _status.humidity = humidity;
  applyWeatherBuff(weatherType);
  _status.airQuality = airQuality;
  updateHealthAdvice();
  _status.environmentAdvice = _status.healthAdvice;
  _status.showEnvironmentAdvice = true;
  setEvent("手動模擬情境已更新。");
}

void GameEngine::setAirQuality(const String &airQuality) {
  _status.airQuality = airQuality;
  updateHealthAdvice();
  _status.environmentAdvice = _status.healthAdvice;
  _status.showEnvironmentAdvice = true;
  setEvent("空氣品質情境已更新：" + airQuality);
}

void GameEngine::showExternalEnvironmentDetail() {
  updateHealthAdvice();
  _status.environmentAdvice = _status.healthAdvice;
  _status.showEnvironmentAdvice = true;
  setEvent("要求外在環境詳細數據。");
}

void GameEngine::checkEnvironment() {
  simulateAirQualityUpdate();
  _status.showEnvironmentAdvice = true;
  setEvent("環境檢查完成：" + _status.weatherType + "，" + _status.airQuality + " 空氣。");
}

void GameEngine::simulateWeatherApiUpdate() {
  applyWeatherBuff(_status.weatherType);
  _status.showWeatherBuff = true;
  _status.lastWeatherApiUpdate = millis();
  setEvent("已模擬氣象 API 更新。");
}

void GameEngine::simulateAirQualityUpdate() {
  if (_status.humidity >= 80) {
    _status.airQuality = "潮濕";
  } else if (_status.temperature >= 32) {
    _status.airQuality = "乾燥";
  } else if (_status.weatherType == "雷陣雨") {
    _status.airQuality = "帶電";
  } else {
    _status.airQuality = "良好";
  }
  _status.lastAirQualityUpdate = millis();
  _statusChanged = true;
}

void GameEngine::updateSatiety(int delta) {
  _status.satiety = clampInt(_status.satiety + delta, SATIETY_MIN, SATIETY_MAX);
  checkMood();
  checkSatietyBuffDebuff();
}

void GameEngine::checkMood() {
  if (_status.satiety >= 8) {
    _status.mood = "開心";
  } else if (_status.satiety <= 2) {
    _status.mood = "飢餓";
    if (_state != BATTLE && _state != RESULT) {
      setState(WARNING);
    }
  } else {
    _status.mood = "正常";
    if (_state == WARNING) {
      setState(IDLE);
    }
  }
  _status.moodFace = getCurrentExpression();
  _status.webExpression = getCurrentExpression();
  _status.oledExpression = getCurrentOledExpression();
  _statusChanged = true;
}

void GameEngine::checkSatietyBuffDebuff() {
  if (_status.satiety >= 7) {
    _status.satietyBuffName = "飽足靈魂：傷害 +25%";
    _status.showSatietyWarning = false;
  } else if (_status.satiety <= 3) {
    _status.satietyBuffName = "飢餓減益：傷害 -20%";
    _status.showSatietyWarning = true;
    if (_state != BATTLE && _state != RESULT) {
      setState(WARNING);
    }
  } else {
    _status.satietyBuffName = "飽食度正常";
  }
  _statusChanged = true;
}

void GameEngine::checkBattleResult() {
  if (_status.monsterHp <= 0) {
    _status.monsterAlive = false;
    _status.inBattle = false;
    addSatiety(_status.bossBattle ? 2 : 2);
    _status.mood = "開心";
    updateExpressionPositive();
    setState(RESULT);
    if (_status.bossBattle) {
      grantBossSkill();
      setEvent("魔王已被擊敗。SoulBox 學會了魔王技能。");
    } else {
      if (_status.dayCount == 1 && !_firstDayMonsterDefeated) {
        _firstDayMonsterDefeated = true;
        _tutorialMonsterActive = false;
        grantFirstDayWeatherSkill();
        setEvent("第一隻怪物已被擊敗。SoulBox 學會了今日的天氣技能。");
        return;
      }
      setEvent("怪物已被擊敗。SoulBox 表情變好了。");
    }
  }
}

void GameEngine::tick() {
  if (_state == RESULT && millis() - _stateChangedMs > 6000) {
    setState(IDLE);
  }
}

void GameEngine::updateExpressionPositive() {
  _status.expressionCounter = clampInt(_status.expressionCounter + 1, -5, 5);
  _status.moodFace = getCurrentExpression();
  _status.webExpression = getCurrentExpression();
  _status.oledExpression = getCurrentOledExpression();
  _statusChanged = true;
}

void GameEngine::updateExpressionNegative() {
  _status.expressionCounter = clampInt(_status.expressionCounter - 1, -5, 5);
  _status.moodFace = getCurrentExpression();
  _status.webExpression = getCurrentExpression();
  _status.oledExpression = getCurrentOledExpression();
  _statusChanged = true;
}

String GameEngine::getCurrentExpression() const {
  if (_status.expressionCounter >= 3) {
    return "(●´ω｀●)ゞ";
  }
  if (_status.expressionCounter >= 2) {
    return "( ～'ω')～";
  }
  if (_status.expressionCounter >= 1) {
    return "ヽ(・×・´)ゞ";
  }
  if (_status.expressionCounter <= -5) {
    return "｡ﾟヽ(ﾟ´Д`)ﾉﾟ｡";
  }
  if (_status.expressionCounter <= -3) {
    return "Σ(ﾟДﾟ；≡；ﾟдﾟ)";
  }
  if (_status.expressionCounter <= -1) {
    return "Σ(ﾟдﾟ)";
  }
  return "/ᐠ .ᆺ. ᐟ\\ﾉ";
}

String GameEngine::getCurrentOledExpression() const {
  return getCurrentExpression();
}

void GameEngine::updateEnvironmentAdvice() {
  applyWeatherBuff(_status.weatherType);
}

void GameEngine::clearModalFlags() {
  _status.showTutorial = false;
  _status.showWeatherBuff = false;
  _status.showEnvironmentAdvice = false;
  _status.showFirstStory = false;
  _status.showSatietyWarning = false;
  _status.weatherBuffModalPending = false;
  _statusChanged = true;
}

void GameEngine::grantFirstDayWeatherSkill() {
  if (_status.dayCount == 1 && _status.skillCount < 2) {
    if (_status.weatherType == "下雨") {
      addSkill("雨之波紋");
    } else if (_status.weatherType == "晴天") {
      addSkill("光之劍");
    } else if (_status.weatherType == "炎熱") {
      addSkill("火焰之刃");
    } else {
      addSkill("雲之守護");
    }
  }
}

void GameEngine::grantBossSkill() {
  String newSkill = "魔王剋星";
  if (_status.carriedBossSkill != "" && (millis() % 2 == 0)) {
    newSkill = _status.carriedBossSkill;
  }
  _status.carriedBossSkill = "";

  if (canAddSkill(newSkill)) {
    addSkill(newSkill);
    setEvent("魔王已被擊敗。學會新技能：" + newSkill);
  } else {
    _status.pendingSkill = newSkill;
    _status.hasPendingSkillChoice = true;
    setEvent("魔王已被擊敗。請從 4 個技能中選擇 3 個。");
  }
  _statusChanged = true;
}

void GameEngine::handleBossFailedPenalty() {
  reduceSatiety(1);
  _status.story = "魔王太強大了。SoulBox 需要更好的天氣時機。";
  setEvent("魔王挑戰失敗。飽食度下降，SoulBox 感到疲倦。");
}

bool GameEngine::canAddSkill(const String &skillName) const {
  for (int i = 0; i < _status.skillCount; i++) {
    if (_status.skills[i] == skillName) {
      return false;
    }
  }
  return _status.skillCount < 3;
}

void GameEngine::addSkill(const String &skillName) {
  if (!canAddSkill(skillName)) {
    return;
  }
  _status.skills[_status.skillCount] = skillName;
  _status.skillCount += 1;
  _statusChanged = true;
}

void GameEngine::replaceSkill(int oldSkillIndex, const String &newSkillName) {
  if (oldSkillIndex < 0 || oldSkillIndex >= 3) {
    return;
  }
  _status.skills[oldSkillIndex] = newSkillName;
  if (_status.skillCount <= oldSkillIndex) {
    _status.skillCount = oldSkillIndex + 1;
  }
  _statusChanged = true;
}

GameStatus GameEngine::getStatus() const {
  GameStatus copy = _status;
  copy.gameState = stateToString(_state);
  copy.moodFace = getCurrentExpression();
  copy.webExpression = getCurrentExpression();
  copy.oledExpression = getCurrentOledExpression();
  copy.weatherMultiplier = _weatherMultiplier;
  return copy;
}

bool GameEngine::hasStatusChanged() const {
  return _statusChanged;
}

void GameEngine::clearStatusChanged() {
  _statusChanged = false;
}

void GameEngine::setState(GameState state) {
  if (_state != state) {
    _stateChangedMs = millis();
  }
  _state = state;
  _status.gameState = stateToString(state);
  _statusChanged = true;
}

void GameEngine::setEvent(const String &event) {
  _status.lastEvent = event;
  _statusChanged = true;
}

String GameEngine::stateToString(GameState state) const {
  switch (state) {
    case BOOT: return "啟動";
    case IDLE: return "待機";
    case BATTLE: return "戰鬥中";
    case RESULT: return "結算";
    case WARNING: return "警告";
  }
  return "待機";
}

int GameEngine::calculateDamage(int baseDamage) const {
  float satietyMultiplier = 1.0;
  if (_status.satiety >= 7) {
    satietyMultiplier = 1.25;
  } else if (_status.satiety <= 3) {
    satietyMultiplier = 0.8;
  }
  return max(1, (int)(baseDamage * _weatherMultiplier * satietyMultiplier));
}

int GameEngine::clampInt(int value, int minValue, int maxValue) const {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

void GameEngine::resolvePendingSkill(int replaceIndex) {
  if (!_status.hasPendingSkillChoice) {
    return;
  }

  if (replaceIndex >= 0 && replaceIndex < 3) {
    _status.skills[replaceIndex] = _status.pendingSkill;
    setEvent("技能已更新：" + _status.pendingSkill);
  }

  _status.pendingSkill = "";
  _status.hasPendingSkillChoice = false;
  _statusChanged = true;
}

void GameEngine::discardPendingSkill() {
  if (!_status.hasPendingSkillChoice) {
    return;
  }

  setEvent("捨棄新技能：" + _status.pendingSkill);
  _status.pendingSkill = "";
  _status.hasPendingSkillChoice = false;
  _statusChanged = true;
}

void GameEngine::updateHealthAdvice() {
  String advice = "";

  if (_status.temperature >= 32.0 || _status.extTemperature >= 32.0) {
    advice += "溫度過高。請多喝水，避免長時間在陽光下曝曬。\n";
  } else if ((_status.temperature > 0 && _status.temperature <= 15.0) || (_status.extTemperature > 0 && _status.extTemperature <= 15.0)) {
    advice += "溫度過低。外出前請注意保暖。\n";
  }

  if (_status.humidity >= 80.0) {
    advice += "濕度過高。建議開啟除濕機。\n";
  } else if (_status.humidity > 0 && _status.humidity <= 40.0) {
    advice += "空氣乾燥。請多喝水並注意皮膚保濕。\n";
  }

  if (_status.extPressure > 0 && _status.extPressure < 1000) {
    advice += "氣壓較低。若您對頭痛敏感，請多休息。\n";
  } else if (_status.extPressure > 1025) {
    advice += "氣壓較高。請注意心血管舒適度。\n";
  }

  if (_status.extVisibility > 0 && _status.extVisibility < 5.0) {
    advice += "能見度低。戶外空氣或霧氣可能不佳，外出請小心。\n";
  }

  if (_status.extWindSpeed > 8.0) {
    advice += "風力強勁。注意掉落物與交通安全。\n";
  }

  if (_status.extDewPoint >= 24.0) {
    advice += "露點高。體感可能悶熱，盡量留在室內休息。\n";
  }

  if (_status.airQuality == "差" || _status.airQuality == "Poor") {
    _status.airQuality = "差";
    advice += "空氣品質差。減少戶外活動，並考慮關閉窗戶。\n";
  } else if (_status.airQuality == "普通" || _status.airQuality == "Moderate") {
    _status.airQuality = "普通";
    advice += "空氣品質尚可。敏感族群應留意戶外時間。\n";
  } else if (_status.airQuality == "良好" || _status.airQuality == "Good") {
    _status.airQuality = "良好";
    advice += "空氣品質良好。若戶外條件安全，可以適度開窗通風。\n";
  } else if (_status.airQuality == "潮濕" || _status.airQuality == "Humid") {
    _status.airQuality = "潮濕";
    advice += "感覺空氣潮濕。除濕可能會讓室內更舒適。\n";
  } else if (_status.airQuality == "乾燥" || _status.airQuality == "Dry") {
    _status.airQuality = "乾燥";
    advice += "感覺空氣乾燥。請補充水分並考慮室內加濕。\n";
  } else if (_status.airQuality == "帶電" || _status.airQuality == "Electric") {
    _status.airQuality = "帶電";
    advice += "偵測到雷陣雨前的帶電空氣。請注意戶外安全。\n";
  }

  if (advice == "") {
    advice = "目前環境看起來很舒適。保持穩定節奏。";
  }

  _status.healthAdvice = advice;
  _statusChanged = true;
}

void GameEngine::optimizeMemory() {
  if (ESP.getFreeHeap() >= 35000) {
    return;
  }

  _status.lastEvent = "記憶體防護已優化儀表板文本。";
  _status.story = "SoulBox 清理了暫時的想法，並保留了核心遊戲狀態。";
  _status.environmentAdvice = "長時間的環境文字已重新整理，以保持 ESP32 穩定。";
  _statusChanged = true;
}

void GameEngine::setSystemEvent(const String &event) {
  setEvent(event);
}
