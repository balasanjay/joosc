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

struct StackFrame {
  int fid;
  ast::TypeId::Base tid;
  ast::MethodId mid;
  int line;
};

class Writer {
public:
  Writer(const types::TypeInfoMap& tinfo_map, const backend::common::OffsetTable& offsets, const ir::RuntimeLinkIds& rt_ids, const base::FileSet& fs) : tinfo_map_(tinfo_map), offsets_(offsets), rt_ids_(rt_ids), fs_(fs) {}
  void WriteCompUnit(const ir::CompUnit& comp_unit, std::ostream* out) const;
  void WriteMain(std::ostream* out) const;
  void WriteStaticInit(const ir::Program& prog, std::ostream* out) const;
  void WriteConstStrings(const types::ConstStringMap&, std::ostream* out) const;
  void WriteFileNames(std::ostream* out) const;
  void WriteMethods(std::ostream* out) const;
private:
  void WriteFunc(const ir::Stream& stream, const base::File* file, StackFrame frame, vector<StackFrame>* stack_out, std::ostream* out) const;
  void WriteVtable(const ir::Type& type, std::ostream* out) const;
  void WriteItable(const ir::Type& type, std::ostream* out) const;
  void WriteStatics(const ir::Type& type, std::ostream* out) const;
  void WriteConstStringsImpl(const string& prefix, const vector<pair<jstring, u64>>& strings, std::ostream* out) const;
  void WriteStackFrames(const vector<StackFrame>& stack, std::ostream* out) const;

  const types::TypeInfoMap& tinfo_map_;
  const backend::common::OffsetTable& offsets_;
  const ir::RuntimeLinkIds& rt_ids_;
  const base::FileSet& fs_;
};

} // namespace i386
} // namespace backend

#endif
