// parse_com.cpp
#include "parse_com.h"

ComInfo parse_com_payload_preview(const std::vector<uint8_t> &p,
                                  size_t max_preview) {
  ComInfo c;
  c.len = (uint32_t)p.size();
  size_t take = std::min(p.size(), max_preview);
  c.preview.assign((const char *)p.data(), (const char *)p.data() + take);
  for (auto &ch : c.preview) {
    unsigned char u = (unsigned char)ch;
    if (u < 0x20 && ch != '\n' && ch != '\r' && ch != '\t')
      ch = ' ';
  }
  return c;
}
