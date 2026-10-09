# Native HomeKit for Firewatch — design

Date: 2026-10-09. Status: approved in conversation, pending spec review.

## Goal

Firewatch appears in Apple Home as a native accessory, with no bridge. Turning it on or off
from Home (tile, Siri, scenes, automations) does exactly what `/on` and `/off` do today, and
Home shows the real state no matter what started or stopped the scene.

Done means:

- The accessory pairs with a setup code from `secrets.h`.
- Home and Siri turn the scene on and off and set brightness.
- When the scene changes some other way (Roku remote, web page, button, the TV's own
  auto-off), Home reflects it within about 2 s.
- Everything that works today keeps working: the web page and endpoints, the button,
  following the TV's power, and Wi-Fi updates (`pio run -e esp32-ota -t upload`).
- The fire animation's frame rate is unaffected.

## Decisions

| Topic | Decision | Why |
|---|---|---|
| Library | HomeSpan **1.9.1**, pinned | The last release for Arduino-ESP32 core 2.x (its `version.h` rejects any other major version). The project's `espressif32@6.9.0` is core 2.0.17. HomeSpan 2.x needs core 3.x via the community pioarduino platform, where IRremoteESP8266 2.8.6 has known build problems. |
| Accessory | One `Service::LightBulb` called **"Fireplace"**, with `On` and `Brightness`; category Lighting | The user chose a dimmable light, and accepts that Siri's "all lights" commands include it, so the TV goes off or on along with it. |
| Semantics | Home on = `Command::On`; Home off = `Command::Off`, which turns the TV off too | Identical to `/on` and `/off`. There's one source of truth: `fw::Scene`. |
| Brightness | Shares a single setter with the web slider. Persisted in `Preferences` (`"bright"`). | One setting, whichever way it's changed. |
| HomeKit port | `HOMEKIT_PORT 51826` in `config.h` | HomeSpan's default is 80, which the web page already uses. |
| Setup code | `HOMEKIT_SETUP_CODE` in `secrets.h` (8 digits, not trivial like `12345678`) | A per-install secret, so it's kept out of git. |
| Flash layout | `board_build.partitions = min_spiffs.csv` | Two 1.9 MB program slots instead of 1.25 MB, so Wi-Fi updates still work. The NVS partition stays at 0x9000, so saved settings survive. Needs one USB flash. |

## Architecture

```
Home app / Siri ──HAP:51826──► HomeSpan (autoPoll task, core 0)
                                 │ FireLight::update()  ── On/Off ──► commandQueue ──► netTask ─► fw::Scene
                                 │                       ── brightness ──► setBrightness() (atomic + prefs)
                                 │ FireLight::loop()  ◄── fw::HomeSync ◄── lightsOn / brightnessPct atomics
web task (port 80), button ──────┴─── unchanged paths into commandQueue / setBrightness()
render loop (core 1) ── unchanged
```

### New units

**`fw::HomeSync`** (`lib/firecore/src/home_sync.h/.cpp`). Pure logic, unit-tested on the
host. It decides what to report to HomeKit.

- `void homeWrote(uint32_t nowMs)`: Home just changed on/off or brightness.
- `Report poll(bool actualOn, int actualBrightness, uint32_t nowMs)`: returns which values
  differ from what HomeKit last saw, and their new values. Returns nothing during the hold
  window: 5 s after `homeWrote`, ended early once the device's state matches what Home
  asked for.
- `void reported(...)` / initial `seed(on, brightness)`: tracks the value HomeKit last saw.
- Waking counts as on: it reads `lightsOn`, which is true for `Waking` and `On`.

**`homekit` module** (`src/homekit.h/.cpp`). The glue between HomeSpan and the firmware.

- `homekit::begin(const Hooks&)`, where `Hooks` provides `sendCommand(fw::Command)`,
  `setBrightness(int)`, `bool lightsOn()`, `int brightness()`, and `onWifiConnected()`.
- It configures HomeSpan:
  - `setWifiCredentials(WIFI_SSID, WIFI_PASSWORD)`
  - `setPortNum(HOMEKIT_PORT)`
  - `setPairingCode(HOMEKIT_SETUP_CODE)`
  - `setHostNameSuffix("")`
  - `begin(Category::Lighting, "Fireplace", HOSTNAME, "Firewatch")`
  - `setWifiCallback(...)`
  - `setPairCallback(...)`, which sets an atomic `paired` flag
  - `autoPoll()`, on core 0
- No control pin and no status LED, so GPIO0 stays the scene button. HomeSpan's serial CLI
  stays enabled, for unpairing or resetting.
- `FireLight : Service::LightBulb`:
  - `update()` maps writes to the hooks and calls `HomeSync::homeWrote`.
  - `loop()`, every ~250 ms, calls `HomeSync::poll` and `setVal()` on whatever changed.
- `bool homekit::paired()`.

### Changes to existing code

- `main.cpp`:
  - Remove `WiFi.begin` and the blocking 15 s wait. HomeSpan owns connecting and
    reconnecting. `WiFi.mode`, `setHostname` and `setSleep(false)` stay.
  - Remove `MDNS.begin`. HomeSpan calls `MDNS.begin(HOSTNAME)` once Wi-Fi is up.
  - New `onWifiConnected()`, called from HomeSpan's Wi-Fi callback:
    - logs `[wifi] connected <ip>`
    - `MDNS.addService("http","tcp",80)`
    - `ArduinoOTA.setMdnsEnabled(false)`, then `begin()`
    - `MDNS.enableArduino(3232, true)`
  - `loop()` calls `ArduinoOTA.handle()` only after Wi-Fi has connected.
  - Pull out `setBrightness(int)`, used by the web handler and by HomeKit.
  - `/status` adds `"homekit":"paired"|"unpaired"`.
- `config.h`: add `HOMEKIT_PORT`.
- `secrets.example.h`: add `HOMEKIT_SETUP_CODE`. Claude generates a random valid code into the
  user's (git-ignored) `secrets.h` and tells them what it is.
- `platformio.ini`:
  - `homespan/HomeSpan@1.9.1` in `lib_deps`
  - `board_build.partitions = min_spiffs.csv`
  - nothing for the native env: `home_sync` lives in the `firecore` lib, which the native
    tests already build

## Error handling

- **HomeKit write while the TV is unreachable:** the command goes through the normal scene
  path. The lights come on anyway (the existing wake timeout policy), so Home stays on.
- **Wi-Fi loss:** HomeSpan reconnects with backoff. The scene keeps running on its own, and
  HomeKit catches up on reconnect, because `loop()` pushes the current state.
- **Command queue full** (8 entries): `xQueueSend` with 0 wait drops the command, the same as
  for the web page. `HomeSync` then reports the real state after the hold window, so Home
  doesn't lie.
- **Unpaired, or pairing lost:** `/status` shows `unpaired`. Re-add from Home with the
  setup code, or use the serial CLI to reset.

## Testing

- **Host tests (`pio test -e native`), new `test_home_sync`:**
  - no report while values match
  - a report when the remote turns the scene off
  - no report inside the hold window
  - the hold ends early once the state matches
  - the hold expires after 5 s and reports the real state
  - brightness-only changes
  - seeding

  Requires a host compiler: w64devkit, installed at `C:\Dev\tools\w64devkit` and put on PATH.
- **On the device:**
  - build and record the flash and RAM use
  - flash over USB
  - pair from the Home app
  - Home on and off, with the TV following
  - remote on and off, with Home following within about 2 s
  - brightness in both directions
  - `/status` shows `homekit` as paired
  - `pio run -e esp32-ota -t upload` still works
  - check free heap, which should stay above HomeSpan's 80 KB warning
- **Docs:** add a pairing step to `install.md`, plus troubleshooting rows. Mention HomeKit in
  the README.

## Out of scope

Matter, QR-code pairing, a Television or Switch accessory, a HomeKit relaunch-video control,
and pairing status on the web page itself (it's in `/status` only).
