# SoulBox Demo v4

SoulBox is an ESP32 RPG-style IoT demo. Version 4 focuses on polishing the visible demo flow: a normal Monster exists immediately after boot, Boss replaces the Monster panel with double HP, charts scale to the active monster, Player HP gets its own chart, OLED shows only SoulBox/environment information, and modals use a simple overlay controlled by WebSocket flags.

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
8. Defeating Boss grants or replaces the third skill with `Boss Breaker`.
9. Use `Check Environment` to show environment advice without blocking the ESP32 loop.
10. Watch Serial Monitor for heap reports every 5 seconds during long demo testing.

## Current dashboard buttons

- Attack
- Skill 1 / Skill 2 / Skill 3
- Call Boss
- Rain / Clear / Clouds / Hot / Thunderstorm
- New Day
- Rest / Recover
- Check Environment
- Weather API Tick

`Rest / Recover` is kept as a demo recovery control so testing does not get stuck.

## v4 behavior

- Boot starts with `Monster`, `monsterHp = 100`, `monsterMaxHp = 100`, `isBoss = false`.
- `Call Boss` changes the same Monster panel to `Boss`.
- Boss HP is `MONSTER_NORMAL_HP * 2`, currently `200`.
- Monster HP chart Y-axis follows `monsterMaxHp`, so Boss switches the chart from 100 to 200.
- Dashboard shows Player HP and a Player HP chart.
- Dashboard shows State from `gameState`.
- OLED only shows SoulBox expression plus weather, temperature, humidity, and air quality.
- Modal UI uses `modalOverlay` + `hidden`, and can be closed by pressing OK or clicking outside the modal box.

## Implementation notes

- WebSocket JSON uses `StaticJsonDocument` and a fixed `char` buffer.
- Dashboard modal text is stored in `data/www/app.js`, not in ESP32 C++ strings.
- `Weather API Tick` is a simulated non-blocking update. It is ready to be replaced with real OpenWeather API scheduling later.
- `Check Environment` simulates the 10-minute environment-advice modal trigger.
- Air quality is simulated from current weather, temperature, and humidity. It is ready to be replaced with a real API later.
- OLED reads `oledExpression`, while the dashboard reads `webExpression`.
