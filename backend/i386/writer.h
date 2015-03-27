#ifndef BACKEND_I386_WRITER_H
#define BACKEND_I386_WRITER_H

#include <ostream>

#include "ir/ir_generator.h"
#include "ir/stream.h"

namespace backend {
namespace i386 {

class Writer {
public:
  void WriteCompUnit(const ir::CompUnit& comp_unit, std::ostream* out) const;
  void WriteMain(std::ostream* out) const;
private:
  void WriteFunc(const ir::Stream& stream, std::ostream* out) const;
};

} // namespace i386
} // namespace backend

#endif
