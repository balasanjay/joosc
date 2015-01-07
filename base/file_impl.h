#ifndef BASE_FILE_IMPL_H
#define BASE_FILE_IMPL_H

#include "base/file.h"
#include "third_party/gtest/gtest.h"
#include <string>

namespace base {

class StringFileTest;

class StringFile: public File {
public:
  StringFile(std::string s);

protected:
  FRIEND_TEST(StringFileTest, Simple);

  ~StringFile() override;
};

} // namespace base

#endif
