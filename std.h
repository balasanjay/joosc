#ifndef STD_H
#define STD_H

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <sstream>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

using std::make_pair;
using std::make_shared;
using std::pair;
using std::string;
using std::stringstream;
using std::vector;

template<
  class T,
  class Deleter = std::default_delete<T>
> using uptr = std::unique_ptr<T, Deleter>;

template<class T>
using sptr = std::shared_ptr<T>;

// Copied from Chromium.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  void operator=(const TypeName&) = delete

#endif
