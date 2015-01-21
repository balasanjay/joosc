#ifndef BASE_UNIQUE_PTR_VECTOR
#define BASE_UNIQUE_PTR_VECTOR

namespace base {

template <typename T>
class UniquePtrVector {
public:
  UniquePtrVector() = default;
  UniquePtrVector(UniquePtrVector&&) = default;
  ~UniquePtrVector() {
    for (uint i = 0; i < vec_.size(); ++i) {
      delete vec_.at(i);
    }
    vec_.clear();
  }

  // Returns the number of elements in the vector.
  int Size() const {
    return (int)vec_.size();
  }

  // Returns the i-th element of the vector; the vector retains ownership.
  T* At(int i) const {
    return vec_.at(i);
  }

  // Adds t to the vector; takes ownership of t.
  void Append(T* t) {
    vec_.push_back(t);
  }

  // Releases ownership of all contained elements; appends elements to provided vector.
  void Release(vector<T*>* out) {
    for (uint i = 0; i < vec_.size(); ++i) {
      out->push_back(vec_.at(i));
    }
    vec_.clear();
  }

private:
  DISALLOW_COPY_AND_ASSIGN(UniquePtrVector);

  vector<T*> vec_;
};

} // namespace base`

#endif
