#ifndef BACKEND_I386_WRITER_H
#define BACKEND_I386_WRITER_H

#include <ostream>

#include "backend/common/offset_table.h"
#include "base/fileset.h"
#include "base/joos_types.h"
#include "ir/ir_generator.h"
#include "ir/stream.h"
#include "types/type_info_map.h"
#include "types/types.h"

namespace backend {
namespace i386 {

class Writer {
public:
  Writer(const backend::common::OffsetTable& offsets, const ir::RuntimeLinkIds& rt_ids, const base::FileSet& fs) : offsets_(offsets), rt_ids_(rt_ids), fs_(fs) {}
  void WriteCompUnit(const ir::CompUnit& comp_unit, std::ostream* out) const;
  void WriteMain(std::ostream* out) const;
  void WriteStaticInit(const ir::Program& prog, const types::TypeInfoMap& tinfo_map, std::ostream* out) const;
  void WriteConstStrings(const types::ConstStringMap&, std::ostream* out) const;
  void WriteFileNames(std::ostream* out) const;
  void WriteMethods(const types::TypeInfoMap& tinfo_map, std::ostream* out) const;
private:
  void WriteFunc(const ir::Stream& stream, std::ostream* out) const;
  void WriteVtable(const ir::Type& type, std::ostream* out) const;
  void WriteItable(const ir::Type& type, std::ostream* out) const;
  void WriteStatics(const ir::Type& type, std::ostream* out) const;
  void WriteConstStringsImpl(const string& prefix, const vector<pair<jstring, u64>>& strings, std::ostream* out) const;

  const backend::common::OffsetTable& offsets_;
  const ir::RuntimeLinkIds& rt_ids_;
  const base::FileSet& fs_;
};

} // namespace i386
} // namespace backend

#endif
