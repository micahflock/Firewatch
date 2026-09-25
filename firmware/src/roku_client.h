#pragma once
#include <Arduino.h>

#include "scene.h"

// Roku External Control Protocol (HTTP on port 8060). All calls block for at most a
// few seconds, so this is only used from the network task, never the render loop.
class RokuClient {
 public:
  void begin(const char* fixedHost);
  fw::TvStatus poll();
  void wake();
  void powerOff();
  void launchFire();

  String host() const { return host_; }
  String model() const { return model_; }
  String activeApp() const { return activeApp_; }
  String lastError() const { return lastError_; }

 private:
  bool discover();
  bool get(const String& path, String& body);
  bool post(const String& path);
  void sendWol();

  String fixedHost_, host_, model_, activeApp_, lastError_;
  uint8_t macs_[2][6] = {};
  bool haveMac_[2] = {false, false};
  uint32_t lastDiscoverMs_ = 0;
  uint16_t failures_ = 0;
};
