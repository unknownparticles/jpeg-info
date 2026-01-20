// parse_jfif.h
#pragma once
#include "jpeg_types.h"
#include <optional>
#include <vector>

std::optional<JfifInfo>
parse_jfif_from_app0_payload(const std::vector<uint8_t> &payload);
