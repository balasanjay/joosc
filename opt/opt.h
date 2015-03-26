#ifndef OPT_OPT_H
#define OPT_OPT_H

#include "ir/ir_generator.h"

namespace opt {

// Performs IR-to-IR optimization.
ir::Program Optimize(const ir::Program&);

} // namespace opt

#endif
