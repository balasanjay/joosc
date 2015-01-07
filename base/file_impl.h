#ifndef BASE_FILE_IMPL_H
#define BASE_FILE_IMPL_H

#include "base/file.h"
#include <string>

namespace base {
  class StringFile: public File {
  public:
    StringFile(std::string s);

  protected:
    virtual ~StringFile();
  };
} // namespace base

#endif
