// file_reader.h
#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>

class FileReader {
public:
  explicit FileReader(const char *path);
  ~FileReader();
  bool ok() const;

  bool seek(uint64_t off);
  uint64_t tell();

  bool read_u8(uint8_t &out);
  bool read_bytes(uint8_t *dst, size_t n);
  bool read_bytes(std::vector<uint8_t> &buf, size_t n);

private:
  FILE *f_ = nullptr;
};
