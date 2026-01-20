// parse_com.h
#pragma once
#include "jpeg_types.h"
#include <optional>
#include <vector>

ComInfo parse_com_payload_preview(const std::vector<uint8_t> &payload,
                                  size_t max_preview);
