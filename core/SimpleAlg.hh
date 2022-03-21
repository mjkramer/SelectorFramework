#pragma once

#include "Kernel.hh"

#include <optional>

template <class ReaderT>     // assuming ReaderT { _& getData(); bool ready(); }
using algdata_t = std::decay_t<decltype(((ReaderT*)0)->getData())>;

// template <class ReaderT>
// using algfunc_t = std::function<Algorithm::Status (typename ReaderT::Data*)>;
template <class ReaderT>
using algfunc_t = Algorithm::Status(const algdata_t<ReaderT> &);

template <class ReaderT, class TagT = int>
class SimpleAlg : public Algorithm {
public:
  using Pred = ::Pipeline::Pred<ReaderT>;

  SimpleAlg() {};
  SimpleAlg(Pipeline::Pred<ReaderT> pred);
  SimpleAlg(TagT tag);

  void connect(Pipeline& pipeline) override;
  Algorithm::Status execute() override;

  virtual Algorithm::Status consume(const algdata_t<ReaderT>& data) = 0;

  virtual int rawTag() const override { return this->tag_ ? static_cast<int>(*tag_): 0; }
  TagT tag() { return tag_? *tag_: TagT(0); }

protected:
  const ReaderT* reader = nullptr;
  Pred pred_;

private:
  std::optional<TagT> tag_ = std::nullopt;
};

template <class ReaderT, algfunc_t<ReaderT> func, class TagT = int>
class PureAlg : public SimpleAlg<ReaderT, TagT> {
public:
  Algorithm::Status consume(const algdata_t<ReaderT>& data) override
  {
    return func(data);
  };
};

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
