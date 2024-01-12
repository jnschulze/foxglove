#include "error_details.h"

#include <cassert>
#include <sstream>

#include "string_utils.h"

namespace foxglove {

namespace {

#ifdef _WIN32

std::string HResultToString(HRESULT hr) {
  assert(hr != S_OK);
  if (HRESULT_FACILITY(hr) == FACILITY_WINDOWS) {
    hr = HRESULT_CODE(hr);
  }
  wchar_t* buffer;
  if (::FormatMessageW(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr,
          hr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&buffer, 0,
          nullptr) == 0) {
    return {};
  }
  auto str = util::Utf8FromUtf16(buffer);
  ::LocalFree(buffer);
  util::TrimRight(str);
  return str;
}

#endif

}  // namespace

#ifdef _WIN32
// Static
ErrorDetails ErrorDetails::FromHResult(HRESULT hr, std::string_view message) {
  return ErrorDetails(message, HResultToString(hr), hr);
}
#endif

std::string ErrorDetails::ToString() const {
  std::stringstream ss;
  ss << message_;
  if (description_.has_value()) {
    ss << ": " << description_.value();
  }
  if (error_code_.has_value()) {
    ss << " (" << CodeToString() << ")";
  }
  return ss.str();
}

std::string ErrorDetails::CodeToString() const {
  if (error_code_.has_value()) {
    std::stringstream ss;
    ss << "0x" << std::hex << error_code_.value();
    return ss.str();
  }
  return {};
}

}  // namespace foxglove
