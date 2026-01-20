// parse_adobe.h
#pragma once
#include "jpeg_types.h"
#include <optional>
#include <vector>

std::optional<AdobeInfo>
parse_adobe_app14_payload(const std::vector<uint8_t> &payload);
