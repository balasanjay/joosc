#ifndef BASE_ERRORLIST_H
#define BASE_ERRORLIST_H

#include "base/error.h"
#include "base/unique_ptr_vector.h"

namespace base {

class ErrorList : public UniquePtrVector<Error> {
 public:
  void PrintTo(std::ostream* out, const OutputOptions& opt) const;

  bool IsFatal() const {
    // TODO: handle non-fatal errors.
    return Size() != 0;
  }
};

std::ostream& operator<<(std::ostream& out, const ErrorList& e);

}  // namespace base

#endif
