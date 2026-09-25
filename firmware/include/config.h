#pragma once
// Everything you're likely to tune lives here. Wi-Fi credentials go in secrets.h.

#include "layout.h"

// ---- LED strip -------------------------------------------------------------------
// 1 = SK6812 RGBW (recommended, warm-white die), 0 = RGB strip (WS2812B/WS2815).
#define LED_RGBW 1
#define LED_PIN 16  // through the 74AHCT125 level shifter

// Strip wiring, in data order, as seen FROM THE FRONT of the TV (x: 0=left, 1=right;
// y: 0=bottom, 1=top). Default: 60 LED/m loop inset ~20 mm on a TCL 32S335 back,
// data entering at the bottom-left corner (front view) and running clockwise.
// A run with count 0 is skipped, e.g. if you leave the bottom edge unlit.
static const fw::Segment kLedLayout[] = {
    {0, 0, 0, 1, 23},  // left side, bottom -> top
    {0, 1, 1, 1, 41},  // top, left -> right
    {1, 1, 1, 0, 23},  // right side, top -> bottom
    {1, 0, 0, 0, 41},  // bottom, right -> left
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
// "v=" in the URL). Paste the id of the fireplace video you already use; a long (8-12 h)
// one avoids end-of-video autoplay. Left empty, YouTube just opens to its home screen.
#define FIRE_APP_ID "837"
#define FIRE_CONTENT_ID ""
#define FIRE_MEDIA_TYPE "movie"

#define POLL_INTERVAL_MS 2000
#define FOLLOW_TV 1  // TV power (Roku remote etc.) starts/stops the scene

// ---- Optional hardware -----------------------------------------------------------------
// Momentary button to GND toggles the scene. GPIO0 is the dev board's BOOT button.
#define BUTTON_PIN 0
// IR fallback for TVs whose network sleeps in standby (fast TV start off/unsupported).
// Set to the GPIO driving the IR LED transistor, or -1 to disable.
#define IR_LED_PIN -1

#define HOSTNAME "firewatch"
