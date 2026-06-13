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
  _status.monsterName = "Monster";
  _status.monsterHp = 45;
  _status.monsterMaxHp = 45;
  _status.satiety = 10;
  _status.dayCount = 1;
  _status.expressionCounter = 0;
  _status.role = "Soul Keeper";
  _status.mood = "curious";
  _status.moodFace = getCurrentExpression();
  _status.webExpression = getCurrentExpression();
  _status.oledExpression = getCurrentOledExpression();
  _status.weatherType = "Clouds";
  _status.buffName = "Normal Attack x1.0";
  _status.satietyBuffName = "Full Soul: damage +25%";
  _status.airQuality = "Good";
  _status.environmentAdvice = "Cloudy air is stable. Watch the monster and prepare a skill.";
  _status.story = "Day 1: SoulBox woke up. Weather will shape its first magic.";
  _status.skills[0] = "Basic Slash";
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
  _status.healthAdvice = "Waiting for real weather data.";
  _status.inBattle = false;
  _status.monsterAlive = true;
  _status.bossBattle = false;
  _firstDayMonsterDefeated = false;
  _tutorialMonsterActive = true;
  setState(IDLE);
  setEvent("SoulBox woke up on Day 1.");
  startFirstDayTutorial();
}

void GameEngine::initDailyStatus() {
  _status.dayCount += 1;
  _status.satiety = SATIETY_MAX;
  _status.playerHp = PLAYER_MAX_HP;
  _status.monsterHp = 0;
  _status.monsterMaxHp = MONSTER_NORMAL_HP;
  _status.monsterName = "Monster";
  _status.monsterHp = MONSTER_NORMAL_HP;
  _status.monsterAlive = true;
  _status.inBattle = false;
  _status.bossBattle = false;
  _status.expressionCounter = 0;
  _status.mood = "curious";
  _status.moodFace = getCurrentExpression();
  _status.webExpression = getCurrentExpression();
  _status.oledExpression = getCurrentOledExpression();
  _status.showTutorial = true;
  _status.showFirstStory = true;
  _status.showEnvironmentAdvice = true;
  _firstDayMonsterDefeated = true;
  _tutorialMonsterActive = false;
  _status.story = "A new day begins. SoulBox is ready to learn from the weather.";
  checkSatietyBuffDebuff();
  setState(IDLE);
  setEvent("Daily status initialized.");
}

void GameEngine::advanceDayFromNtp() {
  _status.dayCount += 1;
  reduceSatiety(DAILY_SATIETY_DECAY);
  _status.monsterName = "Monster";
  _status.monsterMaxHp = MONSTER_NORMAL_HP;
  _status.monsterHp = MONSTER_NORMAL_HP;
  _status.monsterAlive = true;
  _status.inBattle = false;
  _status.bossBattle = false;
  _status.expressionCounter = 0;
  _status.showEnvironmentAdvice = true;
  _firstDayMonsterDefeated = true;
  _tutorialMonsterActive = false;
  _status.story = "A real new day has passed. SoulBox used satiety overnight and a new monster appeared.";
  if (_status.satiety <= 3) {
    setState(WARNING);
  } else {
    setState(IDLE);
  }
  setEvent("NTP daily reset: satiety decreased and monster was reset.");
}

void GameEngine::startFirstDayTutorial() {
  setEvent("Tutorial: defeat the first monster with three normal attacks to learn today's weather skill.");
}

void GameEngine::startBattle(bool isBoss) {
  _status.bossBattle = isBoss;
  _status.monsterName = isBoss ? "Boss" : "Monster";
  if (isBoss) {
    _status.monsterMaxHp = MONSTER_NORMAL_HP * 2;
    reduceSatiety(2);
  } else if (_status.dayCount == 1 && !_firstDayMonsterDefeated) {
    _status.monsterMaxHp = 45;
    _tutorialMonsterActive = true;
  } else {
    _status.monsterMaxHp = MONSTER_NORMAL_HP;
    reduceSatiety(1);
    _tutorialMonsterActive = false;
  }
  _status.monsterHp = _status.monsterMaxHp;
  _status.monsterAlive = true;
  _status.inBattle = true;
  setState(BATTLE);
  setEvent(isBoss ? "Boss appeared. Use your collected skills." : "A monster appeared.");
}

void GameEngine::startNormalMonster() {
  startBattle(false);
}

void GameEngine::callBoss() {
  startBattle(true);
}

void GameEngine::callScheduledBoss() {
  if (_status.inBattle && _status.monsterAlive && !_status.bossBattle) {
    _status.inBattle = false;
    _status.monsterAlive = false;
    reduceSatiety(1);
    setEvent("Unfinished monster escaped before mealtime Boss. Satiety decreased.");
  } else if (_status.inBattle && _status.monsterAlive && _status.bossBattle) {
    handleMissedBoss();
  }
  startBattle(true);
  setEvent("Scheduled mealtime Boss appeared.");
}

void GameEngine::handleMissedBoss() {
  if (_status.skillCount > 1) {
    const int index = _status.skillCount - 1;
    _status.carriedBossSkill = _status.skills[index];
    _status.skills[index] = "";
    _status.skillCount -= 1;
    setEvent("Boss was missed. A skill was sealed for the next Boss reward.");
  } else {
    reduceSatiety(1);
    setEvent("Boss was missed. Basic Attack stayed safe, but satiety decreased.");
  }
}

void GameEngine::playerAttack(const String &source) {
  if (!_status.inBattle || !_status.monsterAlive) {
    if (_status.bossBattle && !_status.monsterAlive) {
      setEvent("Boss is already defeated. Call Boss again or start a new day.");
      return;
    }
    startBattle(false);
  }

  const int damage = calculateDamage(source == "button" ? 20 : 15);
  _status.monsterHp = clampInt(_status.monsterHp - damage, 0, _status.monsterMaxHp);
  reduceSatiety(1);
  updateExpressionPositive();

  setEvent(source + " attack caused " + String(damage) + " damage.");
  checkBattleResult();

  if (_status.inBattle && _status.monsterAlive) {
    monsterAttack();
  }
}

void GameEngine::normalAttack() {
  playerAttack("normal");
}

void GameEngine::useSkill(int skillIndex) {
  if (skillIndex < 0 || skillIndex >= _status.skillCount) {
    setEvent("That skill slot is empty.");
    return;
  }

  if (!_status.inBattle || !_status.monsterAlive) {
    if (_status.bossBattle && !_status.monsterAlive) {
      setEvent("Boss is already defeated. Call Boss again or start a new day.");
      return;
    }
    startBattle(false);
  }

  const int baseDamage = 24 + skillIndex * 8;
  const int damage = calculateDamage(baseDamage);
  _status.monsterHp = clampInt(_status.monsterHp - damage, 0, _status.monsterMaxHp);
  reduceSatiety(_status.bossBattle ? 2 : 1);
  updateExpressionPositive();
  setEvent(_status.skills[skillIndex] + " caused " + String(damage) + " damage.");
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
  setEvent(_status.monsterName + " countered. SoulBox expression changed.");
}

void GameEngine::feedPet() {
  addSatiety(2);
  _status.playerHp = clampInt(_status.playerHp + 8, 0, _status.playerMaxHp);
  checkMood();
  updateExpressionPositive();
  setEvent("SoulBox rested. Satiety and HP recovered.");
}

void GameEngine::addSatiety(int amount) {
  updateSatiety(amount);
}

void GameEngine::reduceSatiety(int amount) {
  updateSatiety(-amount);
}

void GameEngine::applyWeatherBuff(const String &weatherType) {
  _status.weatherType = weatherType;

  if (weatherType == "Rain") {
    _weatherMultiplier = 1.5;
    _status.buffName = "Water Magic x1.5";
    _status.airQuality = "Wet";
    _status.environmentAdvice = "Rain strengthens water magic. Use the rain skill before Boss.";
    addSkill("Rain Ripple");
    _status.showWeatherBuff = true;
    _status.weatherBuffModalPending = true;
  } else if (weatherType == "Clear") {
    _weatherMultiplier = 1.2;
    _status.buffName = "Light Sword x1.2";
    _status.airQuality = "Bright";
    _status.environmentAdvice = "Clear weather gives steady light damage.";
    addSkill("Light Sword");
    _status.showWeatherBuff = true;
    _status.weatherBuffModalPending = true;
  } else if (weatherType == "Thunderstorm") {
    _weatherMultiplier = 1.7;
    _status.buffName = "Thunder Spark x1.7";
    _status.airQuality = "Electric";
    _status.environmentAdvice = "Thunderstorm is risky but powerful.";
    addSkill("Thunder Spark");
    _status.showWeatherBuff = true;
    _status.weatherBuffModalPending = true;
  } else if (weatherType == "Hot") {
    _weatherMultiplier = 1.3;
    _status.buffName = "Fire Blade x1.3";
    _status.airQuality = "Dry";
    _status.environmentAdvice = "Hot air boosts fire damage but drains energy.";
    addSkill("Fire Blade");
    _status.showWeatherBuff = true;
    _status.weatherBuffModalPending = true;
  } else {
    _weatherMultiplier = 1.0;
    _status.buffName = "Normal Attack x1.0";
    _status.airQuality = "Good";
    _status.environmentAdvice = "Clouds are balanced. Save satiety for Boss.";
    addSkill("Cloud Guard");
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
  setEvent("Manual environment scenario updated.");
}

void GameEngine::setAirQuality(const String &airQuality) {
  _status.airQuality = airQuality;
  updateHealthAdvice();
  _status.environmentAdvice = _status.healthAdvice;
  _status.showEnvironmentAdvice = true;
  setEvent("Air quality scenario updated: " + airQuality);
}

void GameEngine::showExternalEnvironmentDetail() {
  updateHealthAdvice();
  _status.environmentAdvice = _status.healthAdvice;
  _status.showEnvironmentAdvice = true;
  setEvent("External environment details requested.");
}

void GameEngine::checkEnvironment() {
  simulateAirQualityUpdate();
  _status.showEnvironmentAdvice = true;
  setEvent("Environment checked: " + _status.weatherType + ", " + _status.airQuality + " air.");
}

void GameEngine::simulateWeatherApiUpdate() {
  applyWeatherBuff(_status.weatherType);
  _status.showWeatherBuff = true;
  _status.lastWeatherApiUpdate = millis();
  setEvent("Weather API update simulated.");
}

void GameEngine::simulateAirQualityUpdate() {
  if (_status.humidity >= 80) {
    _status.airQuality = "Humid";
  } else if (_status.temperature >= 32) {
    _status.airQuality = "Dry";
  } else if (_status.weatherType == "Thunderstorm") {
    _status.airQuality = "Electric";
  } else {
    _status.airQuality = "Good";
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
    _status.mood = "happy";
  } else if (_status.satiety <= 2) {
    _status.mood = "hungry";
    if (_state != BATTLE && _state != RESULT) {
      setState(WARNING);
    }
  } else {
    _status.mood = "normal";
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
    _status.satietyBuffName = "Full Soul: damage +25%";
    _status.showSatietyWarning = false;
  } else if (_status.satiety <= 3) {
    _status.satietyBuffName = "Hungry Debuff: damage -20%";
    _status.showSatietyWarning = true;
    if (_state != BATTLE && _state != RESULT) {
      setState(WARNING);
    }
  } else {
    _status.satietyBuffName = "Normal satiety";
  }
  _statusChanged = true;
}

void GameEngine::checkBattleResult() {
  if (_status.monsterHp <= 0) {
    _status.monsterAlive = false;
    _status.inBattle = false;
    addSatiety(_status.bossBattle ? 2 : 2);
    _status.mood = "happy";
    updateExpressionPositive();
    setState(RESULT);
    if (_status.bossBattle) {
      grantBossSkill();
      setEvent("Boss defeated. SoulBox learned a Boss skill.");
    } else {
      if (_status.dayCount == 1 && !_firstDayMonsterDefeated) {
        _firstDayMonsterDefeated = true;
        _tutorialMonsterActive = false;
        grantFirstDayWeatherSkill();
        setEvent("First monster defeated. SoulBox learned today's weather skill.");
        return;
      }
      setEvent("Monster defeated. SoulBox expression improved.");
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
    if (_status.weatherType == "Rain") {
      addSkill("Rain Ripple");
    } else if (_status.weatherType == "Clear") {
      addSkill("Light Sword");
    } else if (_status.weatherType == "Hot") {
      addSkill("Fire Blade");
    } else {
      addSkill("Cloud Guard");
    }
  }
}

void GameEngine::grantBossSkill() {
  String newSkill = "Boss Breaker";
  if (_status.carriedBossSkill != "" && (millis() % 2 == 0)) {
    newSkill = _status.carriedBossSkill;
  }
  _status.carriedBossSkill = "";

  if (canAddSkill(newSkill)) {
    addSkill(newSkill);
    setEvent("Boss defeated. New skill learned: " + newSkill);
  } else {
    _status.pendingSkill = newSkill;
    _status.hasPendingSkillChoice = true;
    setEvent("Boss defeated. Choose 3 skills from 4.");
  }
  _statusChanged = true;
}

void GameEngine::handleBossFailedPenalty() {
  reduceSatiety(1);
  _status.story = "Boss was too strong. SoulBox needs better weather timing.";
  setEvent("Boss failed. Satiety decreased and SoulBox became tired.");
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
    case BOOT: return "BOOT";
    case IDLE: return "IDLE";
    case BATTLE: return "BATTLE";
    case RESULT: return "RESULT";
    case WARNING: return "WARNING";
  }
  return "IDLE";
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
    setEvent("Skill updated: " + _status.pendingSkill);
  }

  _status.pendingSkill = "";
  _status.hasPendingSkillChoice = false;
  _statusChanged = true;
}

void GameEngine::discardPendingSkill() {
  if (!_status.hasPendingSkillChoice) {
    return;
  }

  setEvent("New skill discarded: " + _status.pendingSkill);
  _status.pendingSkill = "";
  _status.hasPendingSkillChoice = false;
  _statusChanged = true;
}

void GameEngine::updateHealthAdvice() {
  String advice = "";

  if (_status.temperature >= 32.0 || _status.extTemperature >= 32.0) {
    advice += "Temperature is high. Drink water and avoid staying under direct sun too long.\n";
  } else if ((_status.temperature > 0 && _status.temperature <= 15.0) || (_status.extTemperature > 0 && _status.extTemperature <= 15.0)) {
    advice += "Temperature is low. Keep warm before going outside.\n";
  }

  if (_status.humidity >= 80.0) {
    advice += "Humidity is high. Consider dehumidifying the room.\n";
  } else if (_status.humidity > 0 && _status.humidity <= 40.0) {
    advice += "Air is dry. Drink water and watch skin dryness.\n";
  }

  if (_status.extPressure > 0 && _status.extPressure < 1000) {
    advice += "Pressure is low. If you are sensitive to headaches, take more rest.\n";
  } else if (_status.extPressure > 1025) {
    advice += "Pressure is high. Pay attention to cardiovascular comfort.\n";
  }

  if (_status.extVisibility > 0 && _status.extVisibility < 5.0) {
    advice += "Visibility is low. Outdoor air or fog may be poor, so be careful outside.\n";
  }

  if (_status.extWindSpeed > 8.0) {
    advice += "Wind is strong. Watch loose objects and traffic safety.\n";
  }

  if (_status.extDewPoint >= 24.0) {
    advice += "Dew point is high. It may feel muggy, so rest indoors if possible.\n";
  }

  if (_status.airQuality == "Poor") {
    advice += "Air quality is poor. Reduce outdoor activity and consider closing windows.\n";
  } else if (_status.airQuality == "Moderate") {
    advice += "Air quality is acceptable but not perfect. Sensitive users should watch outdoor time.\n";
  } else if (_status.airQuality == "Good") {
    advice += "Air quality is good. If outdoor conditions are safe, opening a window is reasonable.\n";
  } else if (_status.airQuality == "Humid") {
    advice += "Air feels humid. Dehumidifying may make the room more comfortable.\n";
  } else if (_status.airQuality == "Dry") {
    advice += "Air feels dry. Hydrate and consider adding moisture indoors.\n";
  } else if (_status.airQuality == "Electric") {
    advice += "Thunderstorm-like air detected. Stay aware of outdoor safety.\n";
  }

  if (advice == "") {
    advice = "Current environment looks comfortable. Keep a steady pace.";
  }

  _status.healthAdvice = advice;
  _statusChanged = true;
}

void GameEngine::optimizeMemory() {
  if (ESP.getFreeHeap() >= 35000) {
    return;
  }

  _status.lastEvent = "Memory guard optimized dashboard text.";
  _status.story = "SoulBox cleared temporary thoughts and kept the core game state.";
  _status.environmentAdvice = "Long environment text was refreshed to keep the ESP32 stable.";
  _statusChanged = true;
}

void GameEngine::setSystemEvent(const String &event) {
  setEvent(event);
}
