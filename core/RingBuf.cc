#pragma once

#include <algorithm>
#include <memory>

#include "Util.cc"              // relational

template <typename T>
class RingBufIter;

template<typename T>
class RingBuf {
public:
  using iter_t = RingBufIter<T>;

  explicit RingBuf(size_t size) :
    buf_(new T[size]),
    max_size_(size) { }

  void put(T item)
  {
    buf_[head_] = item;
    head_ = (head_ + 1) % max_size_;
    size_ = std::min(size_ + 1, max_size_);
  }

  const T& top() const
  {
    return *begin();
  }

  iter_t begin() const
  {
    return iter_t(*this, 0);
  }

  iter_t end() const
  {
    return iter_t(*this, size_);
  }

  bool full() const
  {
    return size_ == max_size_;
  }

  size_t size() const
  {
    return size_;
  }

  T& at(size_t i) const
  {
    return *iter_t(*this, i);
  }

private:
  std::unique_ptr<T[]> buf_;
  const size_t max_size_;
  size_t head_ = 0;
  size_t size_ = 0;

  friend class RingBufIter<T>;
};

// This should be nested inside RingBuf, but older cling doesn't like a nested
// class of a template class. Nesting doesn't work in root 6.12 on cori; does
// work in 6.16 on dellite.
template<typename T>
class RingBufIter : relational::tag {
public:
  RingBufIter(const RingBuf<T>& ring, int pos) :
    ring_(ring), pos_(pos) { }

  bool operator==(const RingBufIter<T>& rhs) const
  {
    return pos_ == rhs.pos_;
  }

  // // yes this is necessary
  // bool operator!=(const RingBufIter<T>& rhs) const
  // {
  //   return !(*this == rhs);
  // }

  bool operator<(const RingBufIter<T>& rhs) const
  {
    return pos_ < rhs.pos_;
  }

  RingBufIter& operator++()
  {
    ++pos_;
    return *this;
  }

  RingBufIter<T> prev()
  {
    return {ring_, pos_ + 1};
  }

  RingBufIter<T> next()
  {
    return {ring_, pos_ - 1};
  }

  T& operator*() const
  {
    int idx = ring_.head_ - pos_ - 1;
    if (idx < 0)
      idx += ring_.max_size_;

    return ring_.buf_[idx];
  }

private:
  const RingBuf<T>& ring_;
  size_t pos_;
};
