# Setup and install

## 1. TV settings (5 minutes)

On the TCL 32S335:

1. **Settings → System → Power → Fast TV start → On.** Required for waking the TV from Firewatch. Following the TV's power works without it.
2. **Settings → System → Advanced system settings → Control by mobile apps → Network access → Default** (or Enabled/Permissive). Not *Limited* or *Disabled*.
3. **Settings → System → Power → Power On → Home.**
4. Optional: **Settings → System → Power → Auto power savings.** Turn off "turn off after 4 hours" if you want to run the fire longer without touching the remote. Leave it on if you like it as a sleep timer; the glow follows the TV either way.
5. In your router, give the TV a **DHCP reservation**, so its IP address never changes.
6. **The ESP32 needs a 2.4 GHz Wi-Fi network on the same LAN as the TV.** It has no 5 GHz radio. A router that broadcasts one network name on both bands is fine: the ESP32 joins on 2.4 GHz automatically. If your network is 5 GHz-only, enable a 2.4 GHz network on the same router and subnet, and put that name in `secrets.h`.

## 2. Prove the network path before building anything

From a computer on the same network (replace `TV` with the TV's IP address, `192.168.1.176`). These commands work in a macOS/Linux terminal and in Windows Command Prompt. In PowerShell, type `curl.exe` instead of `curl`. On Windows, replace `| grep -E "a|b|c"` with `| findstr "a b c"`. Commands that send something (`-d ""`) print nothing when they succeed.

```sh
# Is ECP reachable, and is the screen on?
curl -s http://TV:8060/query/device-info | grep -E "model-name|power-mode|wifi-mac"

# Put the TV in standby with the remote, wait 10 s, then:
curl -d "" http://TV:8060/keypress/PowerOn        # the screen should come on

# Launch your fire video (youtu.be/CF00JqnCcsU):
curl -d "" "http://TV:8060/launch/837?contentId=CF00JqnCcsU&mediaType=movie"

curl -s http://TV:8060/query/active-app           # expect <app id="837" ...>YouTube</app>
curl -d "" http://TV:8060/keypress/PowerOff
```

- If `keypress` returns **403** or "limited mode", recheck step 1.2.
- If `PowerOn` fails from standby, fast TV start isn't holding the network up. Use the [IR fallback](hardware.md#shopping-list-from-scratch).

While you're there, do the smart-plug [power-restore test](feasibility.md#1-smart-plug-only-the-no-code-option).

## 3. Bench test with WLED (no code)

The Dig-Uno ships with WLED installed, which is handy for proving the wiring before you flash anything.

1. Land the barrel pigtail's leads in the Dig-Uno's power input (+ found with a multimeter), then plug in the supply. Wire the strip's mating JST plug (cut from the reel's far end) to the LED output (5V / LED1 / GND) and plug the strip in, per the [wiring diagram](hardware.md#wiring). Lay the strip loose on a table.
2. Power up, join the `WLED-AP` Wi-Fi network from your phone (password `wled1234`), and give it your home Wi-Fi.
3. In WLED → Config → LED Preferences, set the LED type to **SK6812 RGBW**, the count to **87**, and data GPIO **16**. Then pick the "Fire 2012" effect.
4. If the whole strip lights with sensible colors, the wiring is right. WLED can be reinstalled any time from [install.wled.me](https://install.wled.me) if you ever want it back.

## 4. Firmware

Requires [PlatformIO](https://platformio.org/install/cli) (`pip install platformio`).

```sh
cd firmware
cp include/secrets.example.h include/secrets.h   # Wi-Fi SSID/password, OTA password
$EDITOR include/config.h                          # ROKU_HOST, LED layout
pio test -e native                                # host unit tests
pio run -e esp32 -t upload                        # first flash over the Dig-Uno's USB-C
pio device monitor                                # logs: [wifi] ... [roku] ...
```

For the first USB flash, **unplug the 5 V supply** (USB powers the board by itself). Later updates go over Wi-Fi with `pio run -e esp32-ota -t upload`, so the board can stay in its enclosure.

**The OTA password** (`OTA_PASSWORD` in `secrets.h`) is a password you make up. It's written into the board at the first USB flash, and every later Wi-Fi update must supply the same one, so nobody else on your network can reflash the board. `pio run -e esp32-ota -t upload` reads it from `secrets.h` by itself (`firmware/ota_auth.py`), so it never goes into a file that's committed.

**Updating from a build without Apple Home:** flash once over USB (`pio run -e esp32 -t upload`), because the partition table changed to `min_spiffs` to make room. Your brightness setting is kept; later updates can go over Wi-Fi again.

What to set in [`config.h`](../firmware/include/config.h):

| Setting | What |
|---|---|
| `FIRE_CONTENT_ID` | The YouTube fireplace video ID. It's already set to yours (`CF00JqnCcsU`). |
| `ROKU_HOST` | The TV's reserved IP address (set to `192.168.1.176`). If it's empty, the TV is found by SSDP discovery. |
| `ROKU_WIFI_MAC` | The TV's Wi-Fi MAC, for Wake-on-LAN before the firmware has read it from the TV (set to `D4:AB:CD:A8:96:96`) |
| `kLedLayout` | The strip's runs in data order, in **front-view** coordinates. The default is 23/41/23 LEDs: up the left side from the bottom, across the top, and down the right side. There's no bottom edge. |
| `LED_RGBW` | 1 for SK6812 RGBW, 0 for WS2812B/WS2815 RGB strips |
| `POWER_BUDGET_MA` | Supply current cap (2500 mA; right for a 3 A or 4 A supply) |
| `DEFAULT_BRIGHTNESS` | Start value. After that, the web page's slider sets it, and it's saved across reboots. |
| `FOLLOW_TV` | 1 = TV power drives the glow (recommended) |
| `IR_LED_PIN` | The GPIO for the optional IR LED: 15 (the Dig-Uno's Q1), or −1 for none |
| `LED_PIN`, `BUTTON_PIN` | Already match the Dig-Uno (LED1 = GPIO16, button = GPIO0) |
| `HOMEKIT_PORT` | Apple Home's port (1201). The web page stays on 80. |
| `LIGHTER_ENABLED` | 1 = listen for the Skeumo fireplace lighter over Bluetooth; 0 = Bluetooth off |
| `LIGHTER_DIRECT` | 1 = a strike/snuff turns the fire on/off directly; 0 = only relay the buttons to Apple Home |

And in `secrets.h`: `HOMEKIT_SETUP_CODE` (optional, 8 digits; the build refuses codes Apple rejects, like `12345678`) and `SKEUMO_LINK_KEY` (the lighter's shared key, 32 hex digits; leave the placeholder if you have no lighter).

To tune the flame's character, edit `FireParams` in [`fire.h`](../firmware/lib/firecore/src/fire.h): flicker bands, rise speed, flare rate, and so on. [`tools/preview`](../tools/preview) renders the result to a GIF on your computer without any hardware.

## 5. Build and install

1. Print `firewatch_base.stl`, `firewatch_lid.stl`, and 14 each of `strip_clip_base_10mm.stl` and `strip_clip_cap_10mm.stl` (see the [print notes](hardware.md#mounting)).
2. On the bench, cut the strip into its three runs (23/41/23 LEDs at the cut marks). Keep the JST plug on the start run. Solder the two corner bridges, extend the mating JST plug's pigtail with 22 AWG to reach the enclosure, heat-shrink each joint, and cover the far end's pads (see the [wiring notes](hardware.md#wiring)).
3. Find + on the barrel pigtail with a multimeter and land its leads in the Dig-Uno's power input. Check that the LED1 jumper is on **GPIO16**; the firmware refuses to build for GPIO1.
4. Screw the base onto the TV's VESA holes (M4 × 12), with the LED-end notch facing the bottom-left corner (front view), where the strip starts.
5. Fix the Dig-Uno onto the standoffs with four M2 × 8 screws, with its power terminal and USB-C toward the window. Land the mating JST plug's leads in the LED output terminal (5V / LED1 / GND), and plug in the strip. Cable-tie the pigtail's jack and the strip leads to the anchors. From then on, the supply unplugs at the jack.
6. Stick on the clip bases. Lay the strip on the seats, then press a cap over each seat in a gap between two LEDs until both hooks click under the seat. To take a cap off, spread one leg with a fingernail or small screwdriver. Hold the wire runs down with scraps of VHB.
7. Open `http://firewatch.local` and press **Light the fire**. Check that the steady part of the glow is low on the sides and the lively flicker is across the top. If it's the other way round, the layout direction is reversed. Then fasten the lid with four M2 × 8 screws.

## 6. Apple Home

In the Home app: **+ → Add Accessory → More options… → Firewatch**. The setup code is **466-37-726** unless you set `HOMEKIT_SETUP_CODE`. Accept the "uncertified accessory" warning and place the **Fireplace** (and **Lighter**, if enabled) in the room. The serial log (`pio device monitor`) prints the pairing status. HomeSpan's serial commands also work there: type `?` for the list, and `S 12345678` changes the setup code.

The Fireplace is a dimmable light: its brightness is the same setting as the web page's slider. Siri's "all the lights" commands include it, which turns the TV off or on along with the glow. A command that matches the current state ("turn on" while it's already on) does nothing, so it never interrupts what the TV is playing.

Try it: "Hey Siri, turn on the fireplace". Then add an automation: **When Fireplace turns on → set the living room lights**, and one for **turns off**.

## 7. Daily use

- **On:** press power on the Roku remote. The glow swells in, and within a few seconds the Roku goes from Home to the fire video. You can also use `http://firewatch.local/on` (bookmark it or add it to a phone shortcut) or the Dig-Uno's button.
- **Off:** press power on the remote (the glow fades out), or use `/off`, which also turns the TV off.
- **Apple Home:** the **Fireplace** tile (on/off and brightness), Siri, or any scene that includes it. It also turns on and off by itself when you use the remote.
- **Lighter:** open the lid and roll the wheel to light the fire; close the lid to put it out.
- **Video stuck or paused:** press **Restart video** on the web page, or use `/relaunch`.

Endpoints accept GET or POST: `/on`, `/off`, `/toggle`, `/relaunch`, `/brightness?value=0-100`, `/status` (JSON).

## Troubleshooting

| Symptom | Check |
|---|---|
| `lastError` mentions *Control by mobile apps* | Step 1.2 |
| Board never joins Wi-Fi (no `firewatch.local`) | The network in `secrets.h` is 5 GHz-only; the ESP32 needs 2.4 GHz (step 1.6) |
| `tv: unreachable` while the TV is on | Wrong `ROKU_HOST`, or client isolation / a guest network on the router |
| TV doesn't wake from the web page | Fast TV start is off → turn it on, or fit the IR LED |
| Fire video doesn't start | Try the `curl` launch from step 2; if that fails too, the video may be region-locked or removed, so pick another and update `FIRE_CONTENT_ID` |
| First LEDs flicker or show wrong colors | Data wired to LED2 instead of LED1, the LED1 jumper not on GPIO16, a cracked solder joint, or data entering the wrong end of the strip (check the arrows) |
| Colors look green-tinged | `LED_RGBW` doesn't match the strip, or it's a GRBW vs RGBW variant (change `NeoGrbwFeature` in `leds.cpp`) |
| Firewatch not in Home's "Add Accessory" list | Phone on the same network; the ESP32 needs 2.4 GHz (step 1.6). If it was paired before and removed from Home, type `U` in the serial monitor to unpair, then add it again. |
| Lighter does nothing | `/status` → `lighter.link` must be `ok` (key in `secrets.h`). `rejected` counting up means the two keys differ. `lastSeenS` staying null: out of range, or the lighter isn't sending (check its serial log). |
| Glow too dim on sooty brick | Raise the brightness slider; if the current estimate hits the cap, raise `POWER_BUDGET_MA` (supply permitting) |
| `firewatch.local` sometimes doesn't load from a Windows PC | Windows' `.local` lookups miss now and then; the board itself is fine. Bookmark its IP address instead (shown in the serial log as `[wifi] connected ...`), or give it a DHCP reservation. OTA uploads retry the lookup by themselves. |
| Home says "No Response" | Wi-Fi dropped. HomeSpan reconnects by itself within a minute. Away from home, you need a home hub (Apple TV or HomePod). |
| Re-pair from scratch | Remove Firewatch in the Home app (Accessory Settings → Remove Accessory), then add it again. If Home lost it but `/status` still says `"homekit":"paired"`: USB cable, `pio device monitor`, type `U` and Enter to forget all pairings. |
