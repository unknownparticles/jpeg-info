#include "parse_exif.h"
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

static inline uint16_t rd16(const uint8_t *p, Endian e) {
  return (e == Endian::Little) ? (uint16_t)(p[0] | (p[1] << 8))
                               : (uint16_t)((p[0] << 8) | p[1]);
}
static inline uint32_t rd32(const uint8_t *p, Endian e) {
  if (e == Endian::Little)
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
  return (uint32_t)((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static inline uint32_t tiff_type_size(uint16_t t) {
  switch (t) {
  case 1:
    return 1; // BYTE
  case 2:
    return 1; // ASCII
  case 3:
    return 2; // SHORT
  case 4:
    return 4; // LONG
  case 5:
    return 8; // RATIONAL
  case 7:
    return 1; // UNDEFINED
  case 9:
    return 4; // SLONG
  case 10:
    return 8; // SRATIONAL
  default:
    return 0;
  }
}

static std::string bytes_to_ascii(const uint8_t *src, size_t n) {
  std::string s((const char *)src, (const char *)src + n);
  // trim at first '\0'
  auto pos = s.find('\0');
  if (pos != std::string::npos)
    s.resize(pos);
  for (auto &ch : s) {
    unsigned char u = (unsigned char)ch;
    if (u < 0x20 && ch != '\n' && ch != '\r' && ch != '\t')
      ch = '?';
  }
  return s;
}

static std::string format_rational(uint32_t num, uint32_t den) {
  std::ostringstream oss;
  if (den == 0) {
    oss << num << "/0";
    return oss.str();
  }
  // 显示分数 + 小数近似（便于阅读）
  double v = (double)num / (double)den;
  oss << num << "/" << den << " (~" << std::fixed << std::setprecision(6) << v
      << ")";
  return oss.str();
}

static bool read_value_ptr(const uint8_t *tiff, size_t tiff_len, Endian e,
                           const uint8_t *entry, uint16_t type, uint32_t count,
                           const uint8_t *&out_ptr, uint64_t &out_bytes) {
  uint32_t unit = tiff_type_size(type);
  if (unit == 0)
    return false;
  out_bytes = (uint64_t)unit * count;

  // value_or_offset field
  const uint8_t *val_field = entry + 8;

  if (out_bytes <= 4) {
    out_ptr = val_field; // inline
    return true;
  }
  uint32_t off = rd32(val_field, e);
  if ((uint64_t)off + out_bytes > tiff_len)
    return false;
  out_ptr = tiff + off;
  return true;
}

// EXIF 枚举值映射函数
static std::string map_orientation(uint16_t val) {
  switch (val) {
  case 1:
    return "Horizontal (normal)";
  case 2:
    return "Mirror horizontal";
  case 3:
    return "Rotate 180";
  case 4:
    return "Mirror vertical";
  case 5:
    return "Mirror horizontal and rotate 270 CW";
  case 6:
    return "Rotate 90 CW";
  case 7:
    return "Mirror horizontal and rotate 90 CW";
  case 8:
    return "Rotate 270 CW";
  default:
    return std::to_string(val);
  }
}

static std::string map_resolution_unit(uint16_t val) {
  switch (val) {
  case 1:
    return "None";
  case 2:
    return "inches";
  case 3:
    return "cm";
  default:
    return std::to_string(val);
  }
}

static std::string map_exposure_program(uint16_t val) {
  switch (val) {
  case 0:
    return "Not Defined";
  case 1:
    return "Manual";
  case 2:
    return "Program AE";
  case 3:
    return "Aperture-priority AE";
  case 4:
    return "Shutter speed priority AE";
  case 5:
    return "Creative (Slow speed)";
  case 6:
    return "Action (High speed)";
  case 7:
    return "Portrait";
  case 8:
    return "Landscape";
  default:
    return std::to_string(val);
  }
}

static std::string map_metering_mode(uint16_t val) {
  switch (val) {
  case 0:
    return "Unknown";
  case 1:
    return "Average";
  case 2:
    return "Center-weighted average";
  case 3:
    return "Spot";
  case 4:
    return "Multi-spot";
  case 5:
    return "Multi-segment";
  case 6:
    return "Partial";
  case 255:
    return "Other";
  default:
    return std::to_string(val);
  }
}

static std::string map_flash(uint16_t val) {
  std::ostringstream oss;
  bool fired = (val & 0x01) != 0;

  if (!fired) {
    oss << "Off, Did not fire";
  } else {
    oss << "Fired";

    // Return light detection
    uint8_t return_light = (val >> 1) & 0x03;
    if (return_light == 2)
      oss << ", Return detected";
    else if (return_light == 3)
      oss << ", Return not detected";

    // Flash mode
    uint8_t mode = (val >> 3) & 0x03;
    if (mode == 1)
      oss << ", Compulsory flash firing";
    else if (mode == 2)
      oss << ", Compulsory flash suppression";
    else if (mode == 3)
      oss << ", Auto mode";

    // Red-eye
    if (val & 0x40)
      oss << ", Red-eye reduction";
  }

  return oss.str();
}

static std::string map_color_space(uint16_t val) {
  switch (val) {
  case 1:
    return "sRGB";
  case 65535:
    return "Uncalibrated";
  default:
    return std::to_string(val);
  }
}

static std::string map_sensing_method(uint16_t val) {
  switch (val) {
  case 1:
    return "Not defined";
  case 2:
    return "One-chip color area";
  case 3:
    return "Two-chip color area";
  case 4:
    return "Three-chip color area";
  case 5:
    return "Color sequential area";
  case 7:
    return "Trilinear";
  case 8:
    return "Color sequential linear";
  default:
    return std::to_string(val);
  }
}

static std::string map_scene_capture_type(uint16_t val) {
  switch (val) {
  case 0:
    return "Standard";
  case 1:
    return "Landscape";
  case 2:
    return "Portrait";
  case 3:
    return "Night";
  default:
    return std::to_string(val);
  }
}

static std::string map_light_source(uint16_t val) {
  switch (val) {
  case 0:
    return "Unknown";
  case 1:
    return "Daylight";
  case 2:
    return "Fluorescent";
  case 3:
    return "Tungsten (incandescent)";
  case 4:
    return "Flash";
  case 9:
    return "Fine weather";
  case 10:
    return "Cloudy";
  case 11:
    return "Shade";
  case 17:
    return "Standard light A";
  case 18:
    return "Standard light B";
  case 19:
    return "Standard light C";
  case 20:
    return "D55";
  case 21:
    return "D65";
  case 22:
    return "D75";
  case 23:
    return "D50";
  case 24:
    return "ISO studio tungsten";
  case 255:
    return "Other";
  default:
    return std::to_string(val);
  }
}

static std::string map_exposure_mode(uint16_t val) {
  switch (val) {
  case 0:
    return "Auto";
  case 1:
    return "Manual";
  case 2:
    return "Auto bracket";
  default:
    return std::to_string(val);
  }
}

static std::string map_white_balance(uint16_t val) {
  switch (val) {
  case 0:
    return "Auto";
  case 1:
    return "Manual";
  default:
    return std::to_string(val);
  }
}

static std::string map_custom_rendered(uint16_t val) {
  switch (val) {
  case 0:
    return "Normal";
  case 1:
    return "Custom";
  default:
    return std::to_string(val);
  }
}

static std::string map_contrast_saturation_sharpness(uint16_t val) {
  switch (val) {
  case 0:
    return "Normal";
  case 1:
    return "Low";
  case 2:
    return "High";
  default:
    return std::to_string(val);
  }
}

static std::string format_value(const uint8_t *tiff, size_t tiff_len, Endian e,
                                const uint8_t *entry, uint16_t type,
                                uint32_t count, uint16_t tag) {
  const uint8_t *ptr = nullptr;
  uint64_t bytes = 0;
  if (!read_value_ptr(tiff, tiff_len, e, entry, type, count, ptr, bytes))
    return "";

  std::ostringstream oss;
  switch (type) {
  case 2: { // ASCII
    // ASCII 字段可能不以 \0 结尾（如 ExifVersion, FlashpixVersion）
    // 注意：count<=4 时，数据内联在 value_or_offset 字段中（ptr 指向它）
    std::string s;

    // 读取所有字节
    for (size_t i = 0; i < (size_t)bytes; i++) {
      char ch = (char)ptr[i];
      if (ch == '\0')
        break; // 遇到 null 终止符就停止
      s += ch;
    }

    // 清理不可打印字符
    for (auto &ch : s) {
      unsigned char u = (unsigned char)ch;
      if (u < 0x20 && ch != '\n' && ch != '\r' && ch != '\t')
        ch = '?';
    }

    // GPS Ref 字段：如果为空，显示 "Unknown"
    if (s.empty() && (tag == 0x0001 || tag == 0x0003 || tag == 0x000C ||
                      tag == 0x0010 || tag == 0x0017)) {
      return "Unknown";
    }

    return s;
  }
  case 1: { // BYTE
    // 对于某些特殊 tag（如 GPSVersionID），显示为点分十进制
    if (tag == 0x0000 && count == 4) { // GPSVersionID
      oss << (int)ptr[0] << "." << (int)ptr[1] << "." << (int)ptr[2] << "."
          << (int)ptr[3];
      return oss.str();
    }
    // 其他 BYTE 数组：显示为十六进制预览
    size_t take = (size_t)std::min<uint64_t>(bytes, 16);
    oss << "0x";
    for (size_t i = 0; i < take; i++) {
      oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
          << (int)ptr[i];
    }
    if (bytes > take)
      oss << "... (" << bytes << " bytes)";
    return oss.str();
  }
  case 3: { // SHORT
    // 对特定 tag 应用枚举值映射
    if (count == 1) {
      uint16_t v = rd16(ptr, e);

      // 应用枚举值映射
      switch (tag) {
      case 0x0112: // Orientation
        return map_orientation(v);
      case 0x0128: // ResolutionUnit
        return map_resolution_unit(v);
      case 0x8822: // ExposureProgram
        return map_exposure_program(v);
      case 0x9207: // MeteringMode
        return map_metering_mode(v);
      case 0x9209: // Flash
        return map_flash(v);
      case 0xA001: // ColorSpace
        return map_color_space(v);
      case 0xA217: // SensingMethod
        return map_sensing_method(v);
      case 0xA406: // SceneCaptureType
        return map_scene_capture_type(v);
      case 0x9208: // LightSource
        return map_light_source(v);
      case 0xA402: // ExposureMode
        return map_exposure_mode(v);
      case 0xA403: // WhiteBalance
        return map_white_balance(v);
      case 0xA401: // CustomRendered
        return map_custom_rendered(v);
      case 0xA408: // Contrast
      case 0xA409: // Saturation
      case 0xA40A: // Sharpness
        return map_contrast_saturation_sharpness(v);
      default:
        // 对于其他 tag，显示原始值
        return std::to_string(v);
      }
    }

    // 对于数组，显示前几个
    uint32_t n = std::min<uint32_t>(count, 8);
    for (uint32_t i = 0; i < n; i++) {
      uint16_t v = rd16(ptr + i * 2, e);
      if (i)
        oss << ", ";
      oss << v;
    }
    if (count > n)
      oss << ", ...";
    return oss.str();
  }
  case 4: { // LONG
    uint32_t n = std::min<uint32_t>(count, 8);
    for (uint32_t i = 0; i < n; i++) {
      uint32_t v = rd32(ptr + i * 4, e);
      if (i)
        oss << ", ";
      oss << v;
    }
    if (count > n)
      oss << ", ...";
    return oss.str();
  }
  case 5: { // RATIONAL
    uint32_t n = std::min<uint32_t>(count, 4);
    for (uint32_t i = 0; i < n; i++) {
      uint32_t num = rd32(ptr + i * 8 + 0, e);
      uint32_t den = rd32(ptr + i * 8 + 4, e);
      if (i)
        oss << ", ";
      oss << format_rational(num, den);
    }
    if (count > n)
      oss << ", ...";
    return oss.str();
  }
  case 9: { // SLONG (signed long)
    uint32_t n = std::min<uint32_t>(count, 8);
    for (uint32_t i = 0; i < n; i++) {
      int32_t v = (int32_t)rd32(ptr + i * 4, e);
      if (i)
        oss << ", ";
      oss << v;
    }
    if (count > n)
      oss << ", ...";
    return oss.str();
  }
  case 10: { // SRATIONAL (signed rational)
    uint32_t n = std::min<uint32_t>(count, 4);
    for (uint32_t i = 0; i < n; i++) {
      int32_t num = (int32_t)rd32(ptr + i * 8 + 0, e);
      int32_t den = (int32_t)rd32(ptr + i * 8 + 4, e);
      if (i)
        oss << ", ";
      if (den == 0) {
        oss << num << "/0";
      } else {
        double v = (double)num / (double)den;
        oss << num << "/" << den << " (~" << std::fixed << std::setprecision(6)
            << v << ")";
      }
    }
    if (count > n)
      oss << ", ...";
    return oss.str();
  }
  case 7: { // UNDEFINED
    // 对于 ComponentsConfiguration (0x9101)，映射为可读名称
    if (tag == 0x9101 && count == 4) {
      auto comp_name = [](uint8_t c) -> std::string {
        switch (c) {
        case 0:
          return "-";
        case 1:
          return "Y";
        case 2:
          return "Cb";
        case 3:
          return "Cr";
        case 4:
          return "R";
        case 5:
          return "G";
        case 6:
          return "B";
        default:
          return "?";
        }
      };
      oss << comp_name(ptr[0]) << ", " << comp_name(ptr[1]) << ", "
          << comp_name(ptr[2]) << ", " << comp_name(ptr[3]);
      return oss.str();
    }
    // 对于 UserComment (0x9286)，前 8 字节是编码标识
    if (tag == 0x9286 && bytes > 8) {
      std::string encoding((const char *)ptr, 8);
      // 常见编码: "ASCII\0\0\0", "JIS\0\0\0\0\0", "UNICODE\0"
      if (encoding.find("ASCII") == 0) {
        return "ASCII: " + bytes_to_ascii(ptr + 8, (size_t)(bytes - 8));
      } else if (encoding.find("UNICODE") == 0) {
        return "UNICODE: (binary data, " + std::to_string(bytes - 8) +
               " bytes)";
      } else {
        return "Unknown encoding: " + encoding;
      }
    }
    // 对于 MakerNote (0x927C)，只显示摘要
    if (tag == 0x927C) {
      return "MakerNote (" + std::to_string(bytes) + " bytes, vendor-specific)";
    }
    // 对于 ExifVersion/FlashpixVersion，如果是 4 字节，尝试作为 ASCII
    if ((tag == 0x9000 || tag == 0xA000) && bytes == 4) {
      return bytes_to_ascii(ptr, 4);
    }
    // 其他 UNDEFINED：十六进制预览
    size_t take = (size_t)std::min<uint64_t>(bytes, 16);
    oss << "0x";
    for (size_t i = 0; i < take; i++) {
      oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
          << (int)ptr[i];
    }
    if (bytes > take)
      oss << "... (" << bytes << " bytes)";
    return oss.str();
  }
  default:
    return "";
  }
}

static bool parse_ifd(const uint8_t *tiff, size_t tiff_len, Endian e,
                      uint32_t ifd_off, ExifIfd &out) {
  if ((uint64_t)ifd_off + 2 > tiff_len)
    return false;
  uint16_t n = rd16(tiff + ifd_off, e);
  uint64_t base = (uint64_t)ifd_off + 2;
  uint64_t need = base + (uint64_t)n * 12 + 4;
  if (need > tiff_len)
    return false;

  for (uint16_t i = 0; i < n; i++) {
    const uint8_t *ent = tiff + base + (uint64_t)i * 12;
    ExifTag tg;
    tg.tag = rd16(ent + 0, e);
    tg.type = rd16(ent + 2, e);
    tg.count = rd32(ent + 4, e);
    tg.value_or_offset = rd32(ent + 8, e);
    tg.value.text =
        format_value(tiff, tiff_len, e, ent, tg.type, tg.count, tg.tag);
    out.tags[tg.tag] = tg;
  }
  return true;
}

static std::optional<double>
parse_gps_rational_triplet_deg(const uint8_t *tiff, size_t tiff_len, Endian e,
                               const ExifTag &tag) {
  // GPSLatitude/GPSLongitude are RATIONAL[3]
  if (tag.type != 5 || tag.count < 3)
    return std::nullopt;
  uint32_t unit = tiff_type_size(tag.type);
  uint64_t bytes = (uint64_t)unit * tag.count;
  uint32_t off = tag.value_or_offset;
  if (bytes <= 4) {
    // inline not expected for RATIONAL[3]
    return std::nullopt;
  }
  if ((uint64_t)off + 24 > tiff_len)
    return std::nullopt;
  const uint8_t *p = tiff + off;
  auto rat = [&](int idx) -> double {
    uint32_t num = rd32(p + idx * 8 + 0, e);
    uint32_t den = rd32(p + idx * 8 + 4, e);
    if (den == 0)
      return 0.0;
    return (double)num / (double)den;
  };
  double d = rat(0), m = rat(1), s = rat(2);
  return d + (m / 60.0) + (s / 3600.0);
}

std::optional<ExifResult>
parse_exif_from_app1_payload(const std::vector<uint8_t> &payload) {
  if (payload.size() < 6 + 8)
    return std::nullopt;
  if (std::memcmp(payload.data(), "Exif\0\0", 6) != 0)
    return std::nullopt;

  const uint8_t *tiff = payload.data() + 6;
  size_t tiff_len = payload.size() - 6;

  if (tiff_len < 8)
    return std::nullopt;
  Endian e;
  if (tiff[0] == 'I' && tiff[1] == 'I')
    e = Endian::Little;
  else if (tiff[0] == 'M' && tiff[1] == 'M')
    e = Endian::Big;
  else
    return std::nullopt;

  uint16_t magic = rd16(tiff + 2, e);
  if (magic != 0x2A)
    return std::nullopt;

  uint32_t ifd0_off = rd32(tiff + 4, e);
  if (ifd0_off >= tiff_len)
    return std::nullopt;

  ExifResult res;
  res.endian = e;

  if (!parse_ifd(tiff, tiff_len, e, ifd0_off, res.ifd0))
    return std::nullopt;

  // pointer tags: 0x8769 ExifIFDPointer, 0x8825 GPSInfoIFDPointer
  auto it_exif_ptr = res.ifd0.tags.find(0x8769);
  if (it_exif_ptr != res.ifd0.tags.end()) {
    uint32_t exif_off = it_exif_ptr->second.value_or_offset;
    if (exif_off < tiff_len)
      parse_ifd(tiff, tiff_len, e, exif_off, res.exif_ifd);
  }
  auto it_gps_ptr = res.ifd0.tags.find(0x8825);
  if (it_gps_ptr != res.ifd0.tags.end()) {
    uint32_t gps_off = it_gps_ptr->second.value_or_offset;
    if (gps_off < tiff_len)
      parse_ifd(tiff, tiff_len, e, gps_off, res.gps_ifd);
  }

  // GPS decode to decimal degrees if possible
  // GPS tags:
  // 0x0001 GPSLatitudeRef ("N"/"S")
  // 0x0002 GPSLatitude (RATIONAL[3])
  // 0x0003 GPSLongitudeRef ("E"/"W")
  // 0x0004 GPSLongitude (RATIONAL[3])
  if (!res.gps_ifd.tags.empty()) {
    auto lat_ref_it = res.gps_ifd.tags.find(0x0001);
    auto lat_it = res.gps_ifd.tags.find(0x0002);
    auto lon_ref_it = res.gps_ifd.tags.find(0x0003);
    auto lon_it = res.gps_ifd.tags.find(0x0004);

    if (lat_it != res.gps_ifd.tags.end() && lon_it != res.gps_ifd.tags.end()) {
      auto lat =
          parse_gps_rational_triplet_deg(tiff, tiff_len, e, lat_it->second);
      auto lon =
          parse_gps_rational_triplet_deg(tiff, tiff_len, e, lon_it->second);

      // 检查 GPS 数据是否有效
      bool has_valid_gps = false;
      if (lat && lon) {
        double latv = *lat;
        double lonv = *lon;

        // 检查是否有有效的参考方向
        bool has_lat_ref = (lat_ref_it != res.gps_ifd.tags.end() &&
                            !lat_ref_it->second.value.text.empty());
        bool has_lon_ref = (lon_ref_it != res.gps_ifd.tags.end() &&
                            !lon_ref_it->second.value.text.empty());

        // 检查坐标是否非零（全零通常表示没有 GPS 数据）
        bool is_nonzero = (std::abs(latv) > 0.0001 || std::abs(lonv) > 0.0001);

        if (has_lat_ref && has_lon_ref && is_nonzero) {
          if (lat_ref_it->second.value.text[0] == 'S')
            latv = -latv;
          if (lon_ref_it->second.value.text[0] == 'W')
            lonv = -lonv;

          res.latitude = GpsCoord{latv, true};
          res.longitude = GpsCoord{lonv, true};
          has_valid_gps = true;
        }
      }

      // 如果没有有效的 GPS 数据，标记为 invalid
      if (!has_valid_gps) {
        res.latitude = GpsCoord{0.0, false};
        res.longitude = GpsCoord{0.0, false};
      }
    }
  }

  return res;
}

std::string exif_tag_name(uint16_t tag) {
  switch (tag) {
  // IFD0 常用标签
  case 0x010E:
    return "ImageDescription";
  case 0x010F:
    return "Make";
  case 0x0110:
    return "Model";
  case 0x0112:
    return "Orientation";
  case 0x011A:
    return "XResolution";
  case 0x011B:
    return "YResolution";
  case 0x0128:
    return "ResolutionUnit";
  case 0x0131:
    return "Software";
  case 0x0132:
    return "DateTime";
  case 0x013B:
    return "Artist";
  case 0x013E:
    return "WhitePoint";
  case 0x013F:
    return "PrimaryChromaticities";
  case 0x0211:
    return "YCbCrCoefficients";
  case 0x0212:
    return "YCbCrSubSampling";
  case 0x0213:
    return "YCbCrPositioning";
  case 0x0214:
    return "ReferenceBlackWhite";
  case 0x8298:
    return "Copyright";
  case 0x8769:
    return "ExifIFDPointer";
  case 0x8825:
    return "GPSInfoIFDPointer";

  // EXIF IFD 常用标签
  case 0x829A:
    return "ExposureTime";
  case 0x829D:
    return "FNumber";
  case 0x8822:
    return "ExposureProgram";
  case 0x8824:
    return "SpectralSensitivity";
  case 0x8827:
    return "ISO";
  case 0x8828:
    return "OECF";
  case 0x8830:
    return "SensitivityType";
  case 0x8832:
    return "RecommendedExposureIndex";
  case 0x9000:
    return "ExifVersion";
  case 0x9003:
    return "DateTimeOriginal";
  case 0x9004:
    return "DateTimeDigitized";
  case 0x9010:
    return "OffsetTime";
  case 0x9011:
    return "OffsetTimeOriginal";
  case 0x9012:
    return "OffsetTimeDigitized";
  case 0x9101:
    return "ComponentsConfiguration";
  case 0x9102:
    return "CompressedBitsPerPixel";
  case 0x9201:
    return "ShutterSpeedValue";
  case 0x9202:
    return "ApertureValue";
  case 0x9203:
    return "BrightnessValue";
  case 0x9204:
    return "ExposureBiasValue";
  case 0x9205:
    return "MaxApertureValue";
  case 0x9206:
    return "SubjectDistance";
  case 0x9207:
    return "MeteringMode";
  case 0x9208:
    return "LightSource";
  case 0x9209:
    return "Flash";
  case 0x920A:
    return "FocalLength";
  case 0x9214:
    return "SubjectArea";
  case 0x927C:
    return "MakerNote";
  case 0x9286:
    return "UserComment";
  case 0x9290:
    return "SubSecTime";
  case 0x9291:
    return "SubSecTimeOriginal";
  case 0x9292:
    return "SubSecTimeDigitized";
  case 0xA000:
    return "FlashpixVersion";
  case 0xA001:
    return "ColorSpace";
  case 0xA002:
    return "PixelXDimension";
  case 0xA003:
    return "PixelYDimension";
  case 0xA004:
    return "RelatedSoundFile";
  case 0xA005:
    return "InteroperabilityIFDPointer";
  case 0xA20B:
    return "FlashEnergy";
  case 0xA20C:
    return "SpatialFrequencyResponse";
  case 0xA20E:
    return "FocalPlaneXResolution";
  case 0xA20F:
    return "FocalPlaneYResolution";
  case 0xA210:
    return "FocalPlaneResolutionUnit";
  case 0xA214:
    return "SubjectLocation";
  case 0xA215:
    return "ExposureIndex";
  case 0xA217:
    return "SensingMethod";
  case 0xA300:
    return "FileSource";
  case 0xA301:
    return "SceneType";
  case 0xA302:
    return "CFAPattern";
  case 0xA401:
    return "CustomRendered";
  case 0xA402:
    return "ExposureMode";
  case 0xA403:
    return "WhiteBalance";
  case 0xA404:
    return "DigitalZoomRatio";
  case 0xA405:
    return "FocalLengthIn35mmFilm";
  case 0xA406:
    return "SceneCaptureType";
  case 0xA407:
    return "GainControl";
  case 0xA408:
    return "Contrast";
  case 0xA409:
    return "Saturation";
  case 0xA40A:
    return "Sharpness";
  case 0xA40B:
    return "DeviceSettingDescription";
  case 0xA40C:
    return "SubjectDistanceRange";
  case 0xA420:
    return "ImageUniqueID";
  case 0xA430:
    return "CameraOwnerName";
  case 0xA431:
    return "BodySerialNumber";
  case 0xA432:
    return "LensSpecification";
  case 0xA433:
    return "LensMake";
  case 0xA434:
    return "LensModel";
  case 0xA435:
    return "LensSerialNumber";

  // GPS IFD 常用标签
  case 0x0000:
    return "GPSVersionID";
  case 0x0001:
    return "GPSLatitudeRef";
  case 0x0002:
    return "GPSLatitude";
  case 0x0003:
    return "GPSLongitudeRef";
  case 0x0004:
    return "GPSLongitude";
  case 0x0005:
    return "GPSAltitudeRef";
  case 0x0006:
    return "GPSAltitude";
  case 0x0007:
    return "GPSTimeStamp";
  case 0x0008:
    return "GPSSatellites";
  case 0x0009:
    return "GPSStatus";
  case 0x000A:
    return "GPSMeasureMode";
  case 0x000B:
    return "GPSDOP";
  case 0x000C:
    return "GPSSpeedRef";
  case 0x000D:
    return "GPSSpeed";
  case 0x000E:
    return "GPSTrackRef";
  case 0x000F:
    return "GPSTrack";
  case 0x0010:
    return "GPSImgDirectionRef";
  case 0x0011:
    return "GPSImgDirection";
  case 0x0012:
    return "GPSMapDatum";
  case 0x0013:
    return "GPSDestLatitudeRef";
  case 0x0014:
    return "GPSDestLatitude";
  case 0x0015:
    return "GPSDestLongitudeRef";
  case 0x0016:
    return "GPSDestLongitude";
  case 0x0017:
    return "GPSDestBearingRef";
  case 0x0018:
    return "GPSDestBearing";
  case 0x0019:
    return "GPSDestDistanceRef";
  case 0x001A:
    return "GPSDestDistance";
  case 0x001B:
    return "GPSProcessingMethod";
  case 0x001C:
    return "GPSAreaInformation";
  case 0x001D:
    return "GPSDateStamp";
  case 0x001E:
    return "GPSDifferential";
  case 0x001F:
    return "GPSHPositioningError";

  default:
    return "";
  }
}
