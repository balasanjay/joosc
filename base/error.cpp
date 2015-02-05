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

Error* MakeSimplePosRangeError(const FileSet* fs, PosRange pos, string name,
                               string msg) {
  class Err : public PosRangeError {
   public:
    Err(const FileSet* fs, PosRange posrange, string name, string msg)
        : PosRangeError(fs, posrange), name_(name), msg_(msg) {}

   protected:
    string SimpleError() const override { return name_; }
    string Error() const override { return msg_; }

   private:
    string name_;
    string msg_;
  };
  return new Err(fs, pos, name, msg);
}

}  // namespace base
