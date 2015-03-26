#ifndef OPT_PEEP_H
#define OPT_PEEP_H

#include "ir/stream.h"

namespace opt {

ir::Stream PeepholeOptimize(const ir::Stream&);

} // namespace opt

#endif
