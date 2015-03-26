#include "opt/opt.h"

#include <iostream>

#include "ir/ir_generator.h"
#include "ir/stream.h"
#include "opt/peep.h"

using ir::CompUnit;
using ir::Program;
using ir::Stream;

namespace opt {

Program Optimize(const Program& prog) {
  Program newprog = prog;
  for (CompUnit& unit : newprog.units) {
    for (Stream& stream : unit.streams) {
      stream = PeepholeOptimize(stream);
    }
  }
  return newprog;
}

}
