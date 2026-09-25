# Hardware: survey, recommendation, parts list, wiring

## What the job actually needs

- **Wi-Fi.** The controller has to talk to the Roku (ECP) and serve a small web page. This rules out plain MCUs.
- **Glitch-free LED timing while Wi-Fi is busy.** The LED bitstream should come from DMA hardware, not CPU bit-banging.
- **Good low-brightness color.** A fire glow in a dark room lives in the bottom 10–30% of an LED's range, where 8-bit channels show visible steps.
- **Warm color quality.** The light falls on brick, the mantel, and the room, so it should look like firelight, not like "RGB orange".
- **About 2.2 m of strip.** A loop set about 20 mm in from the edges of the TV's back (the TV is 731 × 437 mm). This is short enough that voltage drop is a non-issue at 5 V, especially since both ends of the loop come back to the same corner, so you can feed power from both ends.

The light doesn't need to be dotless or high-density. The strip faces the firebox walls, not the viewer, so all you see is diffuse reflection.

## Controller options

| Option | Strengths | Weaknesses | Verdict |
|---|---|---|---|
| **ESP32 (classic, WROOM-32E DevKitC)** | Two cores (network on one, animation on the other). I2S/RMT DMA LED output that Wi-Fi can't disturb. The most mature LED ecosystem (WLED, FastLED, NeoPixelBus). OTA updates. About $10. | 3.3 V logic needs a level shifter for 5 V strips. | **Recommended** |
| ESP32-S3 | Native USB, more RAM, PSRAM variants | No benefit for 128 LEDs; the parallel-I2S LED drivers are younger on the S3 | Fine, not better |
| ESP32-C3 / C6 | Tiny and cheap | Single core shared with Wi-Fi; fewer RMT channels; C6 LED support is still young | Workable, not ideal |
| Raspberry Pi Pico 2 W | PIO gives perfect LED timing | HTTP, mDNS and OTA are less polished under Arduino; no real advantage here | No |
| Ready-made ESP32 LED controllers (QuinLED Dig-Uno, Gledopto/Athom ESP32 "WLED" boxes) | Level shifter, fuse, screw terminals and case built in; no soldering | Pinouts vary by vendor; not all expose USB for flashing; needs its own mounting | **Good no-solder alternative.** It's still an ESP32, so this firmware runs on it: set `LED_PIN`. |
| Raspberry Pi Zero 2 W | Could also drive a screen | Overkill, SD card wear, 30 s boot, no real-time I/O | No |

## LED strip options

| Strip | Supply | Color | Strengths | Weaknesses |
|---|---|---|---|---|
| WS2812B | 5 V | RGB, 8-bit | Cheap and everywhere | "Warm" is mixed from narrow red and green peaks, which looks synthetic on brick and wood. Steps at low brightness. |
| **SK6812 RGBW, warm-white (≈3000 K)** | 5 V | RGB + W, 8-bit | The white die is phosphor-converted and broad-spectrum, so the glow's body looks like real warm light. RGB adds the red and orange flicker on top. Same one-wire protocol and libraries as WS2812B. Better lumens per watt for warm tones. | Slightly pricier. W is also 8-bit (the firmware dithers). |
| WS2815 | 12 V | RGB | Backup data line, lower current | Needs a buck converter for the ESP32; RGB only; 12 V is pointless at 2.2 m |
| APA102 / SK9822 | 5 V | RGB + 5-bit global | Fast PWM; the 5-bit global brightness helps low end | Four wires, pricier, still RGB-only color |
| HD108 | 5 V | 16-bit RGB | Excellent dimming | Niche, expensive, young library support |
| COB/FCOB addressable (WS2811, 12/24 V) | 12/24 V | RGB, coarse segments | Dotless | Dots are invisible here anyway (the light is reflected); coarse addressing |

## Recommendation

**Classic ESP32 DevKitC (WROOM-32E) + SK6812 RGBW warm-white, 5 V, 60 LED/m, 10 mm, IP30**, with a 74AHCT125 level shifter on a 50 × 70 mm perfboard inside the printed enclosure on the TV's VESA holes.

Why this combination:
- **RGBW warm-white is the biggest quality lever.** Real firelight is broadband, and a phosphor white die gets much closer to it than any RGB mix. The fire palette ([`fire.cpp`](../firmware/lib/firecore/src/fire.cpp)) uses W for the glow's body and RGB for the red and orange movement.
- **Low-brightness steps are handled in software.** Colors are computed in 16-bit and temporally dithered to 8-bit at 120 fps. That's cheaper and simpler than moving to 16-bit or SPI LEDs.
- **The classic ESP32 is the lowest-risk platform for Wi-Fi plus LEDs.** Its I2S DMA output is immune to Wi-Fi interrupt jitter, and the second core keeps slow Roku HTTP calls away from the animation.
- **60 LED/m over 30/m:** more headroom and a smoother glow for a few dollars more.
- **IP30 (uncoated):** silicone-coated strips trap heat, yellow over time, and hold adhesive worse. There's no moisture in a firebox that isn't burning.

If you'd rather not solder, use a QuinLED Dig-Uno or a similar ESP32 controller in the same role, and mount it on the VESA plate with VHB. The enclosure is designed around the perfboard build.

## Parts list

Prices are rough 2026 US street prices.

| # | Part | Qty | Notes | ≈ $ |
|---|---|---|---|---|
| 1 | ESP32-DevKitC-32E (Espressif) or a 38-pin WROOM-32 clone | 1 | Any ESP32 dev board works if it fits 50 × 70 mm | 10 |
| 2 | SK6812 RGBW **WW (3000 K)**, 5 V, 60 LED/m, 10 mm, IP30 (e.g. BTF-LIGHTING) | 1 × 5 m reel | Uses 2.2 m (128 LEDs). The spare covers mistakes. | 35 |
| 3 | Mean Well GST25A05-P1J (5 V 4 A desktop supply, 5.5 × 2.1 mm plug) | 1 | Firmware caps the draw at 3 A | 15 |
| 4 | 74AHCT125N (DIP-14) + socket | 1 | 3.3 V → 5 V data level shift | 1 |
| 5 | 50 × 70 mm double-sided perfboard | 1 | | 1 |
| 6 | 1 × 19 female headers (2.54 mm) | 2 | The ESP32 plugs in and can be removed | 1 |
| 7 | 5.08 mm PCB screw terminals: 2-pole ×2, 3-pole ×1 | 3 | Power in, strip start (5V/GND/DIN), strip end (5V/GND) | 2 |
| 8 | Female 5.5 × 2.1 mm barrel jack to screw terminal adapter | 1 | Sits outside the box on a cable-tie anchor | 1 |
| 9 | Inline blade fuse holder (18 AWG) + 5 A ATO fuse | 1 | On the 5 V lead | 3 |
| 10 | 1000 µF 10 V electrolytic | 1 | Across 5 V/GND at the strip terminals | 0.5 |
| 11 | 100 nF ceramic | 1 | 74AHCT125 decoupling | 0.1 |
| 12 | 330 Ω resistor | 1 | In series with the data line, at the shifter | 0.1 |
| 13 | 1N5819 Schottky diode | 1 | 5 V rail → ESP32 5V pin, so USB can't power the strip | 0.2 |
| 14 | 18 AWG 2-core wire | 3 m | Supply lead and both strip power feeds | 3 |
| 15 | 22 AWG 3-core wire, or the strip's JST-SM pigtail | 1 m | Data + GND + 5 V to the strip start | 2 |
| 16 | M4 × 12 mm screws + M4 washers | 4 | VESA (the 32S335 uses M4 on 100 × 100) | 1 |
| 17 | M3 × 8 mm self-tapping screws (or M3 heat-set inserts + M3 × 8 machine screws) | 4 | Lid | 1 |
| 18 | 3M VHB tape, 12 mm (e.g. 5952) | 1 roll | Strip clips onto the TV back | 6 |
| 19 | Cable ties (≤ 3.6 mm wide), adhesive cable clips | a few | Strain relief and wire routing | 2 |
| 20 | Printed parts: `firewatch_base.stl`, `firewatch_lid.stl`, `strip_clip_10mm.stl` × 20 | | PETG preferred | — |
| | **Total** | | | **≈ $85** |

### Optional IR blaster

Only needed if you can't use Fast TV start. See [feasibility](feasibility.md#3-ir-blaster-fallback).

| Part | Qty | Notes |
|---|---|---|
| 940 nm IR LED (e.g. Vishay TSAL6200) | 1 | Aim it at the IR window on the TV's front bottom edge |
| PN2222A NPN transistor | 1 | |
| 1 kΩ (base), 33 Ω (LED) resistors | 1 each | About 100 mA pulses from 5 V |
| Thin 2-core wire | 1.5 m | Run from the box, around the TV edge, to the front |

## Wiring

```
 GST25A05 5V/4A ──[5A fuse]──► barrel adapter ──18AWG──► J1 (+5V, GND)   ← PSU lead in
                                                          │
                     ┌────────────────────────────────────┴───────── +5V rail ─────────────┐
                     │                                                                     │
          1N5819 ────┤                                                                     │
      (anode on rail)│                                                  1000µF ═╪═         │
                     ▼                                                         │           │
             ESP32 "5V" pin          74AHCT125 (VCC=+5V, 100nF to GND)         │           │
             ESP32 GND ── GND        pin 1 (1OE) ── GND                        │           │
             GPIO16 ───────────────► pin 2 (1A)                                │           │
                                     pin 3 (1Y) ──[330Ω]──► J2 DIN ─────────────┼──► strip DIN
                                     pins 5,9,12 (2A,3A,4A) ── GND             │
                                     pins 4,10,13 (2OE,3OE,4OE) ── +5V         │
                                                                               │
                                     J2  +5V / GND / DIN ──► strip START (3 wires, ≤ 1 m)
                                     J3  +5V / GND       ──► strip END   (power injection)

 Optional:  GPIO0 = on-board BOOT button (toggles the scene).
            GPIO4 ──[1k]── PN2222 base; emitter → GND; collector → IR LED cathode;
            IR LED anode ──[33Ω]── +5V.   Then set IR_LED_PIN 4 in config.h.
```

Notes:
- **Every GND is common.** Run the data wire alongside a ground wire.
- **Feed power to both ends of the loop.** The strip starts and ends at the same corner, so J3 is a second 18 AWG pair to the last LED's 5V/GND pads.
- **The 1N5819 matters.** If you plug USB into the ESP32 with the strip connected and no PSU, the diode stops the laptop's USB port from trying to power 128 LEDs.
- **Don't cut strip power when "off".** An unpowered strip gets back-powered through its data line. Idle draw is about 1 mA per LED (≈0.6 W), plus about 0.5 W for the ESP32.

### Perfboard layout

- The ESP32 sits on the female headers at the **+Y end**, with its USB port facing the enclosure's USB notch (flush with the board edge).
- The 74AHCT125 (socketed), the 330 Ω resistor, and the 1N5819 fit **under the ESP32**, between the header rows. The headers leave about 8 mm of clearance.
- The screw terminals (J1, J2, J3) and the 1000 µF capacitor (laid flat) go at the **−Y end**, facing the two cable notches.
- **Keep 5 mm at each board corner free of parts.** The base's standoffs and the lid's clamping posts land there.

## Power budget

| | Current (5 V) |
|---|---|
| Theoretical max: 128 LEDs × 4 channels × ~15 mA | 7.7 A (never reached) |
| Firmware cap (`POWER_BUDGET_MA`), including the ESP32 | 3.0 A |
| Fire at 60% brightness (default), firmware estimate over 100 s, all-in | avg 1.4 A, peak 1.75 A |
| Fire at 100% brightness | avg 2.0 A, peak 2.8 A |
| Off (ESP32 on Wi-Fi + strip idle) | ≈ 0.2 A (≈ 1.1 W) |

The web page shows the firmware's live current estimate (`ledCurrentMa`).

## Mounting

![Base](img/base.png) ![Lid](img/lid.png) ![Clip](img/clip.png)

Source: [`hardware/mount/firewatch_mount.scad`](../hardware/mount/firewatch_mount.scad) (parametric). STLs are in [`hardware/mount/stl/`](../hardware/mount/stl).

| File | What | Print |
|---|---|---|
| `firewatch_base.stl` | X-shaped VESA plate + electronics box. The diagonal slots fit **both 75 × 75 and 100 × 100** patterns (the 32S335 is 100 × 100, M4). Cable notches, a USB notch, cable-tie anchors, board standoffs. | Flat side down, 3 walls, 20% infill, no supports |
| `firewatch_base_M6.stl` | Same, with M6 clearance slots for other TVs | |
| `firewatch_lid.stl` | Vented lid with a locating lip and four posts that clamp the perfboard's corners | Flat (outer) face down, no supports |
| `strip_clip_10mm.stl` / `strip_clip_12mm.stl` | 45° clip that holds the strip facing outward and backward toward the firebox walls. The strip slides in from the end, and lips hold the PCB edges clear of the LEDs. | **Profile face down** (as exported). 20 needed. |

**Installing the base:** use M4 × 12 screws with washers through the 4 mm plate, which gives about 7 mm of thread engagement. Turn each screw in by hand first: if one stops before the head seats, it's bottoming out in the insert, so use a shorter screw rather than forcing it. The slots near the 75 mm positions sit close to the box walls, so use a screwdriver with a shaft of at least 75 mm. The 100 mm positions have plenty of clearance.

**Strip placement:** put clips about 20 mm in from the TV's outer edge, every ~15 cm, plus one on each side of every corner (about 16–20 in total), with the channel opening pointing outward. At the corners, cut the strip at a copper cut mark and bridge the three pads with short wires. The strip can't bend in its own plane. Clean the TV back with isopropyl alcohol before applying the VHB, and press for 30 s.
