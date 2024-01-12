#pragma once

#include <string>

namespace util {
void TrimLeft(std::string& s);
void TrimRight(std::string& s);
#ifdef _WIN32
std::string Utf8FromUtf16(std::wstring_view utf16_string);
std::wstring Utf16FromUtf8(std::string_view utf8_string);
#endif
}  // namespace util
