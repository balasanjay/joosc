#include "base/error.h"
#include "base/errorlist.h"

namespace base {

void ErrorList::PrintTo(std::ostream* out, const OutputOptions& opt, const FileSet* fs) const {
  for (int i = 0; i < Size(); ++i) {
    At(i)->PrintTo(out, opt, fs);
    *out << '\n';
  }
}

std::ostream& operator<<(std::ostream& out, const ErrorList& e) {
  e.PrintTo(&out, OutputOptions::kSimpleOutput, nullptr);
  return out;
}

}  // namespace base
