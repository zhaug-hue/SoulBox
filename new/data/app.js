// SoulBox fixed frontend controller.
// Keep the dashboard alive even when Chart.js, a field, or a DOM node is missing.

let socket;
let lastEvent = "";
let lastModalKey = "";
let latestData = null;
let chart = null;
let currentSkills = [];

const labels = [];
const hpValues = [];
const maxLog = 12;

const modalText = {
  tutorial: `Demo flow:\n1. Show SoulBox Day 1 story.\n2. Use Normal Attack or one of the three skill slots.\n3. Call Boss, then defeat it to earn a skill.\n4. If skill slots are full, choose which skill to replace.\n5. Sync Real Weather pulls OpenWeather data when Wi-Fi is available.`,
  firstStory: `第一次帶 SoulBox 回家。它會觀察今天的天氣、溫濕度與空氣品質，慢慢學會適合今天的技能。`,
  weatherBuff: `Weather Buff:\nRain = Water Magic x1.5\nClear = Light Sword x1.2\nClouds = balanced\nHot = Fire Blade x1.3\nThunderstorm = Thunder Spark x1.7`,
  satietyWarning: `Low satiety reduces attack damage. Recover satiety before fighting the Boss.`
};

function $(id) {
  return document.getElementById(id);
}

function initCharts() {
  if (typeof Chart === "undefined") {
    console.warn("Chart.js not loaded. Dashboard will continue with fallback HP bar.");
    setChartStatus("Chart.js not loaded. Fallback HP bar is active.");
    return;
  }

  try {
    const monsterCanvas = $("hpChart");
    if (!monsterCanvas) {
      setChartStatus("Monster HP canvas missing. Fallback HP bar is active.");
      return;
    }

    chart = new Chart(monsterCanvas, {
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
        scales: { y: { beginAtZero: true, suggestedMax: 100 } }
      }
    });

    setChartStatus("Chart ready. Waiting for ESP32 status...");
  } catch (error) {
    console.warn("Chart init failed:", error);
    chart = null;
    setChartStatus("Chart init failed. Fallback HP bar is active.");
  }
}

function connect() {
  socket = new WebSocket(`ws://${location.host}/ws`);

  socket.onopen = () => {
    setConnection("Connected");
    setChartStatus("Connected. Waiting for ESP32 status...");
    addBattleLog("WebSocket connected.");
    sendCommand("get_status");
  };

  socket.onclose = () => {
    setConnection("Disconnected. Reconnecting...");
    setChartStatus("WebSocket disconnected. Waiting to reconnect...");
    setTimeout(connect, 1500);
  };

  socket.onerror = () => {
    setConnection("Socket error");
    addBattleLog("WebSocket error. Check ESP32 serial monitor.");
  };

  socket.onmessage = (event) => {
    console.log("WS MESSAGE:", event.data);
    let data;
    try {
      data = JSON.parse(event.data);
    } catch (error) {
      console.warn("Invalid status JSON:", error);
      addBattleLog("Invalid JSON received from ESP32.");
      setChartStatus("Invalid JSON received from ESP32.");
      return;
    }

    try {
      updateDashboard(normalizeStatus(data));
    } catch (error) {
      console.error("Dashboard update failed:", error, data);
      addBattleLog("Dashboard update failed. Check browser console.");
      setChartStatus("Dashboard update failed. Check browser console.");
    }
  };
}

function sendCommand(cmd) {
  console.log("SEND CMD:", cmd);
  if (!socket || socket.readyState !== WebSocket.OPEN) {
    console.warn("WebSocket not open");
    addBattleLog(`WebSocket not open. Command not sent: ${cmd}`);
    return;
  }
  socket.send(JSON.stringify({ cmd }));
}

function setConnection(message) {
  text("connection", message);
}

function syncRealWeather() {
  addBattleLog("Sync Real Weather requested.");
  sendCommand("sync_real_weather");
  setTimeout(() => sendCommand("get_status"), 1200);
}

function requestExternalEnvironment() {
  addBattleLog("External environment details requested.");
  sendCommand("show_external_environment");
  setTimeout(() => {
    sendCommand("get_status");
    showExternalEnvironment();
  }, 500);
}

function applyManualSelect(select) {
  const command = select.value;
  if (!command) return;
  sendCommand(command);
  addBattleLog(`Manual scenario applied: ${select.options[select.selectedIndex].text}`);
  setTimeout(() => sendCommand("get_status"), 500);
  select.selectedIndex = 0;
}

function normalizeStatus(data) {
  const isBoss = Boolean(data.isBoss ?? data.bossBattle);
  const monsterName = data.monsterName || (isBoss ? "Boss" : "Monster");
  const monsterMaxHp = toFiniteNumber(data.monsterMaxHp, isBoss ? 200 : 100);
  const monsterHp = clamp(toFiniteNumber(data.monsterHp, monsterMaxHp), 0, monsterMaxHp);
  const satiety = clamp(toFiniteNumber(data.satiety, 0), 0, 10);
  const weatherType = data.weatherType || data.weather || "-";

  return {
    ...data,
    isBoss,
    bossBattle: isBoss,
    monsterName,
    monsterHp,
    monsterMaxHp,
    satiety,
    weatherType,
    weather: weatherType,
    temperature: toOptionalNumber(data.temperature),
    humidity: toOptionalNumber(data.humidity),
    weatherMultiplier: toFiniteNumber(data.weatherMultiplier, 1),
    webExpression: data.webExpression || data.moodFace || "(^.^)/",
    oledExpression: data.oledExpression || "(^.^)/",
    lastEvent: data.lastEvent || data.event || ""
  };
}

function updateDashboard(data) {
  latestData = data;
  setConnection("Connected - status received");

  text("role", data.role || "Soul Keeper");
  text("dayCount", data.dayCount ?? "-");
  text("mood", data.mood || "-");
  text("moodFace", data.webExpression || data.moodFace || "(^.^)/");
  text("oledExpression", data.oledExpression || "-");
  text("expressionCounter", data.expressionCounter ?? "-");
  text("story", data.story || "SoulBox is ready.");
  text("weatherType", data.weatherType || data.weather || "-");
  text("temperature", formatOptional(data.temperature, 1, " C"));
  text("humidity", formatOptional(data.humidity, 1, "%"));
  text("airQuality", data.airQuality || "-");
  text("weatherBuffName", `${data.weatherBuffName || data.buff || "-"} (${data.weatherMultiplier || 1}x)`);
  text("environmentAdvice", data.healthAdvice || data.environmentAdvice || "-");
  text("lastWeatherApiUpdate", timeAgo(data.lastWeatherApiUpdate));
  text("lastAirQualityUpdate", timeAgo(data.lastAirQualityUpdate));
  text("satiety", `${data.satiety}/10`);
  text("satietyBuffName", data.satietyBuffName || "-");
  text("monsterName", data.monsterName);
  text("monsterHp", `${data.monsterHp}/${data.monsterMaxHp}`);

  setBar("satietyBar", data.satiety, 10);
  setBar("monsterBar", data.monsterHp, data.monsterMaxHp);
  renderSkills(Array.isArray(data.skills) ? data.skills : []);
  updateBattleWindow(data);
  updateHpChart(data.monsterHp, data.monsterMaxHp);

  if (data.hasPendingSkillChoice) {
    showSkillChoiceModal(Array.isArray(data.skills) ? data.skills : [], data.pendingSkill);
  } else {
    handleModals(data);
  }

  const eventText = data.lastEvent || data.event;
  if (eventText && eventText !== lastEvent) {
    addBattleLog(eventText);
    lastEvent = eventText;
  } else if (!lastEvent) {
    addBattleLog("ESP32 status received.");
    lastEvent = "__status_received__";
  }
}

function showSkillChoiceModal(skills, pendingSkill) {
  const overlay = $("modalOverlay");
  if (!overlay) return;

  const body = `New skill found: ${pendingSkill || "-"}\n\nChoose one slot to replace:\n1. ${skills[0] || "Empty"}\n2. ${skills[1] || "Empty"}\n3. ${skills[2] || "Empty"}`;

  text("modalTitle", "Choose Skill");
  const modalBody = $("modalBody");
  if (modalBody) {
    modalBody.innerHTML = `
      <p>${escapeHtml(body).replace(/\n/g, "<br>")}</p>
      <button onclick="sendCommand('replace_skill_1'); closeModal(false);">Replace Slot 1</button>
      <button onclick="sendCommand('replace_skill_2'); closeModal(false);">Replace Slot 2</button>
      <button onclick="sendCommand('replace_skill_3'); closeModal(false);">Replace Slot 3</button>
      <button onclick="sendCommand('discard_new_skill'); closeModal(false);">Discard New Skill</button>
    `;
  }
  overlay.classList.remove("hidden");
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
  // Weather Buff is shown only from the manual Buff Info button.
  if (data.showEnvironmentAdvice) return showExternalEnvironment();
  if (data.showSatietyWarning) return openInfo("Satiety Warning", modalText.satietyWarning, "satiety");
}

function renderSkills(skills) {
  currentSkills = skills;
  for (let i = 0; i < 3; i++) {
    const skillName = skills[i];
    const label = skillName || `Skill ${i + 1}`;
    const button = $(`skillButton${i}`);
    const battleButton = $(`battleSkillButton${i}`);

    if (button) {
      button.textContent = skillName || "Empty Skill";
      button.disabled = false;
    }
    if (battleButton) {
      battleButton.textContent = label;
      battleButton.disabled = false;
    }
  }
}

function useSkillSlot(index) {
  if (!currentSkills[index]) {
    openInfo("Empty Skill Slot", "This skill slot is empty. Defeat a Boss to earn new skills.", `empty-skill-${index}-${Date.now()}`);
    return;
  }
  sendCommand(`skill_${index + 1}`);
}

function updateHpChart(monsterHp, monsterMaxHp) {
  const hp = toFiniteNumber(monsterHp, NaN);
  const maxHp = toFiniteNumber(monsterMaxHp, NaN);
  if (!Number.isFinite(hp) || !Number.isFinite(maxHp) || maxHp <= 0) {
    setChartStatus("Waiting for valid Monster HP data from ESP32...");
    return;
  }

  setChartFallback(hp, maxHp);
  setChartStatus(`Monster HP chart updated: ${hp}/${maxHp}`);
  if (!chart) return;

  chart.options.scales.y.max = maxHp;
  labels.push(new Date().toLocaleTimeString());
  hpValues.push(hp);
  if (labels.length > 20) {
    labels.shift();
    hpValues.shift();
  }
  chart.update();
}

function updateBattleWindow(data) {
  const isBoss = Boolean(data.isBoss || data.bossBattle);
  const enemyName = data.monsterName || (isBoss ? "Boss" : "Monster");
  const enemyHp = toFiniteNumber(data.monsterHp, 0);
  const enemyMaxHp = toFiniteNumber(data.monsterMaxHp, 100);
  const prompt = data.lastEvent || data.event || "SoulBox is waiting for your command.";

  text("battleEnemyName", enemyName);
  text("battleEnemyLevel", isBoss ? "Boss" : "Lv. 1");
  text("battleEnemyHpText", `${enemyHp}/${enemyMaxHp}`);
  text("battlePrompt", prompt);
  setWidth("battleEnemyHpBar", percent(enemyHp, enemyMaxHp));

  const sprite = $("enemySprite");
  if (sprite) sprite.classList.toggle("boss", isBoss);
}

function showExternalEnvironment() {
  if (!latestData) {
    openManualInfo("外在環境詳細數據", "尚未收到 ESP32 狀態資料。請確認 WebSocket 已連線，或先按 Sync Real Weather。", `external-env-empty-${Date.now()}`);
    sendCommand("get_status");
    return;
  }

  const hasExternalWeather = hasValue(latestData.extTemperature) ||
    hasValue(latestData.windSpeed) ||
    hasValue(latestData.pressure) ||
    hasValue(latestData.visibility) ||
    hasValue(latestData.dewPoint);

  const body = `本地環境
天氣: ${latestData.weatherType || latestData.weather || "-"}
本地溫度: ${formatOptional(latestData.temperature, 1, " C")}
本地濕度: ${formatOptional(latestData.humidity, 1, "%")}
空氣品質: ${latestData.airQuality || "-"}
天氣 Buff: ${latestData.weatherBuffName || latestData.buff || "-"}

真實外部氣象
狀態: ${hasExternalWeather ? "已同步" : "尚未同步"}
外部溫度: ${formatOptional(latestData.extTemperature, 1, " C")}
風速/風向: ${formatOptional(latestData.windSpeed, 1, " m/s")} ${latestData.windDir || ""}
氣壓: ${formatOptional(latestData.pressure, 0, " hPa")}
能見度: ${formatOptional(latestData.visibility, 1, " km")}
露點: ${formatOptional(latestData.dewPoint, 1, " C")}

健康提醒
${latestData.healthAdvice || latestData.environmentAdvice || "請先按 Sync Real Weather 產生健康提醒。"}`;

  openManualInfo("外在環境詳細數據", body, `external-environment-${Date.now()}`);
}

function showChatModal() {
  const body = "Chat demo:\nSoulBox can show story messages, battle reactions, and environment reminders here.\nThis modal is kept on the browser side so the ESP32 only sends compact status data.";
  openManualInfo("Join Chat", body, `chat-modal-${Date.now()}`);
}

function resetTutorialAndModals() {
  localStorage.clear();
  lastModalKey = "";
  closeModal(false);
  sendCommand("clear_modals");
  sendCommand("get_status");
}

function openInfo(title, body, key = title) {
  const overlay = $("modalOverlay");
  if (!overlay || !overlay.classList.contains("hidden") || lastModalKey === key) return;
  lastModalKey = key;
  text("modalTitle", title);
  const modalBody = $("modalBody");
  if (modalBody) modalBody.textContent = body;
  overlay.classList.remove("hidden");
}

function openManualInfo(title, body, key = title) {
  lastModalKey = key;
  text("modalTitle", title);
  const modalBody = $("modalBody");
  if (modalBody) modalBody.textContent = body;
  const overlay = $("modalOverlay");
  if (overlay) overlay.classList.remove("hidden");
}

function closeModal(clearServerFlags = true) {
  const overlay = $("modalOverlay");
  if (overlay) overlay.classList.add("hidden");
  lastModalKey = "";
  if (clearServerFlags) sendCommand("clear_modals");
}

function addBattleLog(message) {
  const log = $("battleLog");
  if (!log) return;
  const row = document.createElement("div");
  row.textContent = `${new Date().toLocaleTimeString()} - ${message}`;
  log.prepend(row);
  while (log.children.length > maxLog) log.removeChild(log.lastChild);
}

function text(id, value) {
  const el = $(id);
  if (el) el.textContent = value ?? "-";
}

function setBar(id, value, max) {
  const span = document.querySelector(`#${id} span`);
  if (span) span.style.width = `${percent(toFiniteNumber(value, 0), toFiniteNumber(max, 0))}%`;
}

function setWidth(id, value) {
  const el = $(id);
  if (el) el.style.width = `${value}%`;
}

function percent(value, max) {
  return max > 0 ? Math.max(0, Math.min(100, (value / max) * 100)) : 0;
}

function setChartStatus(message) {
  text("hpChartStatus", message);
}

function setChartFallback(value, max) {
  const bar = document.querySelector("#hpChartFallback span");
  if (bar) bar.style.width = `${percent(value, max)}%`;
}

function timeAgo(ms) {
  const value = Number(ms);
  if (!Number.isFinite(value) || value <= 0) return "Not yet";
  const seconds = Math.floor((Date.now() - performance.timeOrigin - value) / 1000);
  if (!Number.isFinite(seconds) || seconds < 0) return "Just now";
  return `${seconds}s ago`;
}

function formatOptional(value, digits, suffix = "") {
  const number = Number(value);
  if (!Number.isFinite(number)) return "-";
  return `${number.toFixed(digits)}${suffix}`;
}

function toFiniteNumber(value, fallback) {
  const number = Number(value);
  return Number.isFinite(number) ? number : fallback;
}

function toOptionalNumber(value) {
  const number = Number(value);
  return Number.isFinite(number) ? number : undefined;
}

function hasValue(value) {
  return value !== undefined && value !== null && value !== "" && Number.isFinite(Number(value));
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function escapeHtml(value) {
  return String(value)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#039;");
}

document.addEventListener("DOMContentLoaded", () => {
  const overlay = $("modalOverlay");
  if (overlay) {
    overlay.addEventListener("click", (event) => {
      if (event.target.id === "modalOverlay") closeModal();
    });
  }

  initCharts();
  connect();
});
