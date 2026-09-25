#pragma once
#include <cstdint>
#include <string>

#include "scene.h"

// Minimal parsers for Roku ECP responses. ECP returns small, flat, well-formed XML,
// so tag scanning is enough; no XML library needed on the ESP32.
namespace fw::roku {

// Text content of the first <tag>...</tag>, or "" if absent.
std::string tagText(const std::string& xml, const char* tag);

// device-info <power-mode>: "PowerOn" means the screen is on; "DisplayOff", "Ready",
// etc. are standby variants. Devices without the field (sticks) count as on.
TvPower powerFromDeviceInfo(const std::string& deviceInfo);

// query/active-app: <app id="837" ...>YouTube</app> -> "837"; home screen -> "".
std::string activeAppId(const std::string& activeApp);

// SSDP reply "LOCATION: http://192.168.1.23:8060/" -> "192.168.1.23" ("" if absent).
std::string ssdpLocationHost(const std::string& response);

// "a8:b1:c2:00:11:ff" -> bytes. Returns false if malformed or all-zero.
bool parseMac(const std::string& text, uint8_t out[6]);

// Percent-encode for a query-string value.
std::string urlEncode(const std::string& s);

}  // namespace fw::roku
