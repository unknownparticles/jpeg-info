// i18n.cpp
#include "i18n.h"
#include <unordered_map>

static const std::unordered_map<std::string, std::string> ZH = {
    {"segments", "分区列表"},
    {"idx", "索引"},
    {"marker", "标记"},
    {"name", "名称"},
    {"moff", "标记偏移"},
    {"poff", "载荷偏移"},
    {"plen", "载荷长度"},
    {"subtype", "APP子类型"},
    {"jfif", "JFIF信息"},
    {"sof", "图像基本信息(SOF)"},
    {"exif", "EXIF信息"},
    {"gps", "GPS信息"},
    {"xmp", "XMP信息"},
    {"icc", "ICC Profile信息"},
    {"adobe", "Adobe(APP14)信息"},
    {"com", "注释(COM)"},
    {"error_parse", "解析失败"},
    {"length_segment", "长度(段)"},
    {"length_effective", "长度(有效内容)"},
    {"padding", "填充"},
    {"truncated_preview", "(截断预览)"},
    {"xml", "XML"},
    {"bytes", "字节"},
};

static const std::unordered_map<std::string, std::string> EN = {
    {"segments", "Segments"},
    {"idx", "Idx"},
    {"marker", "Marker"},
    {"name", "Name"},
    {"moff", "MarkerOff"},
    {"poff", "PayloadOff"},
    {"plen", "PayloadLen"},
    {"subtype", "Subtype"},
    {"jfif", "JFIF"},
    {"sof", "SOF"},
    {"exif", "EXIF"},
    {"gps", "GPS"},
    {"xmp", "XMP"},
    {"icc", "ICC Profile"},
    {"adobe", "Adobe(APP14)"},
    {"com", "COM"},
    {"error_parse", "Parse failed"},
    {"length_segment", "Length (segment)"},
    {"length_effective", "Length (effective XML)"},
    {"padding", "Padding"},
    {"truncated_preview", "(Truncated preview)"},
    {"xml", "XML"},
    {"bytes", "bytes"},
};

std::string I18n::t(const std::string &key) const {
  const auto &m = (lang == Lang::ZH) ? ZH : EN;
  auto it = m.find(key);
  if (it != m.end())
    return it->second;
  return key;
}
