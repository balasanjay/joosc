#ifndef BASE_THREAD_POOL_H
#define BASE_THREAD_POOL_H

#include <iostream>
#include <deque>

namespace base {

namespace internal {

template <typename T>
class FutureImpl {
public:
  const T& Get() {
    std::lock_guard<std::mutex> lock(mu_);
    cond_.wait(mu_, [&]{ return set_; });
    return data_;
  }

  void Set(const T& val) {
    std::lock_guard<std::mutex> lock(mu_);

    assert(!set_);
    data_ = val;
    set_ = true;

    cond_.notify_all();
  }

private:
  std::mutex mu_;
  std::condition_variable_any cond_;

  bool set_ = false;
  T data_;
};

} // namespace internal

template <typename T>
class Future {
public:
  Future(sptr<internal::FutureImpl<T>> impl) : impl_(impl) {}

  const T& Get() {
    return impl_->Get();
  }
private:
  sptr<internal::FutureImpl<T>> impl_;
};

template <typename T>
class Promise {
public:
  Promise() : impl_(new internal::FutureImpl<T>()) {}
  Promise(const Promise& other) : impl_(other.impl_) {}

  Future<T> GetFuture() {
    return Future<T>(impl_);
  }

  void Set(const T& val) {
    impl_->Set(val);
  }

private:
  sptr<internal::FutureImpl<T>> impl_;
};

class ThreadPool final {
public:
  ThreadPool(u64 maxthreads) : maxthreads_(maxthreads) {
    assert(maxthreads > 0);

    // TODO: do this lazily.
    if (maxthreads > 1) {
      for (u64 i = 0; i < maxthreads; ++i) {
        threads_.emplace_back(&ThreadPool::WorkerMain, this);
      }
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> lock(mu_);
      done_ = true;
      workers_cond_.notify_all();
    }

    for (auto& thr : threads_) {
      thr.join();
    }
  }

  template <typename R>
  Future<R> Accept(std::function<R()> fn) {
    Promise<R> p;
    auto task = [p, fn]() mutable {
      p.Set(fn());
    };
    Future<R> ret = p.GetFuture();

    if (maxthreads_ == 1) {
      task();
      return ret;
    }

    std::lock_guard<std::mutex> lock(mu_);
    tasks_.emplace_back(std::move(task));
    workers_cond_.notify_all();

    return ret;
  }

  void WorkerMain() {
    while (true) {
      std::function<void()> task = []{};

      {
        std::lock_guard<std::mutex> lock(mu_);
        workers_cond_.wait(mu_, [&]{ return tasks_.size() > 0 || done_; });

        if (tasks_.size() > 0) {
          task = tasks_.front();
          tasks_.pop_front();
        } else if (done_) {
          return;
        } else {
          // Should be caught by one of the prior two cases.
          throw;
        }
      }

      task();
    }
  }

private:
  const u64 maxthreads_;
  vector<std::thread> threads_;

  std::mutex mu_;
  std::condition_variable_any workers_cond_;
  bool done_ = false;
  std::deque<std::function<void()>> tasks_;
};

} // namespace base

#endif
