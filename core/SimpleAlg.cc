#include "SimpleAlg.hh"

template <class ReaderT, class TagT>
SimpleAlg<ReaderT, TagT>::SimpleAlg(Pred pred)
  : pred_(std::move(pred)) {}

template <class ReaderT, class TagT>
SimpleAlg<ReaderT, TagT>::SimpleAlg(TagT tag)
  : tag_(tag) {}

template <class ReaderT, class TagT>
void SimpleAlg<ReaderT, TagT>::connect(Pipeline &pipeline) {
  if (pred_) {
    reader = pipeline.getAlg<ReaderT>(pred_);
  } else if (tag_) {
    reader = pipeline.getAlg<ReaderT>(*tag_);
  } else {
    reader = pipeline.getAlg<ReaderT>();
  }
}

template <class ReaderT, class TagT>
Algorithm::Status SimpleAlg<ReaderT, TagT>::execute()
{
  if (reader->ready())
    return consume(reader->getData());

  else return Status::Continue;
}

// -----------------------------------------------------------------------------

// template <class ReaderT>
// using algfunc_t = std::function<Algorithm::Status (typename ReaderT::Data*)>;

template <class ReaderT, algfunc_t<ReaderT> func, class TagT = int>
class PureAlg : public SimpleAlg<ReaderT, TagT> {
public:
  Algorithm::Status consume(const algdata_t<ReaderT>& data) override
  {
    return func(data);
  };
};
