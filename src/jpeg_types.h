#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct SegmentIndex {
  uint16_t marker = 0;
  uint64_t marker_offset = 0;  // marker 0xFF?? 起始位置
  uint64_t payload_offset = 0; // payload 起始（跳过len字段）
  uint32_t payload_len = 0;    // payload长度
  uint32_t total_len = 0;      // 2+payload_len（有len字段时）
  std::string app_subtype;     // "JFIF"/"EXIF"/"XMP"/"ICC"/"Adobe"/"Unknown"...
};

struct JfifInfo {
  uint16_t version = 0;
  uint8_t units = 0;
  uint16_t x_density = 0;
  uint16_t y_density = 0;
  uint8_t x_thumb = 0;
  uint8_t y_thumb = 0;
};

struct SofComponent {
  uint8_t id = 0;
  uint8_t h = 0;
  uint8_t v = 0;
  uint8_t qt = 0;
};

struct SofInfo {
  uint16_t marker = 0;
  uint8_t precision = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  uint8_t components = 0;
  std::vector<SofComponent> comps;
};

struct AdobeInfo {
  uint16_t version = 0;
  uint16_t flags0 = 0;
  uint16_t flags1 = 0;
  uint8_t transform = 0; // 0 unknown, 1 YCbCr, 2 YCCK
};

struct ComInfo {
  uint32_t len = 0;
  std::string preview; // 预览（可限制）
};

struct XmpInfo {
  uint32_t len = 0;           // segment payload 总长度
  uint32_t effective_len = 0; // 有效 XML 内容长度（不含 padding）
  uint32_t padding_len = 0;   // padding 长度
  std::string xml;            // 有效 XML 内容（不含 padding）
  bool truncated = false;     // 是否截断显示
};

struct IccProfile {
  uint32_t total_len = 0;
  std::vector<uint8_t> data; // 拼接后的profile（可选）
};

enum class Endian { Little, Big };

struct ExifValue {
  // 为了可读性：用“格式化后的文本”承载（不做复杂variant）
  std::string text;
};

struct ExifTag {
  uint16_t tag = 0;
  uint16_t type = 0;
  uint32_t count = 0;
  uint32_t value_or_offset = 0;
  ExifValue value;
};

struct ExifIfd {
  std::map<uint16_t, ExifTag> tags;
};

struct GpsCoord {
  double deg = 0.0; // 十进制度
  bool valid = false;
};

struct ExifResult {
  Endian endian = Endian::Little;
  ExifIfd ifd0;
  ExifIfd exif_ifd;
  ExifIfd gps_ifd;
  std::optional<GpsCoord> latitude;
  std::optional<GpsCoord> longitude;
};
