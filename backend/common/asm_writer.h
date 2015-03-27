#ifndef BACKEND_COMMON_ASM_WRITER_H
#define BACKEND_COMMON_ASM_WRITER_H

#include <iostream>

#include "base/printf.h"

namespace backend {
namespace common {

struct AsmWriter final {

  AsmWriter(std::ostream* out) : out_(out) {}

  template<typename... Args>
  void Col0(const string& fmt, Args... args) const {
    base::Fprintf(out_, fmt, args...);
    *out_ << '\n';
  }

  template<typename... Args>
  void Col1(const string& fmt, Args... args) const {
    *out_ << "    ";
    Col0(fmt, args...);
  }

private:
  std::ostream* out_;
};

} // namespace common
} // namespace backend

#endif
