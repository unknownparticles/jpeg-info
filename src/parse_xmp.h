// parse_xmp.h
#pragma once
#include "jpeg_types.h"
#include <optional>
#include <vector>

std::optional<XmpInfo>
parse_xmp_from_app1_payload(const std::vector<uint8_t> &payload, bool full,
                            size_t max_preview);
