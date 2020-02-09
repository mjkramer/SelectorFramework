#pragma once

#include "Util.hh"              // relational

#include <memory>

template <typename T>
class RingBufIter;

template<typename T>
class RingBuf {
  static constexpr size_t DEFAULT_SIZE = 1000;

public:
  using iter_t = RingBufIter<T>;

  explicit RingBuf(size_t N = DEFAULT_SIZE);

  void resize(size_t N);
  void put(T item);
  const T& top() const;
  iter_t begin() const;
  iter_t end() const;
  bool full() const;
  size_t size() const;
  T& at(size_t i) const;

private:
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
void RingBuf<T>::put(T item)
{
  buf_[head_] = item;
  head_ = (head_ + 1) % max_size_;
  size_ = std::min(size_ + 1, max_size_);
}

template<typename T>
inline
const T& RingBuf<T>::top() const
{
  return *begin();
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

template<typename T>
inline
T& RingBuf<T>::at(size_t i) const
{
  return *iter_t(this, i);
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
  int idx = ring_->head_ - pos_ - 1;
  if (idx < 0)
    idx += ring_->max_size_;

  return ring_->buf_[idx];
}

template <typename T>
inline
T* RingBufIter<T>::operator->() const
{
  return &*(*this);
}

