#ifndef BACKEND_I386_WRITER_H
#define BACKEND_I386_WRITER_H

#include <ostream>

#include "ir/stream.h"

namespace backend {
namespace i386 {

class Writer {
  void WriteAsm(const ir::Stream& stream, std::ostream* out) const;
};

} // namespace i386
} // namespace backend

#endif
