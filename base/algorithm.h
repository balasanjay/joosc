#ifndef BASE_ALGORITHM_H
#define BASE_ALGORITHM_H

namespace base {

// FindEqualRanges identifies ranges that are comprised of consecutive equal
// elements in the range [begin, end). Cmp should be callable and take two
// values in the collection. cb should deal with an iterator pair and number of
// duplicates.
template <typename Iter, typename Cmp, typename Callback>
void FindEqualRanges(Iter cbegin, Iter cend, Cmp cmp, Callback cb) {
  auto cur = cbegin;
  while (cur != cend) {
    // The range [start, end) is meant to track a range of duplicate entries.
    auto start = cur;
    auto end = std::next(start);
    i64 ndups = 1;

    while (end != cend && cmp(*start, *end)) {
      ++ndups;
      ++end;
    }

    // Immediately advance our outer iterator past the relevant range, so
    // that we can't forget to do it later.
    cur = end;
    cb(start, end, ndups);
  }
}

} // namespace base

#endif
