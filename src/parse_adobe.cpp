// parse_adobe.cpp
#include "parse_adobe.h"
#include <cstring>

static inline uint16_t be16(const uint8_t *p) {
  return (uint16_t)((p[0] << 8) | p[1]);
}

std::optional<AdobeInfo>
parse_adobe_app14_payload(const std::vector<uint8_t> &p) {
  // "Adobe" (5) + version(2) + flags0(2) + flags1(2) + transform(1) = 12 bytes
  // minimum after sig
  if (p.size() < 5 + 2 + 2 + 2 + 1)
    return std::nullopt;
  if (std::memcmp(p.data(), "Adobe", 5) != 0)
    return std::nullopt;
  AdobeInfo a;
  a.version = be16(&p[5]);
  a.flags0 = be16(&p[7]);
  a.flags1 = be16(&p[9]);
  a.transform = p[11];
  return a;
}
