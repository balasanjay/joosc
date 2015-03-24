#ifndef IR_STREAM_H
#define IR_STREAM_H

namespace ir {

enum class SizeClass {
  BOOL = 1,
  BYTE = 1,
  SHORT = 2,
  CHAR = 2,
  INT = 4,
  PTR = 4,
};

class Stream {
  // TODO: this needs an API. Maybe just a vector<Op>, and a vector<Arg>.
};

} // namespace ir

#endif
