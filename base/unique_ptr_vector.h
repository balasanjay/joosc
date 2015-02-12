#ifndef BASE_UNIQUE_PTR_VECTOR
#define BASE_UNIQUE_PTR_VECTOR

namespace base {

template <typename T>
class UniquePtrVector {
 public:
  UniquePtrVector() = default;
  UniquePtrVector(UniquePtrVector&&) = default;
  virtual ~UniquePtrVector() = default;
  UniquePtrVector& operator=(UniquePtrVector&&) = default;

  // Returns the number of elements in the vector.
  int Size() const { return (int)vec_.size(); }

  // Returns the i-th element of the vector; the vector retains ownership.
  T* At(int i) const { return vec_.at(i).get(); }

  // Adds t to the vector; takes ownership of t.
  void Append(T* t) { assert(t != nullptr); vec_.emplace_back(t); }

  T* ReleaseBack() {
    T* t = vec_.at(Size() - 1).release();
    vec_.pop_back();
    return t;
  }

  // Releases ownership of all contained elements; appends elements to provided
  // vector.
  void Release(vector<T*>* out) {
    for (uint i = 0; i < vec_.size(); ++i) {
      out->push_back(vec_.at(i).release());
    }
    vec_.clear();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UniquePtrVector);

  vector<uptr<T>> vec_;
};

template <typename T>
class UniquePtrVectorIter : public std::iterator<std::forward_iterator_tag, T> {
  public:
    UniquePtrVectorIter() = default;
    UniquePtrVectorIter(const UniquePtrVectorIter& other) = default;
    UniquePtrVectorIter& operator=(const UniquePtrVectorIter&) = default;
    ~UniquePtrVectorIter() = default;

    UniquePtrVectorIter(int idx, const UniquePtrVector<T>* vec) : idx_(idx), vec_(vec) {}

    bool operator==(const UniquePtrVectorIter& other) const {
      return idx_ == other.idx_ && vec_ == other.vec_;
    }

    bool operator!=(const UniquePtrVectorIter& other) const {
      return !(*this == other);
    }

    const T& operator*() const {
      assert(vec_ != nullptr);
      return *(vec_->At(idx_));
    }

    const T* operator->() const {
      assert(vec_ != nullptr);
      return vec_->Get(idx_);
    }

    // Prefix increment.
    UniquePtrVectorIter& operator++() {
      ++idx_;
      return *this;
    }

    // Postfix increment.
    UniquePtrVectorIter operator++(int) {
      UniquePtrVectorIter copy = *this;
      ++idx_;
      return copy;
    }

private:
  int idx_;
  const UniquePtrVector<T>* vec_;
};

template <typename T>
UniquePtrVectorIter<T> begin(const UniquePtrVector<T>& vec) {
  return UniquePtrVectorIter<T>(0, &vec);
}

template <typename T>
UniquePtrVectorIter<T> end(const UniquePtrVector<T>& vec) {
  return UniquePtrVectorIter<T>(vec.Size(), &vec);
}

}  // namespace base

#endif
