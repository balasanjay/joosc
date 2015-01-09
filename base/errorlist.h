#ifndef BASE_ERRORLIST_H
#define BASE_ERRORLIST_H

#include "base/error.h"

namespace base {

class ErrorList {
  public:
    ~ErrorList() {
      for (auto error : errors_) {
        delete error;
      }
    }

    void Add(Error* err) {
      errors_.push_back(err);
    }

    Error* Get(int i) {
      return errors_.at(i);
    }

    int Size() {
      return errors_.size();
    }

    void PrintTo(std::ostream* out) const {
      PrintTo(out, OutputOptions::kSimpleOutput);
    }

    void PrintTo(std::ostream* out, const OutputOptions& opt) const {
      for (auto error : errors_) {
        error->PrintTo(out, opt);
        *out << '\n';
      }
    }

    bool IsFatal() const {
      return !errors_.empty();
    }

  private:
    vector<Error*> errors_;
};

} // namespace base

#endif

