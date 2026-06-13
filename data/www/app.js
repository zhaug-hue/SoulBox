let socket;
let lastEvent = "";
const labels = [];
const hpValues = [];
let currentSkills = [];
const maxLog = 10;
let lastModalKey = "";

const modalText = {
  tutorial: `Demo flow:
1. Show SoulBox Day 1 story.
2. Use the 2x2 attack panel.
3. Normal Attack is always available.
4. Boss rewards new skills.
5. If you have 3 skills, choose 3 from old skills plus the new one.
6. Sync Real Weather can pull OpenWeather data.`,
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
Use the satiety recovery button before fighting Boss.`
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

  setBar("satietyBar", data.satiety, 10);
  setBar("monsterBar", data.monsterHp, data.monsterMaxHp);
  renderSkills(data.skills || []);
  updateHpChart(data.monsterHp, data.monsterMaxHp);

  if (data.hasPendingSkillChoice) {
    showSkillChoiceModal(data.skills || [], data.pendingSkill);
  } else {
    handleModals(data);
  }

  const eventText = data.lastEvent || data.event;
  if (eventText && eventText !== lastEvent) {
    addBattleLog(eventText);
    lastEvent = eventText;
  }
}

function showSkillChoiceModal(skills, pendingSkill) {
  const text = `
你獲得新技能：${pendingSkill}

目前技能：
1. ${skills[0] || "空槽"}
2. ${skills[1] || "空槽"}
3. ${skills[2] || "空槽"}

請從 4 個技能中留下 3 個。`;

  document.getElementById("modalTitle").textContent = "選擇技能";
  document.getElementById("modalBody").innerHTML = `
    <p>${text.replace(/\n/g, "<br>")}</p>
    <button onclick="sendCommand('replace_skill_1'); closeModal(false);">替換技能 1</button>
    <button onclick="sendCommand('replace_skill_2'); closeModal(false);">替換技能 2</button>
    <button onclick="sendCommand('replace_skill_3'); closeModal(false);">替換技能 3</button>
    <button onclick="sendCommand('discard_new_skill'); closeModal(false);">不要新技能</button>
  `;

  document.getElementById("modalOverlay").classList.remove("hidden");
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
  currentSkills = skills;

  for (let i = 0; i < 3; i++) {
    const button = document.getElementById(`skillButton${i}`);
    const skillName = skills[i];

    if (skillName) {
      button.textContent = skillName;
      button.disabled = false;
    } else {
      button.textContent = "技能空槽";
      button.disabled = true;
    }
  }
}

function useSkillSlot(index) {
  if (!currentSkills[index]) {
    openInfo("技能空槽", "這個技能格目前還沒有技能。", `empty-skill-${index}`);
    return;
  }

  sendCommand(`skill_${index + 1}`);
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

function closeModal(clearServerFlags = true) {
  document.getElementById("modalOverlay").classList.add("hidden");

  if (clearServerFlags) {
    sendCommand("clear_modals");
  }
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

connect();
