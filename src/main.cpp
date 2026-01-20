// main.cpp
#include "format.h"
#include "i18n.h"
#include "jpeg_indexer.h"
#include "parse_adobe.h"
#include "parse_com.h"
#include "parse_exif.h"
#include "parse_icc.h"
#include "parse_jfif.h"
#include "parse_sof.h"
#include "parse_xmp.h"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  I18n i18n;
  i18n.lang = Lang::ZH; // 默认中文

  std::string path;
  bool help_requested = false;

  // 过滤选项
  bool show_segments = false;
  bool show_jfif = false;
  bool show_sof = false;
  bool show_exif = false;
  bool show_xmp = false;
  bool show_icc = false;
  bool show_adobe = false;
  bool show_com = false;
  bool any_filter_set = false;

  // 解析命令行参数
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      help_requested = true;
    } else if (arg == "--lang=en") {
      i18n.lang = Lang::EN;
    } else if (arg == "--lang=zh") {
      i18n.lang = Lang::ZH;
    } else if (arg == "--segments") {
      show_segments = true;
      any_filter_set = true;
    } else if (arg == "--jfif") {
      show_jfif = true;
      any_filter_set = true;
    } else if (arg == "--sof") {
      show_sof = true;
      any_filter_set = true;
    } else if (arg == "--exif") {
      show_exif = true;
      any_filter_set = true;
    } else if (arg == "--xmp") {
      show_xmp = true;
      any_filter_set = true;
    } else if (arg == "--icc") {
      show_icc = true;
      any_filter_set = true;
    } else if (arg == "--adobe") {
      show_adobe = true;
      any_filter_set = true;
    } else if (arg == "--com") {
      show_com = true;
      any_filter_set = true;
    } else if (path.empty()) {
      path = arg;
    }
  }

  if (help_requested || path.empty()) {
    std::cout << "JPEG Info - JPEG 元数据解析工具\n\n";
    std::cout << "用法: " << argv[0] << " <jpeg文件> [选项]\n\n";
    std::cout << "选项:\n";
    std::cout << "  -h, --help      显示此帮助信息\n";
    std::cout << "  --lang=en|zh    设置显示语言 (默认: zh)\n\n";
    std::cout << "选择性输出选项 (可组合使用):\n";
    std::cout << "  --segments      只显示分区列表\n";
    std::cout << "  --jfif          只显示 JFIF 信息\n";
    std::cout << "  --sof           只显示图像基本信息 (SOF)\n";
    std::cout << "  --exif          只显示 EXIF 信息\n";
    std::cout << "  --xmp           只显示 XMP 信息\n";
    std::cout << "  --icc           只显示 ICC Profile 信息\n";
    std::cout << "  --adobe         只显示 Adobe APP14 信息\n";
    std::cout << "  --com           只显示注释信息\n\n";
    std::cout << "示例:\n";
    std::cout << "  " << argv[0] << " image.jpg\n";
    std::cout << "  " << argv[0] << " image.jpg --exif\n";
    std::cout << "  " << argv[0] << " image.jpg --exif --xmp --lang=en\n";
    return help_requested ? 0 : 1;
  }

  // 如果没有设置任何过滤选项，则显示所有内容
  if (!any_filter_set) {
    show_segments = show_jfif = show_sof = show_exif = show_xmp = show_icc =
        show_adobe = show_com = true;
  }

  // 构建JPEG索引
  IndexOptions opt;
  opt.app_peek_bytes = 128; // 读取更多字节用于识别APP子类型
  JpegIndexResult result = build_jpeg_index(path, opt);

  if (result.segments.empty()) {
    std::cerr << i18n.t("error_parse") << ": " << path << "\n";
    return 1;
  }

  std::cout << "JPEG Info: " << path << "\n";
  std::cout << std::string(80, '=') << "\n";

  // 打印分区列表
  if (show_segments) {
    print_segments(std::cout, result.segments, i18n);
  }

  // 解析并打印各种元数据
  for (const auto &seg : result.segments) {
    std::vector<uint8_t> payload;

    // JFIF (APP0)
    if (show_jfif && seg.marker == 0xFFE0 && seg.app_subtype == "JFIF") {
      if (load_segment_payload(path, seg, payload)) {
        auto jfif = parse_jfif_from_app0_payload(payload);
        if (jfif.has_value()) {
          print_jfif_info(std::cout, jfif.value(), i18n);
        }
      }
    }

    // SOF (Start of Frame)
    if (show_sof && is_sof_marker(seg.marker)) {
      if (load_segment_payload(path, seg, payload)) {
        auto sof = parse_sof_payload(seg.marker, payload);
        if (sof.has_value()) {
          print_sof_info(std::cout, sof.value(), i18n);
        }
      }
    }

    // EXIF (APP1)
    if (show_exif && seg.marker == 0xFFE1 && seg.app_subtype == "EXIF") {
      if (load_segment_payload(path, seg, payload)) {
        auto exif = parse_exif_from_app1_payload(payload);
        if (exif.has_value()) {
          print_exif_info(std::cout, exif.value(), i18n);
        }
      }
    }

    // XMP (APP1)
    if (show_xmp && seg.marker == 0xFFE1 && seg.app_subtype == "XMP") {
      if (load_segment_payload(path, seg, payload)) {
        // 默认显示完整 XML（full=true），不截断
        auto xmp = parse_xmp_from_app1_payload(payload, true, 2048);
        if (xmp.has_value()) {
          print_xmp_info(std::cout, xmp.value(), i18n);
        }
      }
    }

    // ICC Profile (APP2)
    if (show_icc && seg.marker == 0xFFE2 && seg.app_subtype == "ICC") {
      if (load_segment_payload(path, seg, payload)) {
        auto chunk = parse_icc_chunk_from_app2_payload(payload);
        if (chunk.has_value()) {
          // 收集所有ICC chunks并拼接
          std::vector<IccChunk> chunks;
          chunks.push_back(chunk.value());

          // 查找其他ICC chunks
          for (const auto &other_seg : result.segments) {
            if (other_seg.marker == 0xFFE2 && other_seg.app_subtype == "ICC" &&
                other_seg.marker_offset != seg.marker_offset) {
              std::vector<uint8_t> other_payload;
              if (load_segment_payload(path, other_seg, other_payload)) {
                auto other_chunk =
                    parse_icc_chunk_from_app2_payload(other_payload);
                if (other_chunk.has_value()) {
                  chunks.push_back(other_chunk.value());
                }
              }
            }
          }

          auto icc = stitch_icc_profile(chunks);
          if (icc.has_value()) {
            print_icc_info(std::cout, icc.value(), i18n);
          }
        }
      }
    }

    // Adobe (APP14)
    if (show_adobe && seg.marker == 0xFFEE && seg.app_subtype == "Adobe") {
      if (load_segment_payload(path, seg, payload)) {
        auto adobe = parse_adobe_app14_payload(payload);
        if (adobe.has_value()) {
          print_adobe_info(std::cout, adobe.value(), i18n);
        }
      }
    }

    // COM (Comment)
    if (show_com && seg.marker == 0xFFFE) {
      if (load_segment_payload(path, seg, payload)) {
        auto com = parse_com_payload_preview(payload, 256);
        print_com_info(std::cout, com, i18n);
      }
    }
  }

  return 0;
}
