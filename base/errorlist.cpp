#include "base/errorlist.h"

namespace base {

std::ostream& operator<<(std::ostream& out, const ErrorList& e) {
  e.PrintTo(&out, OutputOptions::kSimpleOutput);
  return out;
}

}  // namespace base
