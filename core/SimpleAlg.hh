#pragma once

#include "Kernel.hh"

template <class ReaderT>     // assuming ReaderT { _& getData(); bool ready(); }
using algdata_t = std::decay_t<decltype(((ReaderT*)0)->getData())>;

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

  virtual int rawTag() const override { return int(*tag_); }
  TagT tag() { return *tag_; }

protected:
  const ReaderT* reader = nullptr;

private:
  Pred pred_;
  std::optional<TagT> tag_;
};
