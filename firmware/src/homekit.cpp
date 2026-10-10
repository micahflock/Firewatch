#include "homekit.h"

#include <HomeSpan.h>

#include <atomic>

#include "config.h"
#include "home_sync.h"

namespace homekit {
namespace {

Hooks hooks;
QueueHandle_t lighterQueue;
std::atomic<bool> isPaired{false};

fw::HomeState actual() { return {hooks.fireOn(), hooks.brightness()}; }

// fw::HomeSync decides when a Home write is a scene command and when the scene's real state
// goes back to Home (see home_sync.h); this is just the HomeSpan glue.
struct Fireplace : Service::LightBulb {
  SpanCharacteristic* power;
  SpanCharacteristic* level;
  fw::HomeSync sync;
  uint32_t lastPollMs = 0;

  Fireplace() {
    const fw::HomeState now = actual();
    power = new Characteristic::On(now.on);
    level = new Characteristic::Brightness(now.brightness);
    sync.seed(now);
  }

  // A write from Home (tile, Siri, scene, automation).
  boolean update() override {
    const fw::HomeState wanted{power->getNewVal() != 0, level->getNewVal()};
    const fw::HomeState current = actual();
    if (level->updated()) hooks.setBrightness(wanted.brightness);
    // Only an actual On/Off write can start or stop the scene; a brightness-only write
    // carries the tile's current on/off, which may be a few ms stale.
    if (sync.homeWrote(wanted, current, millis()) && power->updated()) hooks.setFire(wanted.on);
    return true;
  }

  // Push changes made elsewhere (remote, web page, button, lighter, TV auto-off) to Home.
  void loop() override {
    const uint32_t now = millis();
    if (now - lastPollMs < 250) return;
    lastPollMs = now;
    fw::HomeState out;
    if (!sync.poll(actual(), now, out)) return;
    if ((power->getVal() != 0) != out.on) power->setVal(out.on);
    if (level->getVal() != out.brightness) level->setVal(out.brightness);
  }
};

struct LighterButton : Service::StatelessProgrammableSwitch {
  SpanCharacteristic* event;

  LighterButton(uint8_t index, const char* name) {
    event = new Characteristic::ProgrammableSwitchEvent();
    event->setValidValues(1, SpanButton::SINGLE);  // the Home app then offers "Single Press" only
    new Characteristic::ServiceLabelIndex(index);
    new Characteristic::Name(name);
  }
};

LighterButton* strikeButton = nullptr;
LighterButton* snuffButton = nullptr;

// Owns the lighter's battery, and drains the queue of verified frames on HomeSpan's task.
struct LighterBattery : Service::BatteryService {
  SpanCharacteristic *level, *charging, *low;

  LighterBattery() {
    level = new Characteristic::BatteryLevel(100, true);  // true: remembered across reboots
    charging = new Characteristic::ChargingState(0);
    low = new Characteristic::StatusLowBattery(0, true);
  }

  void loop() override {
    skeumo::link::Message m;
    while (xQueueReceive(lighterQueue, &m, 0) == pdTRUE) {
      if (m.channel == LIGHTER_STRIKE_CHANNEL) strikeButton->event->setVal(SpanButton::SINGLE);
      if (m.channel == LIGHTER_SNUFF_CHANNEL) snuffButton->event->setVal(SpanButton::SINGLE);
      if (m.batteryPct <= 100) {
        if (level->getVal() != m.batteryPct) level->setVal(m.batteryPct);
        const int isLow = m.batteryPct <= 15 ? 1 : 0;
        if (low->getVal() != isLow) low->setVal(isLow);
      }
    }
  }
};

void identity(const char* name, const char* model) {
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  new Characteristic::Name(name);
  new Characteristic::Manufacturer("Skeumo");
  new Characteristic::Model(model);
}

}  // namespace

void begin(const Hooks& h, const char* ssid, const char* password, const char* setupCode) {
  hooks = h;
  lighterQueue = xQueueCreate(8, sizeof(skeumo::link::Message));

  homeSpan.setLogLevel(0);
  homeSpan.setPortNum(HOMEKIT_PORT);  // the web page keeps port 80
  homeSpan.setHostNameSuffix("");     // advertise as HOSTNAME.local, same as the web page
  homeSpan.setWifiCredentials(ssid, password);  // HomeSpan joins, and rejoins after a drop
  homeSpan.setWifiCallback([] { hooks.onWifiConnected(); });
  // pairCallback covers pairing and unpairing; statusCallback also reports it at boot.
  homeSpan.setPairCallback([](boolean p) { isPaired = p; });
  homeSpan.setStatusCallback([](HS_STATUS s) {
    if (s == HS_PAIRED) isPaired = true;
    if (s == HS_PAIRING_NEEDED) isPaired = false;
  });
  if (setupCode && *setupCode) homeSpan.setPairingCode(setupCode);
  homeSpan.begin(Category::Bridges, "Firewatch", HOSTNAME, "Firewatch");

  new SpanAccessory();
  identity("Firewatch", "Firewatch bridge");

  new SpanAccessory();
  identity("Fireplace", "Firewatch fireplace");
  new Fireplace();

#if LIGHTER_ENABLED
  new SpanAccessory();
  identity("Lighter", "Skeumo fireplace lighter");
  new Service::ServiceLabel();
  new Characteristic::ServiceLabelNamespace(1);  // buttons are numbered 1, 2
  strikeButton = new LighterButton(LIGHTER_STRIKE_CHANNEL, "Strike");
  snuffButton = new LighterButton(LIGHTER_SNUFF_CHANNEL, "Snuff");
  new LighterBattery();
#endif

  homeSpan.autoPoll(8192, 1, 0);  // core 0, next to the net and web tasks
}

bool paired() { return isPaired; }

void lighterEvent(const skeumo::link::Message& m) {
  if (lighterQueue) xQueueSend(lighterQueue, &m, 0);
}

}  // namespace homekit
