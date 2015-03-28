#include "base/printf.h"

#include <ostream>

namespace base {
namespace internal {

void FprintfImpl(std::ostream* out, const string& fmt, size_t begin, size_t end) {
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

    throw std::logic_error("Invalid format string: '" + fmt + "', too many placeholders.");
  }
}

} // namespace internal
} // namespace base
