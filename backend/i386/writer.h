#ifndef BACKEND_I386_WRITER_H
#define BACKEND_I386_WRITER_H

#include <ostream>

#include "backend/common/offset_table.h"
#include "ir/ir_generator.h"
#include "ir/stream.h"

namespace backend {
namespace i386 {

class Writer {
public:
  Writer(const backend::common::OffsetTable& offsets) : offsets_(offsets) {}
  void WriteCompUnit(const ir::CompUnit& comp_unit, std::ostream* out) const;
  void WriteMain(std::ostream* out) const;
  void WriteStaticInit(const ir::Program& prog, std::ostream* out) const;
private:
  void WriteFunc(const ir::Stream& stream, std::ostream* out) const;
  void WriteVtable(const ir::Type& type, std::ostream* out) const;
  void WriteStatics(const ir::Type& type, std::ostream* out) const;

  const backend::common::OffsetTable& offsets_;
};

} // namespace i386
} // namespace backend

#endif
