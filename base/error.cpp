#include "base/error.h"

namespace base {
namespace {

inline string IfColor(const OutputOptions& opt, string color) {
  if (!opt.colorize) {
    return "";
  }
  return "\033[" + color;
}

}  // namespace

const OutputOptions OutputOptions::kSimpleOutput(false, true);
const OutputOptions OutputOptions::kUserOutput(true, false);

string Error::Red(const OutputOptions& opt) const {
  return IfColor(opt, "31m");
}
string Error::ResetFmt(const OutputOptions& opt) const {
  return IfColor(opt, "0m");
}

std::ostream& operator<<(std::ostream& out, const Error& e) {
  e.PrintTo(&out, OutputOptions::kSimpleOutput);
  return out;
}

}  // namespace base
