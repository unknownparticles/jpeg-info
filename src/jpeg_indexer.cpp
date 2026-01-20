// jpeg_indexer.cpp
#include "jpeg_indexer.h"
#include "file_reader.h"
#include "jpeg_markers.h"
#include <algorithm>
#include <cstring>

static inline uint16_t be16(const uint8_t *p) {
  return (uint16_t)((p[0] << 8) | p[1]);
}

static bool read_marker(FileReader &r, uint16_t &out_marker) {
  uint8_t b = 0;
  do {
    if (!r.read_u8(b))
      return false;
  } while (b != 0xFF);

  do {
    if (!r.read_u8(b))
      return false;
  } while (b == 0xFF);

  out_marker = (uint16_t)(0xFF00 | b);
  return true;
}

// SOS之后跳过scan data直到下一个marker（处理0xFF00 stuffing）
static bool skip_scan_data_to_next_marker(FileReader &r) {
  while (true) {
    uint8_t b = 0;
    if (!r.read_u8(b))
      return false;
    if (b != 0xFF)
      continue;

    uint64_t ff_pos = r.tell() - 1;
    uint8_t c = 0;
    if (!r.read_u8(c))
      return false;

    if (c == 0x00)
      continue; // stuffed
    if (c == 0xFF) {
      while (c == 0xFF) {
        if (!r.read_u8(c))
          return false;
      }
      if (c == 0x00)
        continue;
    }

    // found marker, rewind
    return r.seek(ff_pos);
  }
}

static std::string detect_app_subtype(uint16_t marker,
                                      const std::vector<uint8_t> &head) {
  if (marker == 0xFFE0) { // APP0
    if (head.size() >= 5 && std::memcmp(head.data(), "JFIF\0", 5) == 0)
      return "JFIF";
    if (head.size() >= 5 && std::memcmp(head.data(), "JFXX\0", 5) == 0)
      return "JFXX";
  }
  if (marker == 0xFFE1) { // APP1
    if (head.size() >= 6 && std::memcmp(head.data(), "Exif\0\0", 6) == 0)
      return "EXIF";
    const char *xmp = "http://ns.adobe.com/xap/1.0/\0";
    size_t xmp_len = std::strlen("http://ns.adobe.com/xap/1.0/") + 1;
    if (head.size() >= xmp_len && std::memcmp(head.data(), xmp, xmp_len) == 0)
      return "XMP";
  }
  if (marker == 0xFFE2) { // APP2
    const char *icc = "ICC_PROFILE\0";
    size_t icc_len = std::strlen("ICC_PROFILE") + 1;
    if (head.size() >= icc_len && std::memcmp(head.data(), icc, icc_len) == 0)
      return "ICC";
  }
  if (marker == 0xFFEE) { // APP14
    if (head.size() >= 5 && std::memcmp(head.data(), "Adobe", 5) == 0)
      return "Adobe";
  }
  return "Unknown";
}

JpegIndexResult build_jpeg_index(const std::string &path,
                                 const IndexOptions &opt) {
  JpegIndexResult out;
  FileReader r(path.c_str());
  if (!r.ok())
    return out;

  uint8_t soi[2] = {0};
  if (!r.read_bytes(soi, 2))
    return out;
  if (!(soi[0] == 0xFF && soi[1] == 0xD8))
    return out;

  out.segments.push_back({0xFFD8, 0, 0, 0, 0, ""});

  while (true) {
    uint64_t marker_off = r.tell();
    uint16_t marker = 0;
    if (!read_marker(r, marker))
      break;

    SegmentIndex seg;
    seg.marker = marker;
    seg.marker_offset = marker_off;

    if (marker == 0xFFD9) { // EOI
      out.segments.push_back(seg);
      break;
    }

    if (!marker_has_length(marker)) {
      out.segments.push_back(seg);
      continue;
    }

    uint8_t lenbuf[2];
    if (!r.read_bytes(lenbuf, 2))
      break;
    uint16_t seglen = be16(lenbuf);
    if (seglen < 2)
      break;

    seg.total_len = seglen;
    seg.payload_len = (uint32_t)(seglen - 2);
    seg.payload_offset = r.tell();

    // APP peek for subtype
    if (marker >= 0xFFE0 && marker <= 0xFFEF) {
      size_t peek = std::min<size_t>(opt.app_peek_bytes, seg.payload_len);
      std::vector<uint8_t> head;
      if (!r.read_bytes(head, peek))
        break;
      seg.app_subtype = detect_app_subtype(marker, head);
      r.seek(seg.payload_offset + seg.payload_len);
      out.segments.push_back(seg);
      continue;
    }

    // SOS: 跳过压缩数据到 EOI
    if (marker == 0xFFDA) {
      out.segments.push_back(seg);
      if (!r.seek(seg.payload_offset + seg.payload_len))
        break;
      if (!skip_scan_data_to_next_marker(r))
        break;

      // 读取 EOI
      uint64_t eoi_off = r.tell();
      uint16_t eoi_marker = 0;
      if (read_marker(r, eoi_marker) && eoi_marker == 0xFFD9) {
        SegmentIndex eoi_seg;
        eoi_seg.marker = 0xFFD9;
        eoi_seg.marker_offset = eoi_off;
        out.segments.push_back(eoi_seg);
      }
      break;
    }

    // default skip
    if (!r.seek(seg.payload_offset + seg.payload_len))
      break;
    out.segments.push_back(seg);
  }

  return out;
}

bool load_segment_payload(const std::string &path, const SegmentIndex &seg,
                          std::vector<uint8_t> &out) {
  FileReader r(path.c_str());
  if (!r.ok())
    return false;
  if (!r.seek(seg.payload_offset))
    return false;
  return r.read_bytes(out, seg.payload_len);
}
