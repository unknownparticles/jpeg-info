cmake_minimum_required(VERSION 3.16)
project(jpeg_info LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(jpeg_info
  src/main.cpp
  src/i18n.cpp
  src/file_reader.cpp
  src/jpeg_indexer.cpp
  src/parse_jfif.cpp
  src/parse_sof.cpp
  src/parse_adobe.cpp
  src/parse_com.cpp
  src/parse_xmp.cpp
  src/parse_icc.cpp
  src/parse_exif.cpp
  src/format.cpp
)

target_include_directories(jpeg_info PRIVATE src)

if (MSVC)
  target_compile_options(jpeg_info PRIVATE /W4)
else()
  target_compile_options(jpeg_info PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS jpeg_info RUNTIME DESTINATION bin)
