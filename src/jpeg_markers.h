#pragma once
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

inline bool marker_has_length(uint16_t m) {
  if (m == 0xFFD8 || m == 0xFFD9)
    return false; // SOI/EOI
  if (m >= 0xFFD0 && m <= 0xFFD7)
    return false; // RSTn
  if (m == 0xFF01)
    return false; // TEM
  return true;
}

inline std::string marker_name(uint16_t m) {
  switch (m) {
  case 0xFFD8:
    return "SOI";
  case 0xFFD9:
    return "EOI";
  case 0xFFDA:
    return "SOS";
  case 0xFFFE:
    return "COM";
  case 0xFFDB:
    return "DQT";
  case 0xFFC4:
    return "DHT";
  case 0xFFDD:
    return "DRI";
  default:
    if (m >= 0xFFE0 && m <= 0xFFEF) {
      int n = (int)(m - 0xFFE0);
      return "APP" + std::to_string(n);
    }
    if ((m >= 0xFFC0 && m <= 0xFFCF) && m != 0xFFC4 && m != 0xFFC8 &&
        m != 0xFFCC) {
      std::ostringstream oss;
      oss << "SOF(0x" << std::hex << std::uppercase << (m & 0xFF) << ")";
      return oss.str();
    }
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << std::setw(4)
        << std::setfill('0') << m;
    return oss.str();
  }
}
