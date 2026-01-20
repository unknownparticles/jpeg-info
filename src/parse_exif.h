#pragma once
#include "jpeg_types.h"
#include <optional>
#include <vector>

std::optional<ExifResult>
parse_exif_from_app1_payload(const std::vector<uint8_t> &payload);

// 常用tag名（可扩展）
std::string exif_tag_name(uint16_t tag);
