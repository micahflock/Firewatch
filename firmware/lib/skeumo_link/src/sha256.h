#pragma once
// SHA-256 (FIPS 180-4) and HMAC-SHA256 (RFC 2104). Small and dependency-free, so the
// link frame is computed identically on every chip and on the host test machine.

#include <cstddef>
#include <cstdint>

namespace skeumo {

class Sha256 {
 public:
  static constexpr size_t kDigestLen = 32;
  static constexpr size_t kBlockLen = 64;

  Sha256() { reset(); }
  void reset();
  void update(const uint8_t* data, size_t len);
  void finish(uint8_t out[kDigestLen]);

 private:
  void block(const uint8_t* p);

  uint32_t h_[8];
  uint8_t buf_[kBlockLen];
  size_t bufLen_;
  uint64_t totalLen_;
};

void hmacSha256(const uint8_t* key, size_t keyLen, const uint8_t* msg, size_t msgLen,
                uint8_t out[Sha256::kDigestLen]);

}  // namespace skeumo
