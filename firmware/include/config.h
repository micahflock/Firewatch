#pragma once
// Everything you're likely to tune lives here. Wi-Fi credentials go in secrets.h.

#include "layout.h"

// ---- LED strip -------------------------------------------------------------------
// 1 = SK6812 RGBW (recommended, warm-white die), 0 = RGB strip (WS2812B/WS2815).
#define LED_RGBW 1
// QuinLED Dig-Uno v3: the "LED1" output terminal is GPIO16 (level-shifted on the board).
// Its jumper can move LED1 to GPIO1, but GPIO1 is the serial TX pin: keep it on GPIO16.
#define LED_PIN 16

// LEDs at the start of the strip that stay dark: a short lead-in that extends the plug end
// to where the first lit run begins. They're still clocked through, just never lit.
#define LED_LEAD_IN 3

// Strip wiring after the lead-in, in data order, as seen FROM THE FRONT of the TV (x: 0=left,
// 1=right; y: 0=bottom, 1=top). As fitted: 60 LED/m on the left side, top and right side of
// a TCL 32S335 back, bent (not cut) around the two top corners, no bottom edge. Data enters
// at the bottom of the left side (front view) and runs clockwise, ending at the bottom of the
// right side.
static const fw::Segment kLedLayout[] = {
    {0, 0, 0, 1, 17},  // left side, bottom -> top
    {0, 1, 1, 1, 38},  // top, left -> right
    {1, 1, 1, 0, 17},  // right side, top -> bottom
};

// Power budget for the whole 5 V rail. 2.5 A leaves headroom on a 3 A supply; the fire
// peaks around 1.6 A at full brightness, so this cap is a safety net, not a limiter.
#define POWER_BUDGET_MA 2500
#define MA_PER_CHANNEL 15  // measured-ish for 5050 SK6812/WS2812B at full duty

// Default master brightness (0-100). Adjustable at runtime from the web page.
#define DEFAULT_BRIGHTNESS 60
#define FADE_IN_S 3.0f
#define FADE_OUT_S 2.5f
#define TARGET_FPS 120  // high frame rate makes the temporal dithering invisible

// ---- Roku TV -----------------------------------------------------------------------
// The TV's reserved IP. Leave empty to auto-discover via SSDP instead.
#define ROKU_HOST "192.168.1.176"
// The TV's Wi-Fi MAC (Settings > Network > About) for Wake-on-LAN before the firmware has
// read it from the TV itself, e.g. right after a power cut. Leave empty to skip.
#define ROKU_WIFI_MAC "D4:AB:CD:A8:96:96"
// YouTube's Roku channel id is 837; contentId is the YouTube video id (the part after
// "v=" in the URL, or after youtu.be/). A long (8-12 h) video
// avoids end-of-video autoplay. Left empty, YouTube just opens to its home screen.
#define FIRE_APP_ID "837"
#define FIRE_CONTENT_ID "CF00JqnCcsU"  // youtu.be/CF00JqnCcsU
#define FIRE_MEDIA_TYPE "movie"

#define POLL_INTERVAL_MS 2000
#define FOLLOW_TV 1  // TV power (Roku remote etc.) starts/stops the scene

// ---- Optional hardware -----------------------------------------------------------------
// Momentary button to GND toggles the scene. GPIO0 is the Dig-Uno's button pin (and the
// QuinLED-ESP32's boot button). Set to -1 to disable.
#define BUTTON_PIN 0
// IR fallback for TVs whose network sleeps in standby (fast TV start off/unsupported).
// Set to the GPIO driving the IR LED transistor, or -1 to disable. On the Dig-Uno use
// Q1 = GPIO15 (Q1/Q2 header): it's pulled low, so the IR LED stays off while booting.
#define IR_LED_PIN -1

#define HOSTNAME "firewatch"

// ---- Apple Home (HomeSpan) ---------------------------------------------------------
// Firewatch shows up in the Home app as a dimmable "Fireplace" light (plus the lighter, below).
// Pair it from the Home app: Add Accessory -> More options -> Firewatch, setup code
// 466-37-726 unless you set HOMEKIT_SETUP_CODE in secrets.h.
#define HOMEKIT_PORT 1201  // HomeKit's own port; the web page stays on 80

// ---- Fireplace lighter (Skeumo BLE talisman) ---------------------------------------------
// A battery lighter that broadcasts signed BLE frames. Needs SKEUMO_LINK_KEY in secrets.h,
// identical to the lighter's. Set to 0 to leave Bluetooth off entirely.
#define LIGHTER_ENABLED 0
#define LIGHTER_DEVICE_ID 1       // must match the lighter's LIGHTER_DEVICE_ID
#define LIGHTER_STRIKE_CHANNEL 1  // fixed by the lighter firmware
#define LIGHTER_SNUFF_CHANNEL 2
// 1 = a strike lights the fire and a snuff puts it out directly (fast, and works even when
// the Home hub is away). The strike/snuff also appear in Apple Home either way, for scenes.
#define LIGHTER_DIRECT 1
