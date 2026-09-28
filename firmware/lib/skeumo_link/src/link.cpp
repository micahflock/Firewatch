#include "link.h"

#include "sha256.h"

namespace skeumo {
namespace link {
namespace {

void tag(const Key& key, const uint8_t* body, uint8_t out[kTagLen]) {
  uint8_t mac[Sha256::kDigestLen];
  hmacSha256(key.data(), key.size(), body, kBodyLen, mac);
  for (size_t i = 0; i < kTagLen; ++i) out[i] = mac[i];
}

int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

}  // namespace

MfgData encode(const Key& key, const Message& m) {
  MfgData d{};
  d[0] = kCompanyId & 0xFF;
  d[1] = kCompanyId >> 8;
  uint8_t* f = d.data() + 2;
  f[0] = 'S';
  f[1] = 'K';
  f[2] = kVersion;
  f[3] = m.deviceId;
  f[4] = m.channel;
  f[5] = static_cast<uint8_t>(m.press);
  f[6] = m.batteryPct;
  for (int i = 0; i < 4; ++i) f[7 + i] = uint8_t(m.counter >> (8 * i));
  tag(key, f, f + kBodyLen);
  return d;
}

Result decode(const Key& key, const uint8_t* data, size_t len, Message& out) {
  if (len != kMfgDataLen || data[0] != (kCompanyId & 0xFF) || data[1] != (kCompanyId >> 8) ||
      data[2] != 'S' || data[3] != 'K')
    return Result::NotOurs;
  const uint8_t* f = data + 2;
  if (f[2] != kVersion) return Result::BadVersion;

  uint8_t expected[kTagLen];
  tag(key, f, expected);
  uint8_t diff = 0;  // constant-time compare
  for (size_t i = 0; i < kTagLen; ++i) diff |= expected[i] ^ f[kBodyLen + i];
  if (diff) return Result::BadTag;

  out.deviceId = f[3];
  out.channel = f[4];
  out.press = f[5] <= 2 ? static_cast<Press>(f[5]) : Press::Single;
  out.batteryPct = f[6];
  out.counter = 0;
  for (int i = 0; i < 4; ++i) out.counter |= uint32_t(f[7 + i]) << (8 * i);
  return Result::Ok;
}

bool parseKey(const char* hex, Key& out) {
  if (!hex) return false;
  for (size_t i = 0; i < kKeyLen; ++i) {
    int hi = hexNibble(hex[2 * i]);
    if (hi < 0) return false;
    int lo = hexNibble(hex[2 * i + 1]);
    if (lo < 0) return false;
    out[i] = uint8_t(hi << 4 | lo);
  }
  return hex[2 * kKeyLen] == '\0';
}

}  // namespace link
}  // namespace skeumo
