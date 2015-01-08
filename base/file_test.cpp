#include "base/fileset.h"
#include "third_party/gtest/gtest.h"

namespace base {

TEST(PathTest, BothExist) {
  EXPECT_EQ("foo/bar", Dirname("foo/bar/baz"));
  EXPECT_EQ("baz", Basename("foo/bar/baz"));
}

TEST(PathTest, Empty) {
  EXPECT_EQ("", Dirname(""));
  EXPECT_EQ("", Basename(""));
}

TEST(PathTest, NoDir) {
  EXPECT_EQ("", Dirname("Object.java"));
  EXPECT_EQ("Object.java", Basename("Object.java"));
}

TEST(PathTest, TrailingSlash) {
  EXPECT_EQ("foo/bar", Dirname("foo/bar/"));
  EXPECT_EQ("", Basename("foo/bar/"));
}

}  // namespace base
