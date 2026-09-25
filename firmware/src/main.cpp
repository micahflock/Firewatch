// Firewatch: fire-glow LEDs around a Roku TV, started and stopped together with the TV.
//
// Core 1 (Arduino loop): fire animation -> dithering -> LEDs at TARGET_FPS, plus button/OTA.
// Core 0: "net" task polls the TV and runs the Scene state machine; "web" task serves HTTP.
// The only state crossing cores is a command queue, two atomics and a mutex-guarded status.

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_random.h>
#include <esp_timer.h>

#include <atomic>
#include <mutex>

#include "config.h"
#include "fire.h"
#include "ir.h"
#include "leds.h"
#include "output.h"
#include "roku_client.h"
#include "scene.h"
#include "web_page.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "include/secrets.h missing - copy secrets.example.h and fill in your Wi-Fi details"
#include "secrets.example.h"
#endif

namespace {

QueueHandle_t commandQueue;
std::atomic<bool> lightsOn{false};
std::atomic<int> brightnessPct{DEFAULT_BRIGHTNESS};
std::atomic<float> currentMa{0};

struct Status {
  const char* state = "off";
  const char* tv = "unknown";
  String activeApp, host, model, event, error;
};
std::mutex statusMutex;
Status status;

RokuClient roku;
WebServer server(80);
Preferences prefs;

const char* stateName(fw::Scene::State s) {
  switch (s) {
    case fw::Scene::State::Off: return "off";
    case fw::Scene::State::Waking: return "waking TV";
    case fw::Scene::State::On: return "on";
  }
  return "?";
}

const char* tvName(fw::TvPower p) {
  switch (p) {
    case fw::TvPower::On: return "on";
    case fw::TvPower::Standby: return "standby";
    case fw::TvPower::Unreachable: return "unreachable";
  }
  return "?";
}

void execute(const fw::Actions& a) {
  if (a.powerOff) roku.powerOff();
  if (a.wake) roku.wake();
  if (a.launch) roku.launchFire();
}

// ---- network task: TV polling + scene policy ---------------------------------------
void netTask(void*) {
  fw::SceneTiming timing;
  timing.followTv = FOLLOW_TV;
  fw::Scene scene(timing);
  roku.begin(ROKU_HOST);
  uint32_t lastPoll = 0;
  fw::TvPower lastPower = fw::TvPower::Unreachable;

  for (;;) {
    fw::Command c;
    uint32_t interval = scene.state() == fw::Scene::State::Waking ? 1000 : POLL_INTERVAL_MS;
    uint32_t elapsed = millis() - lastPoll;
    TickType_t wait = pdMS_TO_TICKS(elapsed >= interval ? 0 : interval - elapsed);
    if (xQueueReceive(commandQueue, &c, wait) == pdTRUE) {
      execute(scene.command(c, millis()));
      lightsOn = scene.lightsOn();
    }
    if (WiFi.status() == WL_CONNECTED && millis() - lastPoll >= interval) {
      lastPoll = millis();
      fw::TvStatus s = roku.poll();
      lastPower = s.power;
      execute(scene.observe(s, millis()));
      lightsOn = scene.lightsOn();
    }

    std::lock_guard<std::mutex> lock(statusMutex);
    status.state = stateName(scene.state());
    status.tv = tvName(lastPower);
    status.activeApp = roku.activeApp();
    status.host = roku.host();
    status.model = roku.model();
    status.event = scene.lastEvent();
    status.error = roku.lastError();
  }
}

// ---- web task ------------------------------------------------------------------------
void sendCommand(fw::Command c) {
  xQueueSend(commandQueue, &c, 0);
  server.send(200, "application/json", "{\"ok\":true}");
}

String jsonEscape(const String& s) {
  String o;
  for (char ch : s) {
    if (ch == '"' || ch == '\\') o += '\\';
    if (static_cast<uint8_t>(ch) >= 0x20) o += ch;
  }
  return o;
}

void handleStatus() {
  String j = "{";
  {
    std::lock_guard<std::mutex> lock(statusMutex);
    j += "\"scene\":\"" + String(status.state) + "\",";
    j += "\"tv\":\"" + String(status.tv) + "\",";
    j += "\"activeApp\":\"" + jsonEscape(status.activeApp) + "\",";
    j += "\"rokuHost\":\"" + jsonEscape(status.host) + "\",";
    j += "\"rokuModel\":\"" + jsonEscape(status.model) + "\",";
    j += "\"lastEvent\":\"" + jsonEscape(status.event) + "\",";
    j += "\"lastError\":\"" + jsonEscape(status.error) + "\",";
  }
  j += "\"brightness\":" + String(brightnessPct.load()) + ",";
  j += "\"ledCurrentMa\":" + String(static_cast<int>(currentMa.load())) + ",";
  j += "\"videoConfigured\":" + String(sizeof(FIRE_CONTENT_ID) > 1 ? "true" : "false") + ",";
  j += "\"uptimeS\":" + String(millis() / 1000) + ",";
  j += "\"rssi\":" + String(WiFi.RSSI()) + "}";
  server.send(200, "application/json", j);
}

void webTask(void*) {
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", kIndexHtml); });
  server.on("/status", HTTP_GET, handleStatus);
  // GET is accepted too so a phone shortcut, bookmark or smart-home webhook can trigger it.
  server.on("/on", [] { sendCommand(fw::Command::On); });
  server.on("/off", [] { sendCommand(fw::Command::Off); });
  server.on("/toggle", [] { sendCommand(fw::Command::Toggle); });
  server.on("/relaunch", [] { sendCommand(fw::Command::Relaunch); });
  server.on("/brightness", [] {
    if (server.hasArg("value")) {
      int v = constrain(server.arg("value").toInt(), 0, 100);
      brightnessPct = v;
      prefs.putUChar("bright", v);
    }
    server.send(200, "application/json", "{\"brightness\":" + String(brightnessPct.load()) + "}");
  });
  server.begin();
  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ---- setup / render loop (core 1) ---------------------------------------------------
std::vector<fw::LedPos> layout = fw::buildLayout(kLedLayout, sizeof(kLedLayout) / sizeof(kLedLayout[0]));
fw::FireEngine* fire;
fw::OutputStage* output;
fw::Fader fader(FADE_IN_S, FADE_OUT_S);
std::vector<fw::Rgbw> frame;
std::vector<uint8_t> wire;
int64_t lastFrameUs = 0;
bool stripDark = false;

void pollButton() {
#if BUTTON_PIN >= 0
  static bool stable = true, lastRead = true;
  static uint32_t changedAt = 0;
  bool r = digitalRead(BUTTON_PIN);
  if (r != lastRead) {
    lastRead = r;
    changedAt = millis();
  } else if (r != stable && millis() - changedAt > 30) {
    stable = r;
    if (!stable) {
      fw::Command c = fw::Command::Toggle;
      xQueueSend(commandQueue, &c, 0);
    }
  }
#endif
}

}  // namespace

void setup() {
  Serial.begin(115200);
  prefs.begin("firewatch", false);
  brightnessPct = prefs.getUChar("bright", DEFAULT_BRIGHTNESS);
#if BUTTON_PIN >= 0
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif
  ir::begin();

  fire = new fw::FireEngine(layout, esp_random());
  fw::OutputConfig oc;
  oc.rgbw = LED_RGBW;
  oc.maPerChannel = MA_PER_CHANNEL;
  oc.maBudget = POWER_BUDGET_MA;
  output = new fw::OutputStage(layout.size(), oc);
  frame.resize(layout.size());
  wire.resize(layout.size() * output->channels());
  leds::begin(layout.size());

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setSleep(false);  // modem sleep adds latency and drops SSDP replies
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(100);
  Serial.printf("[wifi] %s %s\n", WiFi.status() == WL_CONNECTED ? "connected" : "not yet connected",
                WiFi.localIP().toString().c_str());

  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();

  commandQueue = xQueueCreate(8, sizeof(fw::Command));
  xTaskCreatePinnedToCore(netTask, "net", 8192, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(webTask, "web", 6144, nullptr, 1, nullptr, 0);
  lastFrameUs = esp_timer_get_time();
}

void loop() {
  ArduinoOTA.handle();
  pollButton();

  const int64_t now = esp_timer_get_time();
  const int64_t frameUs = 1000000 / TARGET_FPS;
  if (now - lastFrameUs < frameUs || !leds::ready()) {
    delay(1);
    return;
  }
  float dt = (now - lastFrameUs) / 1e6f;
  lastFrameUs = now;

  fader.setTarget(lightsOn.load());
  float env = fader.step(dt);
  if (fader.idle()) {
    if (!stripDark) {
      std::fill(wire.begin(), wire.end(), 0);
      leds::show(wire.data());
      stripDark = true;
      currentMa = 0;
    }
    return;
  }
  stripDark = false;
  fire->render(now / 1e6, frame.data());
  output->process(frame.data(), env * brightnessPct.load() / 100.0f, wire.data());
  currentMa = output->currentMa();
  leds::show(wire.data());
}
