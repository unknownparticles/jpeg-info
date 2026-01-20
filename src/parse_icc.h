// parse_icc.h
#pragma once
#include "jpeg_types.h"
#include <optional>
#include <string>
#include <vector>

struct IccChunk {
  uint8_t seq_no = 0;
  uint8_t seq_total = 0;
  std::vector<uint8_t> payload; // 含ICC header后的数据片
};

std::optional<IccChunk>
parse_icc_chunk_from_app2_payload(const std::vector<uint8_t> &payload);
std::optional<IccProfile> stitch_icc_profile(std::vector<IccChunk> &chunks);
