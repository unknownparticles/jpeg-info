// parse_sof.h
#pragma once
#include "jpeg_types.h"
#include <optional>
#include <vector>

std::optional<SofInfo> parse_sof_payload(uint16_t marker,
                                         const std::vector<uint8_t> &payload);
bool is_sof_marker(uint16_t marker);
