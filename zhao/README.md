# SoulBox Demo v5

SoulBox is an ESP32 RPG-style IoT demo. Version 5 keeps the v4 visible demo flow, the 2x2 attack panel, empty skill slots, Boss skill selection, satiety recovery, and adds teammate-provided OpenWeather API synchronization.

## Arduino libraries

Install these libraries before compiling:

- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson
- DHT sensor library
- Adafruit Unified Sensor
- U8g2

## Hardware pins

| Part | Pin |
| --- | --- |
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |
| DHT11 | GPIO 27 |
| Attack button | GPIO 32 |
| Buzzer | GPIO 25 |
| LED | GPIO 26 |

The attack button uses `INPUT_PULLUP`, so connect the button between GPIO 32 and GND.

## Upload flow

1. Open `SoulBox.ino` in Arduino IDE.
2. Edit `config.h` and fill in `WIFI_SSID` and `WIFI_PASSWORD`.
3. Upload the sketch.
4. Upload the full `data/www` folder to SPIFFS. It now contains `index.html`, `style.css`, and `app.js`.
5. Open the IP address printed in Serial Monitor.

## Main demo flow

1. ESP32 boots and shows the SoulBox expression on OLED.
2. The dashboard shows Day 1 story, weather, temperature, humidity, air quality, satiety, skills, and battle log.
3. Press the physical button or dashboard Attack to summon a normal monster and attack it.
4. Change weather with Rain / Clear / Clouds / Hot / Thunderstorm to grant weather skills and trigger the Weather Buff modal.
5. Use Skill 1 / Skill 2 / Skill 3 to attack.
6. Call Boss after showing the first weather skill.
7. Satiety changes during battle. High satiety gives +25% damage; low satiety gives -20% damage and warning feedback.
8. Defeating Boss grants `Boss Breaker`. If 3 skills already exist, choose 3 from the old skills plus the new one.
9. Use `Check Environment` to show environment advice without blocking the ESP32 loop.
10. Use `Sync Real Weather` to pull current OpenWeather data.
11. Watch Serial Monitor for heap reports every 5 seconds during long demo testing.

## Current dashboard buttons

- 2x2 Attack Panel: Normal Attack + three skill slots
- Call Boss
- Rain / Clear / Clouds / Hot / Thunderstorm
- Sync Real Weather
- New Day
- Satiety recovery
- Check Environment
- Weather API Tick

The satiety recovery button restores satiety only; it does not heal player HP.

## v5 behavior

- Boot starts with `Monster`, `monsterHp = 100`, `monsterMaxHp = 100`, `isBoss = false`.
- `Call Boss` changes the same Monster panel to `Boss`.
- Boss HP is `MONSTER_NORMAL_HP * 2`, currently `200`.
- Monster HP chart Y-axis follows `monsterMaxHp`, so Boss switches the chart from 100 to 200.
- Dashboard shows State from `gameState`.
- OLED only shows SoulBox expression plus weather, temperature, humidity, and air quality.
- Modal UI uses `modalOverlay` + `hidden`, and can be closed by pressing OK or clicking outside the modal box.
- OpenWeather API can update weather, temperature, and humidity using `sync_real_weather`.
- DHT11 local updates and OpenWeather API updates are scheduled separately.
- Manual weather buttons still work and intentionally override weather type for demo control.

## Implementation notes

- WebSocket JSON uses `StaticJsonDocument` and a fixed `char` buffer.
- Dashboard modal text is stored in `data/www/app.js`, not in ESP32 C++ strings.
- `Weather API Tick` is still a simulated GameEngine modal/status tick.
- `Sync Real Weather` calls the real OpenWeather API through `WeatherManager::updateFromAPI()`.
- `Check Environment` simulates the 10-minute environment-advice modal trigger.
- Air quality is simulated from current weather, temperature, and humidity. It is ready to be replaced with a real API later.
- OLED reads `oledExpression`, while the dashboard reads `webExpression`.
- OpenWeather settings are in `config.h`. Replace the API key before public sharing.
