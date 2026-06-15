#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <Arduino.h>

enum GameState {
  BOOT,
  IDLE,
  BATTLE,
  RESULT,
  WARNING
};

struct GameStatus {
  int playerHp;
  int playerMaxHp;
  int monsterHp;
  int monsterMaxHp;
  int satiety;
  int dayCount;
  int expressionCounter;

  String role;
  String mood;
  String moodFace;
  String webExpression;
  String oledExpression;
  String gameState;
  String weatherType;
  String buffName;
  String satietyBuffName;
  String airQuality;
  String environmentAdvice;
  String story;
  String monsterName;
  String skills[3];
  String pendingSkill;
  String carriedBossSkill;
  bool hasPendingSkillChoice;
  int skillCount;
  float weatherMultiplier;
  unsigned long lastWeatherApiUpdate;
  unsigned long lastAirQualityUpdate;
  bool showTutorial;
  bool showWeatherBuff;
  bool showEnvironmentAdvice;
  bool showFirstStory;
  bool showSatietyWarning;
  bool weatherBuffModalPending;

  float temperature;
  float humidity;

  bool inBattle;
  bool monsterAlive;
  bool bossBattle;

  String lastEvent;
  float extTemperature;
  float extWindSpeed;
  String extWindDir;
  int extPressure;
  float extVisibility;
  float extDewPoint;
  String healthAdvice;
};

class GameEngine {
public:
  void resolvePendingSkill(int replaceIndex);
  void discardPendingSkill();
  void initGame();
  void initNewPlayer();
  void initDailyStatus();
  void advanceDayFromNtp();
  void startFirstDayTutorial();
  void startBattle(bool isBoss, const String &bossName = "");
  void startNormalMonster();
  void callBoss();
  void callBossWithType(int type);
  void callScheduledBoss();
  void handleMissedBoss();
  void playerAttack(const String &source);
  void normalAttack();
  void useSkill(int skillIndex);
  void webSkillAttack();
  void monsterAttack();
  void feedPet();
  void addSatiety(int amount);
  void reduceSatiety(int amount);

  void applyWeatherBuff(const String &weatherType);
  void setEnvironment(float temperature, float humidity, const String &weatherType);
  void setExtendedWeather(float temperature, float windSpeed, const String &windDir, int pressure, float visibility, float dewPoint);
  void setManualScenario(float temperature, float humidity, const String &weatherType, const String &airQuality);
  void setAirQuality(const String &airQuality);
  void showExternalEnvironmentDetail();
  void checkEnvironment();
  void simulateWeatherApiUpdate();
  void simulateAirQualityUpdate();
  void updateSatiety(int delta);
  void checkMood();
  void checkSatietyBuffDebuff();
  void checkBattleResult();
  void tick();
  void updateExpressionPositive();
  void updateExpressionNegative();
  String getCurrentExpression() const;
  String getCurrentOledExpression() const;
  void updateEnvironmentAdvice();
  void clearModalFlags();
  void grantFirstDayWeatherSkill();
  void grantBossSkill();
  void handleBossFailedPenalty();
  bool canAddSkill(const String &skillName) const;
  void addSkill(const String &skillName);
  void replaceSkill(int oldSkillIndex, const String &newSkillName);
  void updateHealthAdvice();
  void optimizeMemory();
  void setSystemEvent(const String &event);

  GameStatus getStatus() const;
  bool hasStatusChanged() const;
  void clearStatusChanged();

private:
  GameStatus _status;
  GameState _state = BOOT;
  float _weatherMultiplier = 1.0;
  bool _statusChanged = true;
  bool _firstDayMonsterDefeated = false;
  bool _tutorialMonsterActive = true;
  unsigned long _stateChangedMs = 0;

  void setState(GameState state);
  void setEvent(const String &event);
  String stateToString(GameState state) const;
  int calculateDamage(int baseDamage) const;
  int clampInt(int value, int minValue, int maxValue) const;
};

#endif
