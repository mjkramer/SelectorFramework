#pragma once

#include "Assert.hh"
#include "Util.hh"              // relational

#include <algorithm>
#include <iostream>
#include <memory>

template <typename T>
class RingBufIter;

template<typename T>
class RingBuf {
  static constexpr size_t DEFAULT_SIZE = 1000;

public:
  using iter_t = RingBufIter<T>;

  RingBuf(size_t N = DEFAULT_SIZE);
  RingBuf(RingBuf&&) = default;

  void resize(size_t N);
  void put(T item);
  // insert is O(i), use a different data structure if inserting frequently.
  // Puts item AFTER (i.e. newer than, put'd after) the one you get from at(i).
  // Thus insert(0, _) is equivalent to put(_).
  void insert(size_t i, T item);
  const T& top() const;
  iter_t begin() const;
  iter_t end() const;
  bool full() const;
  size_t size() const;
  T& at(size_t i) const;
  void dump() const;            // for debugging

private:
  void advance_head();
  size_t raw_idx(int i) const;

  std::unique_ptr<T[]> buf_;
  size_t max_size_;
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
  RingBufIter(const RingBuf<T>* ring = nullptr, size_t pos = 0) :
    ring_(ring), pos_(pos) { }

  bool operator==(const RingBufIter<T>& rhs) const;
  bool operator<(const RingBufIter<T>& rhs) const;
  RingBufIter& operator++();
  RingBufIter<T> earlier();
  RingBufIter<T> later();
  RingBufIter<T> operator+(int n) const;
  T& operator*() const;
  T* operator->() const;

private:
  const RingBuf<T>* ring_;
  size_t pos_;
};

// -----------------------------------------------------------------------------

template<typename T>
RingBuf<T>::RingBuf(size_t N)
{
  resize(N);
}

template<typename T>
void RingBuf<T>::resize(size_t N)
{
  buf_ = std::make_unique<T[]>(N);
  max_size_ = N;
}

template<typename T>
inline
void RingBuf<T>::advance_head()
{
  head_ = (head_ + 1) % max_size_;
  size_ = std::min(size_ + 1, max_size_);
}

template<typename T>
inline
void RingBuf<T>::put(T item)
{
  buf_[head_] = item;
  advance_head();
}

template<typename T>
inline
size_t RingBuf<T>::raw_idx(int i) const
{
  const int idx = head_ - 1 - i; // subtract 1 since head_ is where we *put*
  return idx + ((idx < 0) ? max_size_ : 0);
}

template<typename T>
inline
T& RingBuf<T>::at(size_t i) const
{
  return buf_[raw_idx(i)];
}

// Battle-tested by tests/stress_ringbuf.py
template<typename T>
inline
void RingBuf<T>::insert(size_t i, T item)
{
  ASSERT(i < size_);

  // Subtract 1 since we're inserting at point *newer than* @i
  const size_t dest_idx = raw_idx(i-1);   // i = 0 => head_

  // Shift up the items that physically lie below head_
  if (head_ != 0) {
    const size_t first = (dest_idx <= head_) ? dest_idx : 0;
    std::copy_backward(&buf_[first], &buf_[head_], &buf_[head_+1]);
  }

  // Shift up those that lie above head_
  if (head_ != max_size_-1 and dest_idx > head_) {
    buf_[0] = buf_[max_size_-1];
    std::copy_backward(&buf_[dest_idx], &buf_[max_size_-1], &buf_[max_size_]);
  }

  buf_[dest_idx] = item;
  advance_head();
}

template<typename T>
inline
const T& RingBuf<T>::top() const
{
  return at(0);
}

template<typename T>
inline
typename RingBuf<T>::iter_t RingBuf<T>::begin() const
{
  return iter_t(this, 0);
}

template<typename T>
inline
typename RingBuf<T>::iter_t RingBuf<T>::end() const
{
  return iter_t(this, size_);
}

template<typename T>
inline
bool RingBuf<T>::full() const
{
  return size_ == max_size_;
}

template<typename T>
inline
size_t RingBuf<T>::size() const
{
  return size_;
}

// -----------------------------------------------------------------------------

template <typename T>
inline
bool RingBufIter<T>::operator==(const RingBufIter<T>& rhs) const
{
  return pos_ == rhs.pos_;
}

// // yes this is necessary
// bool operator!=(const RingBufIter<T>& rhs) const
// {
//   return !(*this == rhs);
// }

template <typename T>
inline
bool RingBufIter<T>::operator<(const RingBufIter<T>& rhs) const
{
  return pos_ < rhs.pos_;
}

template <typename T>
inline
RingBufIter<T>& RingBufIter<T>::operator++()
{
  ++pos_;
  return *this;
}

template <typename T>
inline
RingBufIter<T> RingBufIter<T>::earlier()
{
  return {ring_, pos_ + 1};
}

template <typename T>
inline
RingBufIter<T> RingBufIter<T>::later()
{
  return {ring_, pos_ - 1};
}

template <typename T>
inline
RingBufIter<T> RingBufIter<T>::operator+(int n) const
{
  return {ring_, pos_ + n};
}

template <typename T>
inline
T& RingBufIter<T>::operator*() const
{
  return ring_->at(pos_);
}

template <typename T>
inline
T* RingBufIter<T>::operator->() const
{
  return &*(*this);
}

// --------------------------------------------------------------------------------

// This gives the "raw" contents, whereas __str__() from cppyy gives us the
// "visible" contents, i.e. { at(0), at(1), ... }
template<typename T>
void RingBuf<T>::dump() const
{
  std::cout << "[ ";
  for (size_t i = 0; i < size_; ++i) {
    if (i == raw_idx(0))
      std::cout << "*";
    std::cout << buf_[i] << " ";
  }
  std::cout << "]" << std::endl;
}
