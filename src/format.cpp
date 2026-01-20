// format.cpp
#include "format.h"
#include "jpeg_markers.h"
#include "parse_exif.h"
#include <iomanip>
#include <sstream>

static std::string format_hex(uint64_t val, int width = 0) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::uppercase;
  if (width > 0)
    oss << std::setw(width) << std::setfill('0');
  oss << val;
  return oss.str();
}

void print_segments(std::ostream &os, const std::vector<SegmentIndex> &segs,
                    const I18n &i18n) {
  os << "\n=== " << i18n.t("segments") << " ===\n";
  os << std::setw(4) << i18n.t("idx") << " | " << std::setw(6)
     << i18n.t("marker") << " | " << std::setw(12) << i18n.t("name") << " | "
     << std::setw(12) << i18n.t("moff") << " | " << std::setw(12)
     << i18n.t("poff") << " | " << std::setw(10) << i18n.t("plen") << " | "
     << i18n.t("subtype") << "\n";
  os << std::string(80, '-') << "\n";

  for (size_t i = 0; i < segs.size(); i++) {
    const auto &s = segs[i];
    os << std::setw(4) << i << " | " << std::setw(6) << format_hex(s.marker, 4)
       << " | " << std::setw(12) << marker_name(s.marker) << " | "
       << std::setw(12) << s.marker_offset << " | " << std::setw(12)
       << s.payload_offset << " | " << std::setw(10) << s.payload_len << " | "
       << s.app_subtype << "\n";
  }
  os << "\n";
}

void print_jfif_info(std::ostream &os, const JfifInfo &jfif, const I18n &i18n) {
  os << "=== " << i18n.t("jfif") << " ===\n";
  int major = (jfif.version >> 8) & 0xFF;
  int minor = jfif.version & 0xFF;
  os << "  Version: " << major << "." << std::setfill('0') << std::setw(2)
     << minor << "\n";
  os << "  Units: " << (int)jfif.units;
  if (jfif.units == 0)
    os << " (no units)";
  else if (jfif.units == 1)
    os << " (dots per inch)";
  else if (jfif.units == 2)
    os << " (dots per cm)";
  os << "\n";
  os << "  X-Density: " << jfif.x_density << "\n";
  os << "  Y-Density: " << jfif.y_density << "\n";
  os << "  Thumbnail: " << (int)jfif.x_thumb << "x" << (int)jfif.y_thumb
     << "\n\n";
}

void print_sof_info(std::ostream &os, const SofInfo &sof, const I18n &i18n) {
  os << "=== " << i18n.t("sof") << " ===\n";
  os << "  Marker: " << marker_name(sof.marker) << " ("
     << format_hex(sof.marker, 4) << ")\n";
  os << "  Precision: " << (int)sof.precision << " bits\n";
  os << "  Width x Height: " << sof.width << " x " << sof.height << "\n";
  os << "  Components: " << (int)sof.components << "\n";
  for (size_t i = 0; i < sof.comps.size(); i++) {
    const auto &c = sof.comps[i];
    os << "    [" << i << "] ID=" << (int)c.id << " H=" << (int)c.h
       << " V=" << (int)c.v << " QT=" << (int)c.qt << "\n";
  }
  os << "\n";
}

void print_adobe_info(std::ostream &os, const AdobeInfo &adobe,
                      const I18n &i18n) {
  os << "=== " << i18n.t("adobe") << " ===\n";
  os << "  Version: " << adobe.version << "\n";
  os << "  Flags0: " << format_hex(adobe.flags0, 4) << "\n";
  os << "  Flags1: " << format_hex(adobe.flags1, 4) << "\n";
  os << "  Transform: " << (int)adobe.transform;
  if (adobe.transform == 0)
    os << " (Unknown)";
  else if (adobe.transform == 1)
    os << " (YCbCr)";
  else if (adobe.transform == 2)
    os << " (YCCK)";
  os << "\n\n";
}

void print_com_info(std::ostream &os, const ComInfo &com, const I18n &i18n) {
  os << "=== " << i18n.t("com") << " ===\n";
  os << "  Length: " << com.len << " bytes\n";
  os << "  Preview: \"" << com.preview << "\"\n\n";
}

void print_xmp_info(std::ostream &os, const XmpInfo &xmp, const I18n &i18n) {
  os << "=== " << i18n.t("xmp") << " ===\n";
  os << "  " << i18n.t("length_segment") << ": " << xmp.len << " "
     << i18n.t("bytes") << "\n";
  os << "  " << i18n.t("length_effective") << ": " << xmp.effective_len << " "
     << i18n.t("bytes") << "\n";
  if (xmp.padding_len > 0) {
    os << "  " << i18n.t("padding") << ": " << xmp.padding_len << " "
       << i18n.t("bytes") << "\n";
  }
  if (xmp.truncated) {
    os << "  " << i18n.t("truncated_preview") << "\n";
  }
  os << "  " << i18n.t("xml") << ":\n" << xmp.xml << "\n\n";
}

void print_icc_info(std::ostream &os, const IccProfile &icc, const I18n &i18n) {
  os << "=== " << i18n.t("icc") << " ===\n";
  os << "  Total Length: " << icc.total_len << " bytes\n";
  if (!icc.data.empty()) {
    os << "  Profile Data: " << icc.data.size() << " bytes loaded\n";
  }
  os << "\n";
}

void print_exif_info(std::ostream &os, const ExifResult &exif,
                     const I18n &i18n) {
  os << "=== " << i18n.t("exif") << " ===\n";
  os << "  Endian: " << (exif.endian == Endian::Big ? "Big" : "Little") << "\n";

  if (!exif.ifd0.tags.empty()) {
    os << "  IFD0 Tags:\n";
    for (const auto &[tag, entry] : exif.ifd0.tags) {
      os << "    " << exif_tag_name(tag) << " (0x" << std::hex << std::uppercase
         << std::setw(4) << std::setfill('0') << tag << std::dec
         << "): " << entry.value.text << "\n";
    }
  }

  if (!exif.exif_ifd.tags.empty()) {
    os << "  EXIF IFD Tags:\n";
    for (const auto &[tag, entry] : exif.exif_ifd.tags) {
      os << "    " << exif_tag_name(tag) << " (0x" << std::hex << std::uppercase
         << std::setw(4) << std::setfill('0') << tag << std::dec
         << "): " << entry.value.text << "\n";
    }
  }

  if (!exif.gps_ifd.tags.empty()) {
    os << "  GPS IFD Tags:\n";
    for (const auto &[tag, entry] : exif.gps_ifd.tags) {
      os << "    " << exif_tag_name(tag) << " (0x" << std::hex << std::uppercase
         << std::setw(4) << std::setfill('0') << tag << std::dec
         << "): " << entry.value.text << "\n";
    }
  }

  if (exif.latitude.has_value() || exif.longitude.has_value()) {
    os << "\n=== " << i18n.t("gps") << " ===\n";

    if (exif.latitude.has_value() && exif.latitude->valid) {
      os << "  Latitude: " << std::fixed << std::setprecision(6)
         << exif.latitude->deg << "°\n";
    } else {
      os << "  Latitude: not available\n";
    }

    if (exif.longitude.has_value() && exif.longitude->valid) {
      os << "  Longitude: " << std::fixed << std::setprecision(6)
         << exif.longitude->deg << "°\n";
    } else {
      os << "  Longitude: not available\n";
    }
  }

  os << "\n";
}
