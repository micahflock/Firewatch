#pragma once
// Everything you're likely to tune lives here. Wi-Fi credentials go in secrets.h.

#include "layout.h"

// ---- LED strip -------------------------------------------------------------------
// 1 = SK6812 RGBW (recommended, warm-white die), 0 = RGB strip (WS2812B/WS2815).
#define LED_RGBW 1
// QuinLED Dig-Uno v3: the "LED1" output terminal is GPIO16 (level-shifted on the board).
// Its jumper can move LED1 to GPIO1, but GPIO1 is the serial TX pin: keep it on GPIO16.
#define LED_PIN 16

// Strip wiring, in data order, as seen FROM THE FRONT of the TV (x: 0=left, 1=right;
// y: 0=bottom, 1=top). Default: 60 LED/m on the left side, top and right side of a TCL
// 32S335 back, inset ~20 mm, no bottom edge. Data enters at the bottom of the left side
// (front view) and runs clockwise, ending at the bottom of the right side.
static const fw::Segment kLedLayout[] = {
    {0, 0, 0, 1, 23},  // left side, bottom -> top
    {0, 1, 1, 1, 41},  // top, left -> right
    {1, 1, 1, 0, 23},  // right side, top -> bottom
};

// Power budget for the whole 5 V rail (PSU is 4 A; keep headroom).
#define POWER_BUDGET_MA 3000
#define MA_PER_CHANNEL 15  // measured-ish for 5050 SK6812/WS2812B at full duty

// Default master brightness (0-100). Adjustable at runtime from the web page.
#define DEFAULT_BRIGHTNESS 60
#define FADE_IN_S 3.0f
#define FADE_OUT_S 2.5f
#define TARGET_FPS 120  // high frame rate makes the temporal dithering invisible

// ---- Roku TV -----------------------------------------------------------------------
// Leave empty to auto-discover via SSDP. A DHCP reservation + fixed IP is more robust.
#define ROKU_HOST ""
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
