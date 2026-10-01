#include "homekit.h"

#include <HomeSpan.h>

#include "config.h"

namespace homekit {
namespace {

Hooks hooks;
QueueHandle_t lighterQueue;

// The scene takes a moment to reflect a command (the net task picks it up from its queue),
// so after a write from Apple Home, don't mirror the scene back until it has caught up.
constexpr uint32_t kMirrorHoldMs = 2000;

struct Fireplace : Service::Switch {
  SpanCharacteristic* on;
  uint32_t holdUntil = 0;

  Fireplace() { on = new Characteristic::On(hooks.fireOn()); }

  boolean update() override {
    hooks.setFire(on->getNewVal());
    holdUntil = millis() + kMirrorHoldMs;
    return true;
  }

  void loop() override {
    if (static_cast<int32_t>(millis() - holdUntil) < 0) return;
    const int state = hooks.fireOn() ? 1 : 0;
    if (on->getVal() != state) on->setVal(state);
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
  // Firewatch joins Wi-Fi itself; HomeSpan needs the credentials too, to start its server
  // and to rejoin after a drop.
  homeSpan.setWifiCredentials(ssid, password);
  homeSpan.begin(Category::Bridges, "Firewatch", HOSTNAME, "Firewatch");
  if (setupCode && *setupCode) homeSpan.setPairingCode(setupCode);

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

void lighterEvent(const skeumo::link::Message& m) {
  if (lighterQueue) xQueueSend(lighterQueue, &m, 0);
}

}  // namespace homekit
