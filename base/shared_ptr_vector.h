#ifndef BASE_SHARED_PTR_VECTOR
#define BASE_SHARED_PTR_VECTOR

namespace base {

template <typename T>
struct SharedPtrVectorPtrIter;

template <typename T>
class SharedPtrVector {
 public:
  SharedPtrVector() = default;
  SharedPtrVector(SharedPtrVector&&) = default;
  SharedPtrVector(const SharedPtrVector&) = default;
  virtual ~SharedPtrVector() = default;
  SharedPtrVector& operator=(SharedPtrVector&&) = default;

  // Returns the number of elements in the vector.
  int Size() const { return (int)vec_.size(); }

  // Returns the i-th element of the vector; the vector retains ownership.
  sptr<T> At(int i) const { return vec_.at(i); }

  // Adds t to the vector; takes ownership of t.
  void Append(sptr<T> t) { CHECK(t != nullptr); vec_.push_back(t); }

  sptr<T> PopBack() {
    sptr<T> ret = vec_.at(vec_.size() - 1);
    vec_.pop_back();
    return ret;
  }

  const vector<sptr<T>>& Vec() const {
    return vec_;
  }

 private:
  vector<sptr<T>> vec_;
};

template <typename T>
class SharedPtrVectorIter : public std::iterator<std::forward_iterator_tag, T> {
  public:
    SharedPtrVectorIter() = default;
    SharedPtrVectorIter(const SharedPtrVectorIter& other) = default;
    SharedPtrVectorIter& operator=(const SharedPtrVectorIter&) = default;
    ~SharedPtrVectorIter() = default;

    SharedPtrVectorIter(int idx, const SharedPtrVector<T>* vec) : idx_(idx), vec_(vec) {}

    bool operator==(const SharedPtrVectorIter& other) const {
      return idx_ == other.idx_ && vec_ == other.vec_;
    }

    bool operator!=(const SharedPtrVectorIter& other) const {
      return !(*this == other);
    }

    const T& operator*() const {
      CHECK(vec_ != nullptr);
      return *(vec_->At(idx_));
    }

    const T* operator->() const {
      CHECK(vec_ != nullptr);
      return vec_->Get(idx_);
    }

    // Prefix increment.
    SharedPtrVectorIter& operator++() {
      ++idx_;
      return *this;
    }

    // Postfix increment.
    SharedPtrVectorIter operator++(int) {
      SharedPtrVectorIter copy = *this;
      ++idx_;
      return copy;
    }

private:
  int idx_;
  const SharedPtrVector<T>* vec_;
};

template <typename T>
SharedPtrVectorIter<T> begin(const SharedPtrVector<T>& vec) {
  return SharedPtrVectorIter<T>(0, &vec);
}

template <typename T>
SharedPtrVectorIter<T> end(const SharedPtrVector<T>& vec) {
  return SharedPtrVectorIter<T>(vec.Size(), &vec);
}

}  // namespace base

#endif
