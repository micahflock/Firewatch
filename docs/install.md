# Setup and install

## 1. TV settings (5 minutes)

On the TCL 32S335:

1. **Settings → System → Power → Fast TV start → On.** Required for waking the TV from Firewatch. Following the TV's power works without it.
2. **Settings → System → Advanced system settings → Control by mobile apps → Network access → Default** (or Enabled/Permissive). Not *Limited* or *Disabled*.
3. **Settings → System → Power → Power On → Home.**
4. Optional: **Settings → System → Power → Auto power savings.** Turn off "turn off after 4 hours" if you want to run the fire longer without touching the remote. Leave it on if you like it as a sleep timer; the glow follows the TV either way.
5. In your router, give the TV a **DHCP reservation**, so its IP address never changes.

## 2. Prove the network path before building anything

From a computer on the same network (replace `TV` with the TV's IP address):

```sh
# Is ECP reachable, and is the screen on?
curl -s http://TV:8060/query/device-info | grep -E "model-name|power-mode|wifi-mac"

# Put the TV in standby with the remote, wait 10 s, then:
curl -d '' http://TV:8060/keypress/PowerOn        # the screen should come on

# Launch your fire video (youtu.be/CF00JqnCcsU):
curl -d '' "http://TV:8060/launch/837?contentId=CF00JqnCcsU&mediaType=movie"

curl -s http://TV:8060/query/active-app           # expect <app id="837" ...>YouTube</app>
curl -d '' http://TV:8060/keypress/PowerOff
```

- If `keypress` returns **403** or "limited mode", recheck step 1.2.
- If `PowerOn` fails from standby, fast TV start isn't holding the network up. Use the [IR fallback](hardware.md#shopping-list-from-scratch).

While you're there, do the smart-plug [power-restore test](feasibility.md#1-smart-plug-only-the-no-code-option).

## 3. Bench test with WLED (no code)

The Dig-Uno ships with WLED installed, which is handy for proving the wiring before you flash anything.

1. Land the barrel pigtail's leads in the Dig-Uno's power input (+ found with a multimeter), then plug in the supply. Solder the 22 AWG start leads to the strip and land them in the LED output (5V / LED1 / GND), per the [wiring diagram](hardware.md#wiring). Lay the strip loose on a table.
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

What to set in [`config.h`](../firmware/include/config.h):

| Setting | What |
|---|---|
| `FIRE_CONTENT_ID` | The YouTube fireplace video ID. It's already set to yours (`CF00JqnCcsU`). |
| `ROKU_HOST` | The TV's reserved IP address. If it's empty, the TV is found by SSDP discovery. |
| `kLedLayout` | The strip's runs in data order, in **front-view** coordinates. The default is 23/41/23 LEDs: up the left side from the bottom, across the top, and down the right side. There's no bottom edge. |
| `LED_RGBW` | 1 for SK6812 RGBW, 0 for WS2812B/WS2815 RGB strips |
| `POWER_BUDGET_MA` | Supply current cap (3000 mA for the 4 A supply) |
| `DEFAULT_BRIGHTNESS` | Start value. After that, the web page's slider sets it, and it's saved across reboots. |
| `FOLLOW_TV` | 1 = TV power drives the glow (recommended) |
| `IR_LED_PIN` | The GPIO for the optional IR LED: 15 (the Dig-Uno's Q1), or −1 for none |
| `LED_PIN`, `BUTTON_PIN` | Already match the Dig-Uno (LED1 = GPIO16, button = GPIO0) |

To tune the flame's character, edit `FireParams` in [`fire.h`](../firmware/lib/firecore/src/fire.h): flicker bands, rise speed, flare rate, and so on. [`tools/preview`](../tools/preview) renders the result to a GIF on your computer without any hardware.

## 5. Build and install

1. Print `firewatch_base.stl`, `firewatch_lid.stl`, and 14× `strip_clip_10mm.stl` (see the [print notes](hardware.md#mounting)).
2. On the bench, cut the strip into its three runs (23/41/23 LEDs at the cut marks). Solder the two corner bridges and the 22 AWG lead at the start, heat-shrink each joint, and cover the far end's pads (see the [wiring notes](hardware.md#wiring)).
3. Find + on the barrel pigtail with a multimeter and land its leads in the Dig-Uno's power input. Check that the LED1 jumper is on **GPIO16**; the firmware refuses to build for GPIO1.
4. Screw the base onto the TV's VESA holes (M4 × 12), with the LED-end notch facing the bottom-left corner (front view), where the strip starts.
5. Fix the Dig-Uno onto the standoffs with four M2 × 8 screws, with its power terminal and USB-C toward the window. Land the strip's 22 AWG lead in the LED output terminal (5V / LED1 / GND). Cable-tie the pigtail's jack and the strip leads to the anchors. From then on, the supply unplugs at the jack.
6. Stick on the clips and fit the strip. Hold the wire runs down with scraps of VHB.
7. Open `http://firewatch.local` and press **Light the fire**. Check that the steady part of the glow is low on the sides and the lively flicker is across the top. If it's the other way round, the layout direction is reversed. Then fasten the lid with four M2 × 8 screws.

## 6. Daily use

- **On:** press power on the Roku remote. The glow swells in, and within a few seconds the Roku goes from Home to the fire video. You can also use `http://firewatch.local/on` (bookmark it or add it to a phone shortcut) or the Dig-Uno's button.
- **Off:** press power on the remote (the glow fades out), or use `/off`, which also turns the TV off.
- **Video stuck or paused:** press **Restart video** on the web page, or use `/relaunch`.

Endpoints accept GET or POST: `/on`, `/off`, `/toggle`, `/relaunch`, `/brightness?value=0-100`, `/status` (JSON).

## Troubleshooting

| Symptom | Check |
|---|---|
| `lastError` mentions *Control by mobile apps* | Step 1.2 |
| `tv: unreachable` while the TV is on | Wrong `ROKU_HOST`, or client isolation / a guest network on the router |
| TV doesn't wake from the web page | Fast TV start is off → turn it on, or fit the IR LED |
| Fire video doesn't start | Try the `curl` launch from step 2; if that fails too, the video may be region-locked or removed, so pick another and update `FIRE_CONTENT_ID` |
| First LEDs flicker or show wrong colors | Data wired to LED2 instead of LED1, the LED1 jumper not on GPIO16, a cracked solder joint, or data entering the wrong end of the strip (check the arrows) |
| Colors look green-tinged | `LED_RGBW` doesn't match the strip, or it's a GRBW vs RGBW variant (change `NeoGrbwFeature` in `leds.cpp`) |
| Glow too dim on sooty brick | Raise the brightness slider; if the current estimate hits the cap, raise `POWER_BUDGET_MA` (supply permitting) |
