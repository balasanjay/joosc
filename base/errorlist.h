#ifndef BASE_ERRORLIST_H
#define BASE_ERRORLIST_H

#include "base/error.h"

namespace base {

class ErrorList {
 public:
  ErrorList() = default;
  ErrorList(ErrorList&&) = default;

  ~ErrorList() {
    for (auto error : errors_) {
      delete error;
    }
  }

  void Add(Error* err) { errors_.push_back(err); }

  const Error* Get(int i) { return errors_.at(i); }

  int Size() { return errors_.size(); }

  void PrintTo(std::ostream* out, const OutputOptions& opt) const {
    for (auto error : errors_) {
      error->PrintTo(out, opt);
      *out << '\n';
    }
  }

  bool IsFatal() const {
    // TODO: handle non-fatal errors.
    return !errors_.empty();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ErrorList);

  vector<Error*> errors_;
};

std::ostream& operator<<(std::ostream& out, const ErrorList& e);

}  // namespace base

#endif
