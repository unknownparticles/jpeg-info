// parse_jfif.cpp
#include "parse_jfif.h"
#include <cstring>

std::optional<JfifInfo>
parse_jfif_from_app0_payload(const std::vector<uint8_t> &p) {
  if (p.size() < 14)
    return std::nullopt;
  if (std::memcmp(p.data(), "JFIF\0", 5) != 0)
    return std::nullopt;
  JfifInfo j;
  j.version = (uint16_t)((p[5] << 8) | p[6]);
  j.units = p[7];
  j.x_density = (uint16_t)((p[8] << 8) | p[9]);
  j.y_density = (uint16_t)((p[10] << 8) | p[11]);
  j.x_thumb = p[12];
  j.y_thumb = p[13];
  return j;
}
