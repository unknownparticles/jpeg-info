// i18n.h
#pragma once
#include <string>

enum class Lang { EN, ZH };

struct I18n {
  Lang lang = Lang::ZH;
  std::string t(const std::string &key) const;
};
