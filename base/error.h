#ifndef BASE_ERROR_H
#define BASE_ERROR_H

#include <ostream>

namespace base {

struct OutputOptions {
  OutputOptions(bool colorize, bool simple)
      : colorize(colorize), simple(simple) {}

  const bool colorize = true;
  const bool simple = false;

  static const OutputOptions kSimpleOutput;
  static const OutputOptions kUserOutput;
};

class Error {
 public:
  virtual ~Error() {}

  virtual void PrintTo(std::ostream* out, const OutputOptions& opt) const = 0;

 protected:
  Error() {}

  string Red(const OutputOptions& opt) const;
  string ResetFmt(const OutputOptions& opt) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(Error);
};

std::ostream& operator<<(std::ostream& out, const Error& e);

}  // namespace base

#endif
