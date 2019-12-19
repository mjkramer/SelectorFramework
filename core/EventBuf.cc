#pragma once

// We should move this to IbdSel?

#include "Kernel.cc"
#include "RingBuf.cc"

template <class ReaderT, class TagT = int>
class EventBuf : public SimpleAlg<ReaderT, TagT> {
public:
  using Data = algdata_t<ReaderT>;
  using Iter = typename RingBuf<Data>::iter_t;

  using SimpleAlg<ReaderT, TagT>::SimpleAlg;

  Algorithm::Status consume(const Data& data) override;
  void postExecute() override;

  void resize(size_t N) { buf_.resize(N); }

  // EventBuf is also a Reader itself:
  bool ready() const { return ready_; }
  const Data& getData() const { return pending(); };

  const Data& latest() const { return buf_.top(); }
  const Data& pending() const { return *iter(); };
  Iter iter() const { return buf_.begin() + (pending_ - 1); }

  virtual bool keep() const { return true; }
  virtual bool enough() const = 0;

private:
  RingBuf<Data> buf_;
  size_t pending_ = 0;
  bool ready_ = false;
};

template <class RT, class TagT>
Algorithm::Status EventBuf<RT, TagT>::consume(const Data &data)
{
  if (keep()) {
    buf_.put(data);
    ++pending_;

    if (enough()) {
      ready_ = true;
    }
  }

  return Algorithm::Status::Continue;
}

template <class RT, class TagT>
void EventBuf<RT, TagT>::postExecute()
{
  if (ready_)                   // released event this cycle?
    --pending_;

  ready_ = false;
}

// ----------------------------------------------------------------------

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

// XXX Need to extract BaseSimpleAlg from SimpleAlg
// BaseSimpleAlg would have no consume(), execute() undefined
// SimpleAlg then gets consume(), BufferedSimpleAlg gets consume_iter()
// For now, just make BufferedSimpleAlg::consume be a noop that never gets called
template <class EventBufT, class TagT>
Algorithm::Status BufferedSimpleAlg<EventBufT, TagT>::consume(const Data&)
{
  throw std::runtime_error("BufferedSimpleAlg::consume should not be called");
}

template <class EventBufT, class TagT>
Algorithm::Status BufferedSimpleAlg<EventBufT, TagT>::execute()
{
  if (this->reader->ready())
    return consume_iter(this->reader->iter());

  else return Algorithm::Status::Continue;
}
