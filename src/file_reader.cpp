// file_reader.cpp
#include "file_reader.h"
#include <cstdlib>

FileReader::FileReader(const char *path) { f_ = std::fopen(path, "rb"); }
FileReader::~FileReader() {
  if (f_)
    std::fclose(f_);
}
bool FileReader::ok() const { return f_ != nullptr; }

bool FileReader::seek(uint64_t off) {
#if defined(_WIN32)
  return _fseeki64(f_, (int64_t)off, SEEK_SET) == 0;
#else
  return fseeko(f_, (off_t)off, SEEK_SET) == 0;
#endif
}
uint64_t FileReader::tell() {
#if defined(_WIN32)
  return (uint64_t)_ftelli64(f_);
#else
  return (uint64_t)ftello(f_);
#endif
}
bool FileReader::read_u8(uint8_t &out) {
  int c = std::fgetc(f_);
  if (c == EOF)
    return false;
  out = (uint8_t)c;
  return true;
}
bool FileReader::read_bytes(uint8_t *dst, size_t n) {
  return std::fread(dst, 1, n, f_) == n;
}
bool FileReader::read_bytes(std::vector<uint8_t> &buf, size_t n) {
  buf.resize(n);
  return read_bytes(buf.data(), n);
}
