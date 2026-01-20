// jpeg_indexer.h
#pragma once
#include "jpeg_types.h"
#include <string>
#include <vector>

struct IndexOptions {
  size_t app_peek_bytes = 64; // 识别APP subtype只读前缀
};

struct JpegIndexResult {
  std::vector<SegmentIndex> segments;
};

JpegIndexResult build_jpeg_index(const std::string &path,
                                 const IndexOptions &opt);
bool load_segment_payload(const std::string &path, const SegmentIndex &seg,
                          std::vector<uint8_t> &out);
