#pragma once

#include "Kernel.hh"
#include "RingBuf.hh"
#include "SimpleAlg.hh"

template <class ReaderT, class TagT = int>
class EventBuf : public SimpleAlg<ReaderT, TagT> {
public:
  using Data = algdata_t<ReaderT>;
  using Iter = typename RingBuf<Data>::iter_t;

  using SimpleAlg<ReaderT, TagT>::SimpleAlg;

  Algorithm::Status consume(const Data& data) override;
  void postExecute() override;

  void resize(size_t N);

  // EventBuf is also a Reader itself:
  bool ready() const;
  const Data& getData() const;
  const Data& latest() const;
  const Data& pending() const;
  Iter iter() const;

  virtual bool keep() const { return true; }
  virtual bool enough() const = 0;

private:
  RingBuf<Data> buf_;
  size_t pending_ = 0;
  bool ready_ = false;
};

template <class EventBufT, class TagT = int>
class BufferedSimpleAlg : public SimpleAlg<EventBufT, TagT> {
public:
  using Data = typename EventBufT::Data;
  using Iter = typename EventBufT::Iter;

  using SimpleAlg<EventBufT, TagT>::SimpleAlg;

  Algorithm::Status consume(const Data&) override;
  Algorithm::Status execute() override;
  virtual Algorithm::Status consume_iter(Iter) = 0;
};

template <class ReaderT, class TagT>
void EventBuf<ReaderT, TagT>::resize(size_t N)
{
  buf_.resize(N);
}

// EventBuf is also a Reader itself:
template <class ReaderT, class TagT>
inline
bool EventBuf<ReaderT, TagT>::ready() const
{
  return ready_;
}

template <class ReaderT, class TagT>
inline
const typename EventBuf<ReaderT, TagT>::Data&
EventBuf<ReaderT, TagT>::getData() const
{
  return pending();
};

template <class ReaderT, class TagT>
inline
const typename EventBuf<ReaderT, TagT>::Data&
EventBuf<ReaderT, TagT>::latest() const
{
  return buf_.top();
}

template <class ReaderT, class TagT>
inline
const typename EventBuf<ReaderT, TagT>::Data&
EventBuf<ReaderT, TagT>::pending() const
{
  return *iter();
};

template <class ReaderT, class TagT>
inline
typename EventBuf<ReaderT, TagT>::Iter EventBuf<ReaderT, TagT>::iter() const
{
  return buf_.begin() + (pending_ - 1);
}
