#include "roku_client.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "config.h"
#include "ir.h"
#include "roku_parse.h"

namespace {
constexpr uint16_t kEcpPort = 8060;
constexpr uint32_t kRediscoverMs = 60000;
}  // namespace

void RokuClient::begin(const char* fixedHost) {
  fixedHost_ = fixedHost;
  host_ = fixedHost_;
  haveMac_[0] = fw::roku::parseMac(ROKU_WIFI_MAC, macs_[0]);
}

bool RokuClient::discover() {
  lastDiscoverMs_ = millis();
  WiFiUDP udp;
  if (!udp.begin(1901)) return false;
  static const char kSearch[] =
      "M-SEARCH * HTTP/1.1\r\nHost: 239.255.255.250:1900\r\nMan: \"ssdp:discover\"\r\n"
      "ST: roku:ecp\r\nMX: 2\r\n\r\n";
  udp.beginPacket(IPAddress(239, 255, 255, 250), 1900);
  udp.write(reinterpret_cast<const uint8_t*>(kSearch), sizeof(kSearch) - 1);
  udp.endPacket();

  char buf[512];
  uint32_t start = millis();
  while (millis() - start < 3000) {
    int len = udp.parsePacket();
    if (len > 0) {
      len = udp.read(buf, sizeof(buf) - 1);
      buf[len > 0 ? len : 0] = 0;
      std::string found = fw::roku::ssdpLocationHost(buf);
      if (!found.empty()) {
        host_ = found.c_str();
        udp.stop();
        Serial.printf("[roku] discovered %s\n", host_.c_str());
        return true;
      }
    }
    delay(20);
  }
  udp.stop();
  return false;
}

bool RokuClient::get(const String& path, String& body) {
  WiFiClient client;
  HTTPClient http;
  http.setConnectTimeout(1000);
  http.setTimeout(2500);
  if (!http.begin(client, host_, kEcpPort, path)) return false;
  int code = http.GET();
  bool ok = code == 200;
  body = ok ? http.getString() : String();
  if (!ok) lastError_ = "GET " + path + " -> " + code;
  http.end();
  return ok;
}

bool RokuClient::post(const String& path) {
  if (host_.isEmpty()) return false;
  WiFiClient client;
  HTTPClient http;
  http.setConnectTimeout(1000);
  http.setTimeout(2500);
  if (!http.begin(client, host_, kEcpPort, path)) return false;
  int code = http.POST("");
  if (code != 200) {
    String body = code > 0 ? http.getString() : String();
    lastError_ = "POST " + path + " -> " + code;
    if (body.indexOf("limited mode") >= 0 || code == 403)
      lastError_ += " (set Settings > System > Advanced system settings > Control by mobile apps to Enabled)";
    Serial.printf("[roku] %s\n", lastError_.c_str());
  }
  http.end();
  return code == 200;
}

fw::TvStatus RokuClient::poll() {
  fw::TvStatus s;
  if (host_.isEmpty() || (fixedHost_.isEmpty() && failures_ >= 10)) {
    if (millis() - lastDiscoverMs_ >= kRediscoverMs || host_.isEmpty()) {
      if (discover()) failures_ = 0;
    }
    if (host_.isEmpty()) return s;
  }

  String info;
  if (!get("/query/device-info", info)) {
    if (failures_ < 0xFFFF) ++failures_;
    return s;
  }
  failures_ = 0;
  std::string xml(info.c_str());
  s.power = fw::roku::powerFromDeviceInfo(xml);
  model_ = fw::roku::tagText(xml, "model-name").c_str();
  haveMac_[0] |= fw::roku::parseMac(fw::roku::tagText(xml, "wifi-mac"), macs_[0]);
  haveMac_[1] |= fw::roku::parseMac(fw::roku::tagText(xml, "ethernet-mac"), macs_[1]);

  if (s.power == fw::TvPower::On) {
    String app;
    if (get("/query/active-app", app)) {
      activeApp_ = fw::roku::activeAppId(app.c_str()).c_str();
      s.fireAppActive = activeApp_ == FIRE_APP_ID;
    }
  } else {
    activeApp_ = "";
  }
  return s;
}

void RokuClient::sendWol() {
  WiFiUDP udp;
  for (int m = 0; m < 2; ++m) {
    if (!haveMac_[m]) continue;
    uint8_t pkt[102];
    memset(pkt, 0xFF, 6);
    for (int i = 0; i < 16; ++i) memcpy(pkt + 6 + i * 6, macs_[m], 6);
    udp.beginPacket(IPAddress(255, 255, 255, 255), 9);
    udp.write(pkt, sizeof(pkt));
    udp.endPacket();
  }
}

void RokuClient::wake() {
  Serial.println("[roku] wake");
  ir::powerOn();
  sendWol();
  post("/keypress/PowerOn");
}

void RokuClient::powerOff() {
  Serial.println("[roku] power off");
  if (!post("/keypress/PowerOff")) ir::powerOff();
}

void RokuClient::launchFire() {
  String path = "/launch/" FIRE_APP_ID;
  std::string content = FIRE_CONTENT_ID;
  if (!content.empty()) {
    path += "?contentId=";
    path += fw::roku::urlEncode(content).c_str();
    path += "&mediaType=" FIRE_MEDIA_TYPE;
  }
  Serial.printf("[roku] launch %s\n", path.c_str());
  post(path);
}
