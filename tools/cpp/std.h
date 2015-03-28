#ifndef STD_H
#define STD_H

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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
using std::map;
using std::multimap;
using std::multiset;
using std::pair;
using std::set;
using std::string;
using std::stringstream;
using std::vector;

// Joos char and string - Java's char is u16.
using jchar = u16;
using jstring = std::basic_string<jchar>;

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

#define CHECK(cond) { \
  if (!(cond)) { \
    stringstream ss; \
    ss << __FILE__ << ':' << __LINE__ << ' ' << #cond; \
    throw std::logic_error(ss.str()); \
  } \
}

#define UNIMPLEMENTED() { \
  stringstream ss; \
  ss << __FILE__ << ':' << __LINE__ << " Unimplemented."; \
  throw std::logic_error(ss.str()); \
}

#define UNREACHABLE() { \
  stringstream ss; \
  ss << __FILE__ << ':' << __LINE__ << " Should be unreachable."; \
  throw std::logic_error(ss.str()); \
}

#endif
