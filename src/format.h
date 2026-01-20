// format.h
#pragma once
#include "i18n.h"
#include "jpeg_types.h"
#include <iostream>
#include <string>
#include <vector>

// 格式化输出分区列表
void print_segments(std::ostream &os, const std::vector<SegmentIndex> &segs,
                    const I18n &i18n);

// 格式化输出JFIF信息
void print_jfif_info(std::ostream &os, const JfifInfo &jfif, const I18n &i18n);

// 格式化输出SOF信息
void print_sof_info(std::ostream &os, const SofInfo &sof, const I18n &i18n);

// 格式化输出Adobe信息
void print_adobe_info(std::ostream &os, const AdobeInfo &adobe,
                      const I18n &i18n);

// 格式化输出COM信息
void print_com_info(std::ostream &os, const ComInfo &com, const I18n &i18n);

// 格式化输出XMP信息
void print_xmp_info(std::ostream &os, const XmpInfo &xmp, const I18n &i18n);

// 格式化输出ICC Profile信息
void print_icc_info(std::ostream &os, const IccProfile &icc, const I18n &i18n);

// 格式化输出EXIF信息
void print_exif_info(std::ostream &os, const ExifResult &exif,
                     const I18n &i18n);
