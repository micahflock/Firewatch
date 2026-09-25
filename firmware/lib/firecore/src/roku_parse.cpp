#include "roku_parse.h"

#include <cctype>

namespace fw::roku {

std::string tagText(const std::string& xml, const char* tag) {
  const std::string open = std::string("<") + tag + ">";
  const std::string close = std::string("</") + tag + ">";
  size_t a = xml.find(open);
  if (a == std::string::npos) return "";
  a += open.size();
  size_t b = xml.find(close, a);
  if (b == std::string::npos) return "";
  return xml.substr(a, b - a);
}

TvPower powerFromDeviceInfo(const std::string& deviceInfo) {
  if (deviceInfo.find("<device-info") == std::string::npos) return TvPower::Unreachable;
  if (deviceInfo.find("<power-mode>") == std::string::npos) return TvPower::On;
  return tagText(deviceInfo, "power-mode") == "PowerOn" ? TvPower::On : TvPower::Standby;
}

std::string activeAppId(const std::string& xml) {
  size_t app = xml.find("<app");
  if (app == std::string::npos) return "";
  size_t end = xml.find('>', app);
  if (end == std::string::npos) return "";
  size_t id = xml.find(" id=\"", app);
  if (id == std::string::npos || id > end) return "";
  id += 5;
  size_t q = xml.find('"', id);
  if (q == std::string::npos || q > end) return "";
  return xml.substr(id, q - id);
}

std::string ssdpLocationHost(const std::string& resp) {
  std::string lower(resp.size(), '\0');
  for (size_t i = 0; i < resp.size(); ++i) lower[i] = static_cast<char>(std::tolower((unsigned char)resp[i]));
  size_t h = lower.find("\nlocation:");
  if (h == std::string::npos) return "";
  size_t p = lower.find("http://", h);
  if (p == std::string::npos) return "";
  p += 7;
  size_t e = p;
  while (e < resp.size() && resp[e] != ':' && resp[e] != '/' && resp[e] != '\r' && resp[e] != '\n') ++e;
  return resp.substr(p, e - p);
}

bool parseMac(const std::string& t, uint8_t out[6]) {
  if (t.size() != 17) return false;
  bool nonzero = false;
  for (int i = 0; i < 6; ++i) {
    unsigned v = 0;
    for (int k = 0; k < 2; ++k) {
      char c = t[i * 3 + k];
      v <<= 4;
      if (c >= '0' && c <= '9') v |= c - '0';
      else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
      else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
      else return false;
    }
    if (i < 5 && t[i * 3 + 2] != ':' && t[i * 3 + 2] != '-') return false;
    out[i] = static_cast<uint8_t>(v);
    nonzero |= v != 0;
  }
  return nonzero;
}

std::string urlEncode(const std::string& s) {
  static const char* hex = "0123456789ABCDEF";
  std::string o;
  for (unsigned char c : s) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      o += static_cast<char>(c);
    } else {
      o += '%';
      o += hex[c >> 4];
      o += hex[c & 15];
    }
  }
  return o;
}

}  // namespace fw::roku
