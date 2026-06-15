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
  tutorial: `遊戲流程：\n1. 查看 SoulBox 第一天的故事。\n2. 使用普通攻擊或三個技能槽中的技能。\n3. 召喚魔王，擊敗它以獲得新技能。\n4. 如果技能槽已滿，選擇要替換的技能。\n5. 同步真實天氣會在 Wi-Fi 可用時抓取 OpenWeather 數據。`,
  firstStory: `第一次帶 SoulBox 回家。它會觀察今天的天氣、溫濕度與空氣品質，慢慢學會適合今天的技能。`,
  weatherBuff: `天氣加成：\n下雨 = 水魔法 x1.5\n晴天 = 光之劍 x1.2\n多雲 = 均衡\n炎熱 = 火焰之刃 x1.3\n雷陣雨 = 雷電火花 x1.7`,
  satietyWarning: `飽食度過低會降低攻擊傷害。在挑戰魔王前請先補充飽食度。`
};

function $(id) {
  return document.getElementById(id);
}

function initCharts() {
  if (typeof Chart === "undefined") {
    console.warn("未載入 Chart.js。儀表板將繼續使用備用血量條。");
    setChartStatus("未載入 Chart.js。備用血量條已啟用。");
    return;
  }

  try {
    const monsterCanvas = $("hpChart");
    if (!monsterCanvas) {
      setChartStatus("找不到怪物 HP 畫布。備用血量條已啟用。");
      return;
    }

    chart = new Chart(monsterCanvas, {
      type: "line",
      data: {
        labels,
        datasets: [{
          label: "怪物血量",
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

    setChartStatus("圖表已就緒。正在等待 ESP32 狀態...");
  } catch (error) {
    console.warn("圖表初始化失敗:", error);
    chart = null;
    setChartStatus("圖表初始化失敗。備用血量條已啟用。");
  }
}

function connect() {
  socket = new WebSocket(`ws://${location.host}/ws`);

  socket.onopen = () => {
    setConnection("已連線");
    setChartStatus("已連線。正在等待 ESP32 狀態...");
    addBattleLog("WebSocket 已連線。");
    sendCommand("get_status");
  };

  socket.onclose = () => {
    setConnection("連線中斷。正在重新連線...");
    setChartStatus("WebSocket 連線中斷。等待重新連線...");
    setTimeout(connect, 1500);
  };

  socket.onerror = () => {
    setConnection("通訊埠錯誤");
    addBattleLog("WebSocket 錯誤。請檢查 ESP32 序列監控器。");
  };

  socket.onmessage = (event) => {
    console.log("WS 訊息:", event.data);
    let data;
    try {
      data = JSON.parse(event.data);
    } catch (error) {
      console.warn("無效的狀態 JSON:", error);
      addBattleLog("從 ESP32 收到無效的 JSON。");
      setChartStatus("從 ESP32 收到無效的 JSON。");
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
  addBattleLog("要求同步真實天氣。");
  sendCommand("sync_real_weather");
  setTimeout(() => sendCommand("get_status"), 1200);
}

let ffTimer = null;
function startFastForwardDay() {
  const btn = $("ffDayBtn");
  if (!btn || ffTimer) return;

  let timeLeft = 5;
  btn.disabled = true;
  btn.textContent = `倒數 ${timeLeft} 秒...`;

  ffTimer = setInterval(() => {
    timeLeft--;
    if (timeLeft <= 0) {
      clearInterval(ffTimer);
      ffTimer = null;
      btn.disabled = false;
      btn.textContent = "5秒過一天";
      sendCommand("new_day");
      addBattleLog("快進一天完成：天數增加、飽食度回滿、怪物已刷新。");
    } else {
      btn.textContent = `倒數 ${timeLeft} 秒...`;
    }
  }, 1000);
}

function requestExternalEnvironment() {
  addBattleLog("要求外在環境詳細數據。");
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
  addBattleLog(`已套用模擬情境：${select.options[select.selectedIndex].text}`);
  setTimeout(() => sendCommand("get_status"), 500);
  select.selectedIndex = 0;
}

function normalizeStatus(data) {
  const isBoss = Boolean(data.isBoss ?? data.bossBattle);
  const monsterName = data.monsterName || (isBoss ? "魔王" : "怪物");
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
  setConnection("已連線 - 收到狀態");

  text("role", data.role || "靈魂守護者");
  text("dayCount", data.dayCount ?? "-");
  text("mood", data.mood || "-");
  text("moodFace", data.webExpression || data.moodFace || "(^.^)/");
  text("oledExpression", data.oledExpression || "-");
  text("expressionCounter", data.expressionCounter ?? "-");
  text("story", data.story || "SoulBox 已就緒。");
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
    addBattleLog("收到 ESP32 狀態。");
    lastEvent = "__status_received__";
  }
}

function showSkillChoiceModal(skills, pendingSkill) {
  const overlay = $("modalOverlay");
  if (!overlay) return;

  const body = `發現新技能：${pendingSkill || "-"}\n\n請選擇一個欄位進行替換：\n1. ${skills[0] || "空"}\n2. ${skills[1] || "空"}\n3. ${skills[2] || "空"}`;

  text("modalTitle", "選擇技能");
  const modalBody = $("modalBody");
  if (modalBody) {
    modalBody.innerHTML = `
      <p>${escapeHtml(body).replace(/\n/g, "<br>")}</p>
      <button onclick="sendCommand('replace_skill_1'); closeModal(false);">替換欄位 1</button>
      <button onclick="sendCommand('replace_skill_2'); closeModal(false);">替換欄位 2</button>
      <button onclick="sendCommand('replace_skill_3'); closeModal(false);">替換欄位 3</button>
      <button onclick="sendCommand('discard_new_skill'); closeModal(false);">捨棄新技能</button>
    `;
  }
  overlay.classList.remove("hidden");
}

function handleModals(data) {
  if (data.showFirstStory && !localStorage.getItem("firstStoryShown")) {
    localStorage.setItem("firstStoryShown", "true");
    return openInfo("首日故事", modalText.firstStory, "firstStory");
  }
  if (data.showTutorial && !localStorage.getItem("tutorialShown")) {
    localStorage.setItem("tutorialShown", "true");
    return openInfo("教學", modalText.tutorial, "tutorial");
  }
  // Weather Buff is shown only from the manual Buff Info button.
  if (data.showEnvironmentAdvice) return showExternalEnvironment();
  if (data.showSatietyWarning) return openInfo("飽食度警告", modalText.satietyWarning, "satiety");
}

function renderSkills(skills) {
  currentSkills = skills;
  for (let i = 0; i < 3; i++) {
    const skillName = skills[i];
    const label = skillName || `技能 ${i + 1}`;
    const button = $(`skillButton${i}`);
    const battleButton = $(`battleSkillButton${i}`);

    if (button) {
      button.textContent = skillName || "空技能";
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
    openInfo("空技能欄位", "此技能欄位是空的。擊敗魔王以獲得新技能。", `empty-skill-${index}-${Date.now()}`);
    return;
  }
  sendCommand(`skill_${index + 1}`);
}

function updateHpChart(monsterHp, monsterMaxHp) {
  const hp = toFiniteNumber(monsterHp, NaN);
  const maxHp = toFiniteNumber(monsterMaxHp, NaN);
  if (!Number.isFinite(hp) || !Number.isFinite(maxHp) || maxHp <= 0) {
    setChartStatus("正在等待 ESP32 發送有效的怪物 HP 數據...");
    return;
  }

  setChartFallback(hp, maxHp);
  setChartStatus(`怪物 HP 圖表已更新：${hp}/${maxHp}`);
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
  const enemyName = data.monsterName || (isBoss ? "魔王" : "怪物");
  const enemyHp = toFiniteNumber(data.monsterHp, 0);
  const enemyMaxHp = toFiniteNumber(data.monsterMaxHp, 100);
  const prompt = data.lastEvent || data.event || "SoulBox 正在等待指令。";

  text("battleEnemyName", enemyName);
  text("battleEnemyLevel", isBoss ? "魔王" : "Lv. 1");
  text("battleEnemyHpText", `${enemyHp}/${enemyMaxHp}`);
  text("battlePrompt", prompt);
  setWidth("battleEnemyHpBar", percent(enemyHp, enemyMaxHp));

  const sprite = $("enemySprite");
  if (sprite) {
    sprite.classList.remove("boss", "boss-breakfast", "boss-lunch", "boss-dinner");
    if (isBoss) {
      sprite.classList.add("boss");
      if (enemyName === "早餐魔王") sprite.classList.add("boss-breakfast");
      else if (enemyName === "午餐魔王") sprite.classList.add("boss-lunch");
      else if (enemyName === "晚餐魔王") sprite.classList.add("boss-dinner");
    }
  }
}


function showExternalEnvironment() {
  if (!latestData) {
    openManualInfo("外在環境詳細數據", "尚未收到 ESP32 狀態資料。請確認 WebSocket 已連線，或先按「同步真實天氣」。", `external-env-empty-${Date.now()}`);
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
天氣加成: ${latestData.weatherBuffName || latestData.buff || "-"}

真實外部氣象
狀態: ${hasExternalWeather ? "已同步" : "尚未同步"}
外部溫度: ${formatOptional(latestData.extTemperature, 1, " C")}
風速/風向: ${formatOptional(latestData.windSpeed, 1, " m/s")} ${latestData.windDir || ""}
氣壓: ${formatOptional(latestData.pressure, 0, " hPa")}
能見度: ${formatOptional(latestData.visibility, 1, " km")}
露點: ${formatOptional(latestData.dewPoint, 1, " C")}

健康提醒
${latestData.healthAdvice || latestData.environmentAdvice || "請先按「同步真實天氣」以產生健康提醒。"}`;

  openManualInfo("外在環境詳細數據", body, `external-environment-${Date.now()}`);
}

function showChatModal() {
  const body = "聊天展示：\nSoulBox 可以顯示故事訊息、戰鬥反應和環境提醒。\n此彈窗保留在瀏覽器端，因此 ESP32 只需發送壓縮後的狀態數據。";
  openManualInfo("加入聊天", body, `chat-modal-${Date.now()}`);
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
  if (!Number.isFinite(value) || value <= 0) return "尚未更新";
  const seconds = Math.floor((Date.now() - performance.timeOrigin - value) / 1000);
  if (!Number.isFinite(seconds) || seconds < 0) return "剛剛";
  return `${seconds}秒前`;
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
