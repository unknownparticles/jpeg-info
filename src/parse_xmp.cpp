// parse_xmp.cpp
#include "parse_xmp.h"
#include <cstring>

// 查找 XMP packet 的有效结束位置
static size_t find_xmp_end(const char *data, size_t len) {
  size_t end_pos = len;

  // 优先查找 <?xpacket end= 标记（最可靠）
  const char *xpacket_end = "<?xpacket end=";
  size_t xpacket_end_len = std::strlen(xpacket_end);

  for (size_t i = 0; i + xpacket_end_len < len; i++) {
    if (std::memcmp(data + i, xpacket_end, xpacket_end_len) == 0) {
      // 找到 <?xpacket end=，继续查找结束的 ?>
      for (size_t j = i + xpacket_end_len; j + 1 < len; j++) {
        if (data[j] == '?' && data[j + 1] == '>') {
          end_pos = j + 2; // ?> 之后的位置
          break;
        }
      }
      if (end_pos < len)
        break; // 找到了，跳出外层循环
    }
  }

  // 如果没找到 <?xpacket end=，查找 </x:xmpmeta>（次可靠）
  if (end_pos == len) {
    const char *xmpmeta_end = "</x:xmpmeta>";
    size_t xmpmeta_end_len = std::strlen(xmpmeta_end);

    for (size_t i = 0; i + xmpmeta_end_len <= len; i++) {
      if (std::memcmp(data + i, xmpmeta_end, xmpmeta_end_len) == 0) {
        end_pos = i + xmpmeta_end_len;
        break;
      }
    }
  }

  // 关键修复：无论找到哪种结束标记，都要 trim 尾部的空白和 \0
  while (end_pos > 0) {
    unsigned char ch = (unsigned char)data[end_pos - 1];
    if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\0' ||
        ch == 0x1A) {
      end_pos--;
    } else {
      break;
    }
  }

  return end_pos;
}

// 简单的 XML 标签提取器（不是完整的 XML 解析器）
static std::string extract_xml_tag_content(const std::string &xml,
                                           const std::string &tag) {
  // 查找 <tag>content</tag> 或 <prefix:tag>content</prefix:tag>
  std::string pattern1 = "<" + tag + ">";
  std::string pattern2 = "</" + tag + ">";
  std::string pattern3 = ":" + tag + ">";
  std::string pattern4 = ":/" + tag + ">";

  size_t start = xml.find(pattern1);
  if (start == std::string::npos) {
    // 尝试带命名空间的形式
    start = xml.find(pattern3);
    if (start == std::string::npos)
      return "";
    start = xml.rfind('<', start);
    if (start == std::string::npos)
      return "";
    start = xml.find('>', start);
    if (start == std::string::npos)
      return "";
  } else {
    start += pattern1.length();
  }

  size_t end = xml.find(pattern2, start);
  if (end == std::string::npos) {
    end = xml.find(pattern4, start);
    if (end == std::string::npos)
      return "";
    end = xml.rfind('<', end);
  }

  if (end <= start)
    return "";
  return xml.substr(start, end - start);
}

std::optional<XmpInfo>
parse_xmp_from_app1_payload(const std::vector<uint8_t> &p, bool full,
                            size_t max_preview) {
  const char *sig = "http://ns.adobe.com/xap/1.0/\0";
  size_t siglen = std::strlen("http://ns.adobe.com/xap/1.0/") + 1;
  if (p.size() < siglen)
    return std::nullopt;
  if (std::memcmp(p.data(), sig, siglen) != 0)
    return std::nullopt;

  XmpInfo x;
  x.len = (uint32_t)p.size();

  const char *xml_start = (const char *)p.data() + siglen;
  size_t xml_total_len = p.size() - siglen;

  // 查找有效 XML 内容的结束位置
  size_t effective_end = find_xmp_end(xml_start, xml_total_len);
  x.effective_len = (uint32_t)effective_end;
  x.padding_len = (uint32_t)(xml_total_len - effective_end);

  // 根据 full 参数决定是否截断显示
  if (full || max_preview >= effective_end) {
    x.xml.assign(xml_start, xml_start + effective_end);
    x.truncated = false;
  } else {
    size_t take = std::min(effective_end, max_preview);
    x.xml.assign(xml_start, xml_start + take);
    x.truncated = true;
  }

  // 清理不可打印字符
  for (auto &ch : x.xml) {
    unsigned char u = (unsigned char)ch;
    if (u < 0x20 && ch != '\n' && ch != '\r' && ch != '\t')
      ch = ' ';
  }

  // 提取常见字段（轻量级提取，不是完整 XML 解析）
  std::string rating = extract_xml_tag_content(x.xml, "Rating");
  std::string create_date = extract_xml_tag_content(x.xml, "CreateDate");
  std::string modify_date = extract_xml_tag_content(x.xml, "ModifyDate");
  std::string lens = extract_xml_tag_content(x.xml, "Lens");
  std::string lens_model = extract_xml_tag_content(x.xml, "LensModel");

  // 将提取的字段添加到 XMP 信息中
  if (!rating.empty() || !create_date.empty() || !lens.empty()) {
    std::string summary = "\n[Extracted Fields]\n";
    if (!rating.empty())
      summary += "  Rating: " + rating + "\n";
    if (!create_date.empty())
      summary += "  CreateDate: " + create_date + "\n";
    if (!modify_date.empty())
      summary += "  ModifyDate: " + modify_date + "\n";
    if (!lens.empty())
      summary += "  Lens: " + lens + "\n";
    if (!lens_model.empty())
      summary += "  LensModel: " + lens_model + "\n";
    summary += "\n[Full XML]\n";
    x.xml = summary + x.xml;
  }

  return x;
}
