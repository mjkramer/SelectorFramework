#include "EventBuf.hh"

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
