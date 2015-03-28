#ifndef BASE_PRINTF_H
#define BASE_PRINTF_H

#include <ostream>

namespace base {
namespace internal {

void FprintfImpl(std::ostream* out, const string& fmt, size_t begin, size_t end);

template<typename T, typename... Args>
void FprintfImpl(std::ostream* out, const string& fmt, size_t begin, size_t end, const T& value, Args... args) {
  while (begin < end) {
    if (fmt[begin] != '%') {
      *out << fmt[begin];
      ++begin;
      continue;
    }

    if (begin + 1 == end) {
      throw std::logic_error("Invalid format string: '" + fmt + "', trailing percent sign.");
    }
    if (fmt[begin + 1] == '%') {
      *out << '%';
      begin += 2;
      continue;
    }

    if (fmt[begin + 1] != 'v') {
      throw std::logic_error("Invalid format string: '" + fmt + "', unknown format specifier.");
    }

    *out << value;
    begin += 2;
    FprintfImpl(out, fmt, begin, end, args...);
    return;
  }

  throw std::logic_error("Invalid format string: '" + fmt + "', too few placeholders.");
}

} // namespace internal

template<typename... Args>
void Fprintf(std::ostream* out, const string& fmt, Args... args) {
  internal::FprintfImpl(out, fmt, 0, fmt.size(), args...);
}

template<typename... Args>
string Sprintf(const string& fmt, Args... args) {
  stringstream ss;
  Fprintf(&ss, fmt, args...);
  return ss.str();
}

} // namespace base

#endif
