let socket;
let lastEvent = "";
let lastWeatherData = {}; // 新增：用來記錄最新的氣象包
const labels = [];
const hpValues = [];
const playerLabels = [];
const playerHpValues = [];
const maxLog = 10;
let lastModalKey = "";

const modalText = {
  tutorial: `Demo flow:
1. Show SoulBox Day 1 story.
2. Press Attack to summon and hit a monster.
3. Change weather to grant a weather skill.
4. Use Skill 1/2/3.
5. Call Boss and show Boss skill reward.
6. Use Check Environment to show advice without blocking ESP32.`,
  firstStory: `Day 1: SoulBox wakes up and starts sensing the world.
Weather, satiety, air quality, and battle results all change its expression.`,
  weatherBuff: `Weather Buff:
Rain = Water Magic x1.5
Clear = Light Sword x1.2
Clouds = balanced
Hot = Fire Blade x1.3
Thunderstorm = Thunder Spark x1.7`,
  environmentAdvice: `Environment Advice:
The ESP32 sends compact status flags only.
This dashboard owns the long modal text and display logic.`,
  satietyWarning: `Satiety Warning:
Low satiety reduces damage by 20%.
Use Rest / Recover before fighting Boss.`
};

const chart = new Chart(document.getElementById("hpChart"), {
  type: "line",
  data: {
    labels,
    datasets: [{
      label: "Monster HP",
      data: hpValues,
      borderColor: "#ff6961",
      backgroundColor: "rgba(255, 105, 97, 0.18)",
      tension: 0.25,
      fill: true
    }]
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    animation: false,
    scales: { y: { beginAtZero: true } }
  }
});

const playerChart = new Chart(document.getElementById("playerHpChart"), {
  type: "line",
  data: {
    labels: playerLabels,
    datasets: [{
      label: "Player HP",
      data: playerHpValues,
      borderColor: "#42c783",
      backgroundColor: "rgba(66, 199, 131, 0.18)",
      tension: 0.25,
      fill: true
    }]
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    animation: false,
    scales: { y: { beginAtZero: true, max: 100 } }
  }
});

function connect() {
  socket = new WebSocket(`ws://${location.host}/ws`);
  socket.onopen = () => setConnection("Connected");
  socket.onclose = () => {
    setConnection("Disconnected. Reconnecting...");
    setTimeout(connect, 1500);
  };
  socket.onerror = () => setConnection("Socket error");
  socket.onmessage = (event) => updateDashboard(JSON.parse(event.data));
}

function setConnection(text) {
  document.getElementById("connection").textContent = text;
}

function sendCommand(cmd) {
  if (!socket || socket.readyState !== WebSocket.OPEN) return;
  socket.send(JSON.stringify({ cmd }));
}

function updateDashboard(data) {
  lastWeatherData = data; // 存入最新資料

  text("role", data.role);
  text("dayCount", data.dayCount);
  text("mood", data.mood);
  text("moodFace", data.webExpression || data.moodFace);
  text("oledExpression", data.oledExpression);
  text("expressionCounter", data.expressionCounter);
  text("gameState", data.gameState || data.state);
  text("story", data.story);
  text("weatherType", data.weatherType || data.weather);
  text("temperature", `${Number(data.temperature).toFixed(1)} C`);
  text("humidity", `${Number(data.humidity).toFixed(1)}%`);
  text("airQuality", data.airQuality);
  text("weatherBuffName", `${data.weatherBuffName} (${data.weatherMultiplier}x)`);
  text("environmentAdvice", data.environmentAdvice);
  text("lastWeatherApiUpdate", timeAgo(data.lastWeatherApiUpdate));
  text("lastAirQualityUpdate", timeAgo(data.lastAirQualityUpdate));
  text("satiety", `${data.satiety}/10`);
  text("satietyBuffName", data.satietyBuffName);
  text("monsterName", data.monsterName || (data.isBoss || data.bossBattle ? "Boss" : "Monster"));
  text("monsterHp", `${data.monsterHp}/${data.monsterMaxHp}`);
  text("playerHp", `${data.playerHp}/${data.playerMaxHp}`);

  setBar("satietyBar", data.satiety, 10);
  setBar("monsterBar", data.monsterHp, data.monsterMaxHp);
  setBar("playerBar", data.playerHp, data.playerMaxHp);
  renderSkills(data.skills || []);
  updateHpChart(data.monsterHp, data.monsterMaxHp);
  updatePlayerHpChart(data.playerHp, data.playerMaxHp);
  handleModals(data);

  const eventText = data.lastEvent || data.event;
  if (eventText && eventText !== lastEvent) {
    addBattleLog(eventText);
    lastEvent = eventText;
  }
}

function handleModals(data) {
  if (data.showFirstStory && !localStorage.getItem("firstStoryShown")) {
    localStorage.setItem("firstStoryShown", "true");
    return openInfo("First Story", modalText.firstStory, "firstStory");
  }
  if (data.showTutorial && !localStorage.getItem("tutorialShown")) {
    localStorage.setItem("tutorialShown", "true");
    return openInfo("Tutorial", modalText.tutorial, "tutorial");
  }
  if (data.showWeatherBuff) return openInfo("Weather Buff", modalText.weatherBuff, `weather-${data.weatherType}`);
  if (data.showEnvironmentAdvice) return openInfo("Environment Advice", modalText.environmentAdvice, "environment");
  if (data.showSatietyWarning) return openInfo("Satiety Warning", modalText.satietyWarning, "satiety");
}

function text(id, value) {
  const el = document.getElementById(id);
  if (el) el.textContent = value ?? "-";
}

function setBar(id, value, max) {
  const percent = max > 0 ? Math.max(0, Math.min(100, (value / max) * 100)) : 0;
  document.querySelector(`#${id} span`).style.width = `${percent}%`;
}

function renderSkills(skills) {
  const box = document.getElementById("skills");
  box.innerHTML = "";
  for (let i = 0; i < 3; i++) {
    const row = document.createElement("div");
    row.className = "skill";
    row.innerHTML = `<strong>Skill ${i + 1}</strong><span>${skills[i] || "Empty"}</span>`;
    box.appendChild(row);
  }
}

function updateHpChart(monsterHp, monsterMaxHp) {
  chart.options.scales.y.max = monsterMaxHp || 100;
  labels.push(new Date().toLocaleTimeString());
  hpValues.push(monsterHp);
  if (labels.length > 20) {
    labels.shift();
    hpValues.shift();
  }
  chart.update();
}

function updatePlayerHpChart(playerHp, playerMaxHp) {
  playerChart.options.scales.y.max = playerMaxHp || 100;
  playerLabels.push(new Date().toLocaleTimeString());
  playerHpValues.push(playerHp);
  if (playerLabels.length > 20) {
    playerLabels.shift();
    playerHpValues.shift();
  }
  playerChart.update();
}

function addBattleLog(message) {
  const log = document.getElementById("battleLog");
  const row = document.createElement("div");
  row.textContent = `${new Date().toLocaleTimeString()} - ${message}`;
  log.prepend(row);
  while (log.children.length > maxLog) log.removeChild(log.lastChild);
}

function openInfo(title, body, key = title) {
  const overlay = document.getElementById("modalOverlay");
  if (!overlay.classList.contains("hidden") || lastModalKey === key) return;
  lastModalKey = key;
  document.getElementById("modalTitle").textContent = title;
  document.getElementById("modalBody").textContent = body;
  overlay.classList.remove("hidden");
}

function closeModal() {
  document.getElementById("modalOverlay").classList.add("hidden");
  sendCommand("clear_modals");
}

document.getElementById("modalOverlay").addEventListener("click", function(event) {
  if (event.target.id === "modalOverlay") {
    closeModal();
  }
});

function timeAgo(ms) {
  if (!ms) return "Not yet";
  const seconds = Math.floor((Date.now() - performance.timeOrigin - ms) / 1000);
  if (seconds < 0) return "Just now";
  return `${seconds}s ago`;
}

// 新增：顯示外在真實環境詳細數據的函式
function showExternalEnvironment() {
  if (lastWeatherData.windSpeed === undefined || lastWeatherData.windSpeed === 0) {
    return openInfo("External Environment", "尚未取得外部氣象資料，請稍候或點擊 Sync Real Weather 抓取最新天氣。", "externalEnv");
  }
  
  const body = `🌬️ Wind (風速/風向): 
${lastWeatherData.windSpeed} m/s ${lastWeatherData.windDir}

💧 Humidity (外部濕度): 
${Number(lastWeatherData.humidity).toFixed(1)}%

👁️ Visibility (能見度): 
${Number(lastWeatherData.visibility).toFixed(1)} km

📉 Pressure (大氣壓力): 
${lastWeatherData.pressure} hPa

🌡️ Dew Point (露點溫度): 
${Number(lastWeatherData.dewPoint).toFixed(1)} °C

☀️ UV Index (紫外線指數): 
3 UV (註:免費版API無即時紫外線，此為示意)`;

  openInfo("🌍 外在真實環境 (External Environment)", body, "externalEnv");
}

connect();