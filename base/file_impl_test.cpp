#include "base/file_impl.h"
#include "third_party/gtest/gtest.h"

namespace base {

TEST(StringFileTest, Simple) {
  StringFile sf("hello");

  EXPECT_EQ('h', sf[0]);
}

} // namespace base

