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

# Launch your fire video (the id is the part after v= in the YouTube URL):
curl -d '' "http://TV:8060/launch/837?contentId=VIDEO_ID&mediaType=movie"

curl -s http://TV:8060/query/active-app           # expect <app id="837" ...>YouTube</app>
curl -d '' http://TV:8060/keypress/PowerOff
```

- If `keypress` returns **403** or "limited mode", recheck step 1.2.
- If `PowerOn` fails from standby, fast TV start isn't holding the network up. Use the [IR fallback](hardware.md#optional-ir-blaster).

While you're there, do the smart-plug [power-restore test](feasibility.md#1-smart-plug-only-the-no-code-option).

## 3. Firmware

Requires [PlatformIO](https://platformio.org/install/cli) (`pip install platformio`).

```sh
cd firmware
cp include/secrets.example.h include/secrets.h   # Wi-Fi SSID/password, OTA password
$EDITOR include/config.h                          # FIRE_CONTENT_ID, ROKU_HOST, LED layout
pio test -e native                                # host unit tests
pio run -e esp32 -t upload                        # first flash over USB
pio device monitor                                # logs: [wifi] ... [roku] ...
```

Later updates go over Wi-Fi: `pio run -e esp32-ota -t upload`.

What to set in [`config.h`](../firmware/include/config.h):

| Setting | What |
|---|---|
| `FIRE_CONTENT_ID` | Your YouTube fireplace video ID. If it's empty, YouTube only opens to its home screen. |
| `ROKU_HOST` | The TV's reserved IP address. If it's empty, the TV is found by SSDP discovery. |
| `kLedLayout` | The strip's runs in data order, in **front-view** coordinates. The default is 23/41/23/41 LEDs starting at the bottom-left corner, running up the left side and clockwise. |
| `LED_RGBW` | 1 for SK6812 RGBW, 0 for WS2812B/WS2815 RGB strips |
| `POWER_BUDGET_MA` | Supply current cap (3000 mA for the 4 A supply) |
| `DEFAULT_BRIGHTNESS` | Start value. After that, the web page's slider sets it, and it's saved across reboots. |
| `FOLLOW_TV` | 1 = TV power drives the glow (recommended) |
| `IR_LED_PIN` | The GPIO for the optional IR LED (−1 = none) |

To tune the flame's character, edit `FireParams` in [`fire.h`](../firmware/lib/firecore/src/fire.h): flicker bands, rise speed, flare rate, and so on. [`tools/preview`](../tools/preview) renders the result to a GIF on your computer without any hardware.

## 4. Build and install

1. Print `firewatch_base.stl`, `firewatch_lid.stl`, and 20× `strip_clip_10mm.stl` (see the [print notes](hardware.md#mounting)).
2. Solder the perfboard per the [wiring diagram](hardware.md#wiring). Keep the 5 mm corners clear.
3. Before connecting the strip, power the board from the supply and check for 5.0 V on J2/J3 and about 4.7 V at the ESP32's 5V pin.
4. Bench test: connect the strip loosely, flash, open `http://firewatch.local`, and press **Light the fire**.
5. Screw the base onto the TV's VESA holes (M4 × 12 + washers). Drop the board in, run the wires out through the notches, and fasten the lid.
6. Stick on the clips, fit the strip, and bridge the corners. Wire J2 to the strip's start and J3 to its end.
7. Cable-tie the supply lead and strip wires to the anchors.

## 5. Daily use

- **On:** press power on the Roku remote. The glow swells in, and within a few seconds the Roku goes from Home to the fire video. You can also use `http://firewatch.local/on` (bookmark it or add it to a phone shortcut) or the ESP32's BOOT button.
- **Off:** press power on the remote (the glow fades out), or use `/off`, which also turns the TV off.
- **Video stuck or paused:** press **Restart video** on the web page, or use `/relaunch`.

Endpoints accept GET or POST: `/on`, `/off`, `/toggle`, `/relaunch`, `/brightness?value=0-100`, `/status` (JSON).

## Troubleshooting

| Symptom | Check |
|---|---|
| `lastError` mentions *Control by mobile apps* | Step 1.2 |
| `tv: unreachable` while the TV is on | Wrong `ROKU_HOST`, or client isolation / a guest network on the router |
| TV doesn't wake from the web page | Fast TV start is off → turn it on, or fit the IR LED |
| Fire video doesn't start | `FIRE_CONTENT_ID` is empty or wrong; try the `curl` launch from step 2 |
| First LEDs flicker or show wrong colors | Missing common ground, the level shifter is bypassed, or the data wire is too long |
| Colors look green-tinged | `LED_RGBW` doesn't match the strip, or it's a GRBW vs RGBW variant (change `NeoGrbwFeature` in `leds.cpp`) |
| Glow too dim on sooty brick | Raise the brightness slider; if the current estimate hits the cap, raise `POWER_BUDGET_MA` (supply permitting) |
