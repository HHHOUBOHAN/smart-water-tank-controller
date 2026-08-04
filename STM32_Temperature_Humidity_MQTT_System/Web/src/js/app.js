let client = null;
let demoTimer = null;
let trendChart = null;
let history = [];
let state = {
  temperature: 26.4,
  humidity: 58.0,
  fan_duty: 0,
  mode: "auto",
  alarm: false,
  buzzer_muted: false
};

const $ = (id) => document.getElementById(id);

function getMqttTopics() {
  const deviceId = $("deviceId").value.trim();

  if (!deviceId) {
    throw new Error("设备编号不能为空");
  }

  const topicBase = `temp-fan/${deviceId}`;

  return {
    status: `${topicBase}/status`,
    command: `${topicBase}/command`,
    ack: `${topicBase}/ack`
  };
}


function setFeedback(id, text, type = "idle") {
  const node = $(id);
  if (!node) return;
  node.className = `command-feedback ${type}`;
  const label = node.querySelector("span");
  if (label) label.textContent = text;
}

function addLog(message) {
  const item = $("logItemTemplate").content.cloneNode(true);
  item.querySelector("time").textContent = new Date().toLocaleTimeString("zh-CN", { hour12: false });
  item.querySelector("span").textContent = message;
  const log = $("activityLog");
  log.prepend(item);
  while (log.children.length > 80) log.lastElementChild.remove();
}

function setConnection(text, online) {
  $("connectionText").textContent = text;
  $("connectionDot").className = `status-dot ${online ? "online" : "offline"}`;
  $("wifiValue").innerHTML = online
    ? '<i data-lucide="wifi"></i>已连接'
    : '<i data-lucide="wifi"></i>演示连接';
}

function flashValue(id) {
  const node = $(id);
  node.classList.remove("value-flash");
  void node.offsetWidth;
  node.classList.add("value-flash");
}

function setBadge(id, text, type = "") {
  const badge = $(id);
  badge.textContent = text;
  badge.className = `metric-state ${type}`.trim();
}

function render() {
  const temperatureLimit = Number($("temperatureLimit").value);
  const humidityLimit = Number($("humidityLimit").value);
  const running = state.fan_duty > 0;
  const normal = !state.alarm;

  $("temperatureValue").innerHTML = `${Number(state.temperature).toFixed(1)}<small>℃</small>`;
  $("humidityValue").innerHTML = `${Number(state.humidity).toFixed(1)}<small>%RH</small>`;
  $("fanDutyValue").innerHTML = `${state.fan_duty}<small>%</small>`;
  $("tempTrend").textContent = `上限 ${temperatureLimit.toFixed(1)}℃`;
  $("humidityTrend").textContent = `上限 ${humidityLimit}%RH`;
  $("fanStateValue").textContent = running ? "风扇全速运行" : "风扇已关闭";
  $("modeValue").textContent = state.mode === "auto" ? "自动模式" : "手动模式";
  $("sideModeValue").textContent = $("modeValue").textContent;
  $("sideFanValue").textContent = running ? "全速开启" : "关闭";
  $("buzzerValue").textContent = state.buzzer_muted ? "已静音" : (state.alarm ? "报警中" : "待机");
  $("alarmValue").textContent = normal ? "正常" : "报警";
  $("alarmValue").style.color = normal ? "#86efac" : "#fda4af";
  $("healthValue").innerHTML = normal
    ? '<i data-lucide="circle-check"></i>运行正常'
    : '<i data-lucide="triangle-alert"></i>需要处理';
  $("healthValue").style.color = normal ? "#86efac" : "#fda4af";
  $("updatedAt").textContent = new Date().toLocaleTimeString("zh-CN", { hour12: false });
  $("fanSlider").value = state.fan_duty;
  $("fanSliderOutput").value = state.fan_duty > 0 ? "100%" : "0%";
  $("autoButton").classList.toggle("active", state.mode === "auto");
  $("manualButton").classList.toggle("active", state.mode === "manual");
  $("fanSlider").disabled = state.mode !== "manual";
  $("sendFanButton").disabled = state.mode !== "manual";
  $("fanModeBadge").textContent = state.mode === "auto" ? "自动" : "手动";
  setBadge("tempState", state.temperature >= temperatureLimit ? "异常" : "正常", state.temperature >= temperatureLimit ? "danger" : "");
  setBadge("humidityState", state.humidity >= humidityLimit ? "异常" : "正常", state.humidity >= humidityLimit ? "danger" : "");
  setBadge("alarmBadge", normal ? "安全" : "报警", normal ? "" : "danger");
  document.querySelector(".fan-icon").classList.toggle("spinning", running);

  const alertBox = $("alertBox");
  alertBox.classList.toggle("danger", !normal);
  $("alertTitle").textContent = normal ? "当前无报警" : "温湿度报警已触发";
  $("alertText").textContent = normal ? "温湿度都在安全范围内" : "请检查环境并确认风扇运行状态";
  updateTrendChart();
}

function updateTrendChart() {
  if (!window.Chart) return;
  const labels = history.map((item) => item.time.toLocaleTimeString("zh-CN", { minute: "2-digit", second: "2-digit" }));
  const temperature = history.map((item) => Number(item.temperature.toFixed(1)));
  const humidity = history.map((item) => Number(item.humidity.toFixed(1)));
  if (!trendChart) {
    trendChart = new Chart($("trendChart"), {
      type: "line",
      data: { labels, datasets: [
        { label: "温度 (℃)", data: temperature, borderColor: "#38bdf8", backgroundColor: "#38bdf818", borderWidth: 2.4, pointRadius: 0, pointHoverRadius: 4, tension: .42, fill: true },
        { label: "湿度 (%RH)", data: humidity, borderColor: "#2dd4bf", backgroundColor: "transparent", borderWidth: 2.4, pointRadius: 0, pointHoverRadius: 4, tension: .42, fill: false }
      ] },
      options: {
        responsive: true, maintainAspectRatio: false, animation: { duration: 420 },
        interaction: { intersect: false, mode: "index" },
        plugins: { legend: { display: false }, tooltip: { backgroundColor: "#0b1220", titleColor: "#eaf3fc", bodyColor: "#b6c8d8", padding: 10, cornerRadius: 10, displayColors: true } },
        scales: {
          x: { grid: { display: false }, ticks: { maxTicksLimit: 5, color: "#8096ad", font: { family: "DM Mono", size: 10 } }, border: { display: false } },
          y: { grid: { color: "#33465d" }, ticks: { maxTicksLimit: 5, color: "#8096ad", font: { family: "DM Mono", size: 10 } }, border: { display: false } }
        }
      }
    });
    return;
  }
  trendChart.data.labels = labels;
  trendChart.data.datasets[0].data = temperature;
  trendChart.data.datasets[1].data = humidity;
  trendChart.update();
}

function addHistory() {
  history.push({ time: new Date(), temperature: state.temperature, humidity: state.humidity });
  if (history.length > 36) history.shift();
}

function getCommandFeedbackId(command = {}) {
  return command.target === "threshold" || command.temp_limit !== undefined || command.humidity_limit !== undefined
    ? "thresholdFeedback"
    : "commandFeedback";
}

function publishCommand(command, feedbackId = getCommandFeedbackId(command)) {
  if (!client || !client.connected) {
    setFeedback(feedbackId, "MQTT 未连接，无法发送控制命令", "error");
    addLog("MQTT 未连接，无法发送控制命令");
    return;
  }

  const topics = getMqttTopics();
  const payload = typeof command === "string" ? command : JSON.stringify(command);
  setFeedback(feedbackId, "命令已发送，等待设备应答...", "pending");

  client.publish(
    topics.command,
    payload,
    { qos: 0, retain: false },
    (error) => {
      if (error) {
        setFeedback(feedbackId, `命令发送失败：${error.message}`, "error");
        addLog(`控制命令发送失败：${error.message}`);
        return;
      }

      addLog(`已发送控制命令：${payload}`);
    }
  );
}

function setMode(mode, publish = true) {
  state.mode = mode;
  if (publish) publishCommand(`mode=${mode}`);
  render();
}

function handleStatusMessage(data) {
  const nextState = { ...state };
  const temperature = Number(data.temperature);
  const humidity = Number(data.humidity);
  const fanDutyRaw = data.fanDuty !== undefined ? data.fanDuty : data.fan_duty;
  const fanDuty = Number(fanDutyRaw);

  if (Number.isFinite(temperature)) nextState.temperature = temperature;
  if (Number.isFinite(humidity)) nextState.humidity = humidity;
  if (Number.isFinite(fanDuty)) nextState.fan_duty = fanDuty > 0 ? 100 : 0;
  if (data.mode === "auto" || data.mode === "manual") nextState.mode = data.mode;
  if (data.alarm !== undefined) nextState.alarm = Boolean(data.alarm);
  if (data.buzzer_muted !== undefined) nextState.buzzer_muted = Boolean(data.buzzer_muted);

  state = nextState;
  addHistory();
  flashValue("temperatureValue");
  flashValue("humidityValue");
  render();
}

function handleAckMessage(payloadText) {
  let data = null;
  try {
    data = JSON.parse(payloadText);
  } catch (error) {
    data = null;
  }

  if (data) {
    const feedbackId = data.target === "threshold" || data.type === "threshold" ? "thresholdFeedback" : "commandFeedback";
    const message = data.message || (data.success === true ? "设备已执行" : "设备返回失败");

    if (data.success === true) {
      setFeedback(feedbackId, `设备应答成功：${message}`, "success");
      addLog(`命令执行成功：${message}`);
      return;
    }

    setFeedback(feedbackId, `设备应答失败：${message}`, "error");
    addLog(`命令执行失败：${message}`);
    return;
  }

  const isThresholdAck = payloadText.includes("tempMax") || payloadText.includes("humidityMax");
  const feedbackId = isThresholdAck ? "thresholdFeedback" : "commandFeedback";

  if (payloadText.startsWith("ok=")) {
    setFeedback(feedbackId, `设备应答成功：${payloadText.slice(3)}`, "success");
    addLog(`命令执行成功：${payloadText}`);
    return;
  }

  setFeedback(feedbackId, `设备应答失败：${payloadText}`, "error");
  addLog(`命令执行失败：${payloadText}`);
}

function startDemo() {
  client?.end(true);
  client = null;
  setConnection("演示模式", false);
  clearInterval(demoTimer);
  history = [];
  let temperature = 26.4;
  let humidity = 58.0;
  for (let index = 0; index < 30; index++) {
    temperature += (Math.random() - 0.48) * 0.28;
    humidity += (Math.random() - 0.5) * 0.42;
    history.push({ time: new Date(Date.now() - (30 - index) * 2000), temperature, humidity });
  }
  state.temperature = temperature;
  state.humidity = humidity;
  demoTimer = setInterval(() => {
    state.temperature = Math.min(36, Math.max(20, state.temperature + (Math.random() - 0.48) * 0.28));
    state.humidity = Math.min(92, Math.max(35, state.humidity + (Math.random() - 0.5) * 0.45));
    state.alarm = state.temperature >= Number($("temperatureLimit").value) || state.humidity >= Number($("humidityLimit").value);
    if (state.mode === "auto") state.fan_duty = state.alarm ? 100 : 0;
    addHistory();
    flashValue("temperatureValue");
    flashValue("humidityValue");
    render();
  }, 2000);
  render();
  addLog("已启用平滑演示数据");
}

function connectMqtt() {
  const url = $("brokerUrl").value.trim();
  const deviceId = $("deviceId").value.trim();
  const username = $("brokerUsername").value.trim();
  const password = $("brokerPassword").value;

  if (!url) { addLog("请填写 WebSocket 地址"); return; }
  if (!url.startsWith("ws://") && !url.startsWith("wss://")) { addLog("MQTT 地址必须以 ws:// 或 wss:// 开头"); return; }
  if (!deviceId) { addLog("请填写设备编号"); return; }
  if (!window.mqtt) { addLog("MQTT.js 加载失败，请检查网络"); return; }

  if (demoTimer) {
    clearInterval(demoTimer);
    demoTimer = null;
    addLog("已停止演示数据");
  }
  if (client) {
    client.end(true);
    client = null;
  }

  const topics = getMqttTopics();
  setConnection("正在连接 MQTT", false);

  client = mqtt.connect(url, {
    clientId: `web_${Date.now()}`,
    username: username || undefined,
    password: password || undefined,
    reconnectPeriod: 3000,
    connectTimeout: 10000,
    clean: true
  });

  client.on("connect", () => {
    setConnection("MQTT 已连接", true);
    addLog(`MQTT 已连接：${url}`);

    client.subscribe([topics.status, topics.ack], { qos: 0 }, (error) => {
      if (error) {
        addLog(`订阅失败：${error.message}`);
        return;
      }

      addLog(`已订阅：${topics.status}`);
      addLog(`已订阅：${topics.ack}`);
    });
  });

  client.on("message", (topic, message) => {
    const currentTopics = getMqttTopics();
    const payloadText = message.toString();
    if (topic === currentTopics.status) {
      try {
        handleStatusMessage(JSON.parse(payloadText));
        addLog(`收到设备状态：${topic}`);
      } catch (error) {
        addLog(`状态消息不是有效 JSON：${payloadText}`);
      }
      return;
    }

    if (topic === currentTopics.ack) {
      handleAckMessage(payloadText);
      addLog(`收到执行结果：${topic}`);
    }
  });

  client.on("reconnect", () => {
    setConnection("正在重连 MQTT", false);
    addLog("MQTT 正在重新连接");
  });
  client.on("offline", () => {
    setConnection("MQTT 已离线", false);
    addLog("MQTT 连接已离线");
  });
  client.on("close", () => {
    setConnection("MQTT 已断开", false);
  });
  client.on("error", (error) => {
    setConnection("MQTT 连接失败", false);
    addLog(`MQTT 错误：${error.message}`);
  });
}

function initializeMqttForm() {
  const brokerUrlInput = $("brokerUrl");
  const deviceIdInput = $("deviceId");

  if (brokerUrlInput && !brokerUrlInput.value.trim()) {
    const hostname = window.location.hostname || "127.0.0.1";
    brokerUrlInput.value = `ws://${hostname}:9001`;
  }
  if (deviceIdInput && !deviceIdInput.value.trim()) {
    deviceIdInput.value = "TEMP-FAN-001";
  }
}
function boot() {
  initializeMqttForm();
  installNavigation();
  installNavDragScroll();
  if (window.lucide) window.lucide.createIcons();
  startDemo();
}

function installNavDragScroll() {
  const nav = document.querySelector(".side-nav");
  if (!nav) return;

  let isDown = false;
  let startX = 0;
  let scrollLeft = 0;
  let moved = false;

  nav.addEventListener("pointerdown", (event) => {
    isDown = true;
    moved = false;
    startX = event.clientX;
    scrollLeft = nav.scrollLeft;
    nav.classList.add("dragging");
  });

  nav.addEventListener("pointermove", (event) => {
    if (!isDown) return;
    const deltaX = event.clientX - startX;
    if (Math.abs(deltaX) > 6) moved = true;
    nav.scrollLeft = scrollLeft - deltaX;
  });

  const stopDrag = () => {
    isDown = false;
    nav.classList.remove("dragging");
  };

  nav.addEventListener("pointerup", stopDrag);
  nav.addEventListener("pointercancel", stopDrag);
  nav.addEventListener("pointerleave", stopDrag);
  nav.addEventListener("click", (event) => {
    if (!moved) return;
    event.preventDefault();
    event.stopPropagation();
    moved = false;
  }, true);
}
function installNavigation() {
  const navItems = [...document.querySelectorAll(".nav-item")];
  const sections = navItems
    .map((item) => document.querySelector(item.getAttribute("href")))
    .filter(Boolean);
  let lockedActiveId = "";
  let unlockTimer = null;

  const getHeaderGap = () => {
    const sidebar = document.querySelector(".sidebar");
    return window.matchMedia("(max-width: 760px)").matches ? sidebar.offsetHeight + 12 : 18;
  };
  const getSectionTop = (section) => window.scrollY + section.getBoundingClientRect().top;
  const setActive = (id) => {
    navItems.forEach((item) => item.classList.toggle("active", item.getAttribute("href") === `#${id}`));
  };
  const scrollToSection = (section) => {
    const top = Math.max(0, getSectionTop(section) - getHeaderGap());
    window.scrollTo({ top, behavior: "smooth" });
  };
  const lockActive = (id) => {
    lockedActiveId = id;
    clearTimeout(unlockTimer);
    unlockTimer = setTimeout(() => { lockedActiveId = ""; }, 700);
  };

  navItems.forEach((item) => item.addEventListener("click", (event) => {
    event.preventDefault();
    const id = item.getAttribute("href").slice(1);
    const section = document.getElementById(id);
    if (!section) return;
    setActive(id);
    lockActive(id);
    scrollToSection(section);
    window.history.replaceState(null, "", `#${id}`);
  }));

  let scrolling = false;
  window.addEventListener("scroll", () => {
    if (lockedActiveId || scrolling) return;
    scrolling = true;
    window.requestAnimationFrame(() => {
      const marker = window.scrollY + getHeaderGap() + 24;
      let activeSection = sections[0];
      sections.forEach((section) => {
        if (getSectionTop(section) <= marker) activeSection = section;
      });
      setActive(activeSection.id);
      scrolling = false;
    });
  }, { passive: true });

  const initialId = window.location.hash.slice(1);
  if (sections.some((section) => section.id === initialId)) {
    setActive(initialId);
    window.setTimeout(() => scrollToSection(document.getElementById(initialId)), 30);
  }
}
$("autoButton").onclick = () => setMode("auto");
$("manualButton").onclick = () => setMode("manual");
$("fanSlider").oninput = (event) => {
  $("fanSliderOutput").value = Number(event.target.value) > 0 ? "100%" : "0%";
};
$("sendFanButton").onclick = () => {
  const fanDuty = Number($("fanSlider").value);
  publishCommand(fanDuty > 0 ? "fan=on" : "fan=off");
};
$("muteButton").onclick = () => {
  state.buzzer_muted = !state.buzzer_muted;
  publishCommand(state.buzzer_muted ? "mute=1" : "mute=0");
};
$("saveThresholdButton").onclick = () => {
  const tempLimit = Number($("temperatureLimit").value);
  const humidityLimit = Number($("humidityLimit").value);

  if (!Number.isFinite(tempLimit) || tempLimit < 0 || tempLimit > 80) {
    addLog("温度上限必须在 0 ~ 80℃ 之间");
    return;
  }
  if (!Number.isFinite(humidityLimit) || humidityLimit < 0 || humidityLimit > 100) {
    addLog("湿度上限必须在 0 ~ 100%RH 之间");
    return;
  }

  state.alarm = state.temperature >= tempLimit || state.humidity >= humidityLimit;
  if (state.mode === "auto") state.fan_duty = state.alarm ? 100 : 0;
  publishCommand(`tempMax=${tempLimit.toFixed(1)}`, "thresholdFeedback");
  publishCommand(`humidityMax=${Math.round(humidityLimit)}`, "thresholdFeedback");
  render();
};
$("connectButton").onclick = connectMqtt;
$("demoButton").onclick = startDemo;
$("clearLogButton").onclick = () => { $("activityLog").innerHTML = ""; };
let resizeTimer;
window.addEventListener("resize", () => {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(() => trendChart?.resize(), 150);
});
window.addEventListener("beforeunload", () => {
  clearInterval(demoTimer);
  client?.end(true);
});

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", boot);
} else {
  boot();
}





















