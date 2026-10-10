#pragma once
// Copy to secrets.h (git-ignored) and fill in.
#define WIFI_SSID "your-wifi"
#define WIFI_PASSWORD "your-password"
#define OTA_PASSWORD "change-me"

// Optional: your own 8-digit Apple Home setup code. The build refuses codes Apple rejects:
// all the same digit, 12345678 or 87654321. Without it, HomeSpan's default 466-37-726 is used.
// #define HOMEKIT_SETUP_CODE "20481357"

// Shared secret for the fireplace lighter: 32 hex digits, identical to SKEUMO_LINK_KEY in
// the lighter's secrets.h. Make one with:
//   python -c "import secrets; print(secrets.token_hex(16))"
#define SKEUMO_LINK_KEY "REPLACE_WITH_32_HEX_DIGITS"
