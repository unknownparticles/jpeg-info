// parse_icc.cpp
#include "parse_icc.h"
#include <algorithm>
#include <cstring>

std::optional<IccChunk>
parse_icc_chunk_from_app2_payload(const std::vector<uint8_t> &p) {
  const char *sig = "ICC_PROFILE\0";
  size_t siglen = std::strlen("ICC_PROFILE") + 1;
  if (p.size() < siglen + 2)
    return std::nullopt;
  if (std::memcmp(p.data(), sig, siglen) != 0)
    return std::nullopt;

  IccChunk c;
  c.seq_no = p[siglen];
  c.seq_total = p[siglen + 1];
  c.payload.assign(p.begin() + (siglen + 2), p.end());
  return c;
}

std::optional<IccProfile> stitch_icc_profile(std::vector<IccChunk> &chunks) {
  if (chunks.empty())
    return std::nullopt;
  uint8_t total = chunks[0].seq_total;
  if (total == 0)
    return std::nullopt;

  // filter only matching total
  chunks.erase(std::remove_if(chunks.begin(), chunks.end(),
                              [&](const IccChunk &c) {
                                return c.seq_total != total || c.seq_no == 0 ||
                                       c.seq_no > total;
                              }),
               chunks.end());

  if (chunks.size() != total) {
    // 不强制失败：可返回部分信息；但这里按“完整拼接”语义选择失败
    return std::nullopt;
  }

  std::sort(
      chunks.begin(), chunks.end(),
      [](const IccChunk &a, const IccChunk &b) { return a.seq_no < b.seq_no; });

  IccProfile prof;
  for (const auto &c : chunks)
    prof.total_len += (uint32_t)c.payload.size();
  prof.data.reserve(prof.total_len);
  for (const auto &c : chunks)
    prof.data.insert(prof.data.end(), c.payload.begin(), c.payload.end());
  return prof;
}
