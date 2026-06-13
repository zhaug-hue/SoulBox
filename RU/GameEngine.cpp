#include "GameEngine.h"
#include "config.h"

void GameEngine::initGame() {
  initNewPlayer();
}

void GameEngine::initNewPlayer() {
  _status.playerHp = PLAYER_MAX_HP;
  _status.playerMaxHp = PLAYER_MAX_HP;
  _status.monsterName = "Monster";
  _status.monsterHp = MONSTER_NORMAL_HP;
  _status.monsterMaxHp = MONSTER_NORMAL_HP;
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
  
  // 初始化擴充氣象變數
  _status.extWindSpeed = 0;
  _status.extWindDir = "-";
  _status.extPressure = 0;
  _status.extVisibility = 0;
  _status.extDewPoint = 0;

  _status.showTutorial = true;
  _status.showWeatherBuff = false;
  _status.showEnvironmentAdvice = false;
  _status.showFirstStory = true;
  _status.showSatietyWarning = false;
  _status.weatherBuffModalPending = false;
  _status.temperature = 0;
  _status.humidity = 0;
  _status.inBattle = false;
  _status.monsterAlive = true;
  _status.bossBattle = false;
  setState(IDLE);
  setEvent("SoulBox woke up on Day 1.");
  startFirstDayTutorial();
}

void GameEngine::initDailyStatus() {
  _status.dayCount += 1;
  _status.satiety = SATIETY_MAX;
  _status.playerHp = PLAYER_MAX_HP;
  _status.monsterHp = MONSTER_NORMAL_HP;
  _status.monsterMaxHp = MONSTER_NORMAL_HP;
  _status.monsterName = "Monster";
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
  _status.story = "A new day begins. SoulBox is ready to learn from the weather.";
  checkSatietyBuffDebuff();
  setState(IDLE);
  setEvent("Daily status initialized.");
}

void GameEngine::startFirstDayTutorial() {
  grantFirstDayWeatherSkill();
  setEvent("Tutorial: press Attack, try weather buttons, then call Boss.");
}

void GameEngine::startBattle(bool isBoss) {
  _status.bossBattle = isBoss;
  _status.monsterName = isBoss ? "Boss" : "Monster";
  _status.monsterMaxHp = isBoss ? MONSTER_NORMAL_HP * 2 : MONSTER_NORMAL_HP;
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
  _status.monsterName = "Boss";
  _status.bossBattle = true;
  _status.monsterMaxHp = MONSTER_NORMAL_HP * 2;
  _status.monsterHp = _status.monsterMaxHp;
  _status.monsterAlive = true;
  _status.inBattle = true;
  setState(BATTLE);
  setEvent("Boss appeared!");
}

void GameEngine::playerAttack(const String &source) {
  if (!_status.inBattle || !_status.monsterAlive) {
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

  const int damage = _status.bossBattle ? 14 : 9;
  _status.playerHp = clampInt(_status.playerHp - damage, 0, _status.playerMaxHp);
  updateExpressionNegative();

  if (_status.playerHp <= 0) {
    _status.inBattle = false;
    _status.monsterAlive = true;
    _status.mood = "sad";
    _status.moodFace = getCurrentExpression();
    setState(RESULT);
    if (_status.bossBattle) {
      handleBossFailedPenalty();
    } else {
      setEvent("SoulBox was hit too hard. Try weather skills.");
    }
  } else {
    _statusChanged = true;
  }
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
}

// 新增：寫入外部氣象資料的實作函式
void GameEngine::setExtendedWeather(float windSpeed, const String &windDir, int pressure, float visibility, float dewPoint) {
  _status.extWindSpeed = windSpeed;
  _status.extWindDir = windDir;
  _status.extPressure = pressure;
  _status.extVisibility = visibility;
  _status.extDewPoint = dewPoint;
  _statusChanged = true;
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
    addSatiety(_status.bossBattle ? 2 : 1);
    _status.mood = "happy";
    updateExpressionPositive();
    setState(RESULT);
    if (_status.bossBattle) {
      grantBossSkill();
      setEvent("Boss defeated. SoulBox learned a Boss skill.");
    } else {
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
    return "( ^_^ )";
  }
  if (_status.expressionCounter >= 1) {
    return "( 'u' )";
  }
  if (_status.expressionCounter <= -3) {
    return "( T_T )";
  }
  if (_status.expressionCounter <= -1) {
    return "( >_< )";
  }
  return "( ._. )";
}

String GameEngine::getCurrentOledExpression() const {
  if (_status.expressionCounter >= 3) {
    return "(*^_^*)";
  }
  if (_status.expressionCounter >= 1) {
    return "(^_^)v";
  }
  if (_status.expressionCounter <= -3) {
    return "(T_T)";
  }
  if (_status.expressionCounter <= -1) {
    return "(>_<;)";
  }
  return "(^.^)/";
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
  if (canAddSkill("Boss Breaker")) {
    addSkill("Boss Breaker");
  } else {
    replaceSkill(2, "Boss Breaker");
  }
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
