// parse_sof.cpp
#include "parse_sof.h"

static inline uint16_t be16(const uint8_t *p) {
  return (uint16_t)((p[0] << 8) | p[1]);
}

bool is_sof_marker(uint16_t m) {
  if (m < 0xFFC0 || m > 0xFFCF)
    return false;
  if (m == 0xFFC4 || m == 0xFFC8 || m == 0xFFCC)
    return false;
  return true;
}

std::optional<SofInfo> parse_sof_payload(uint16_t marker,
                                         const std::vector<uint8_t> &p) {
  if (!is_sof_marker(marker))
    return std::nullopt;
  if (p.size() < 6)
    return std::nullopt;

  SofInfo s;
  s.marker = marker;
  s.precision = p[0];
  s.height = be16(&p[1]);
  s.width = be16(&p[3]);
  s.components = p[5];

  size_t need = 6 + (size_t)s.components * 3;
  if (p.size() < need)
    return std::nullopt;

  s.comps.reserve(s.components);
  for (size_t i = 0; i < s.components; i++) {
    size_t off = 6 + i * 3;
    SofComponent c;
    c.id = p[off];
    c.h = (p[off + 1] >> 4) & 0x0F;
    c.v = (p[off + 1] & 0x0F);
    c.qt = p[off + 2];
    s.comps.push_back(c);
  }
  return s;
}
