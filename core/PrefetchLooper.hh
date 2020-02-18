#pragma once

#include "Kernel.hh"

template <class TagT, class Enable = void>
struct PrefetchLooperBase;

template <class TagT>
struct PrefetchLooperBase<TagT, std::enable_if_t<std::is_same_v<TagT, void>>> {
};

template <class TagT>
struct PrefetchLooperBase<TagT, std::enable_if_t<not std::is_same_v<TagT, void>>> {
  TagT tag;
};

// Use this to skip over downstream algs while upstream reader is still
// prefetching (e.g. to delay IBD selection until we've got a buffer of muons)
template <class ReaderT, class TagT = void>
class PrefetchLooper : public Algorithm, public PrefetchLooperBase<TagT> {
public:
  Algorithm::Status execute() override;

  template <class U = TagT, std::enable_if_t<std::is_same_v<U, void>, int> = 0>
  PrefetchLooper() {}

  template <class U = TagT, std::enable_if_t<not std::is_same_v<U, void>, int> = 0>
  PrefetchLooper(U tag)
  {
    this->tag = tag;
  }

  void connect(Pipeline& p) override
  {
    do_connect(p);
  }

private:
  ReaderT* reader;

  template <class U = TagT, std::enable_if_t<std::is_same_v<U, void>, int> = 0>
  void do_connect(Pipeline& p)
  {
    reader = p.getAlg<ReaderT>();
  }

  template <class U = TagT, std::enable_if_t<not std::is_same_v<U, void>, int> = 0>
  void do_connect(Pipeline& p)
  {
    reader = p.getAlg<ReaderT>(this->tag);
  }
};

// template <class ReaderT, class TagT>
// PrefetchLooper<ReaderT>(TagT t) -> PrefetchLooper<ReaderT, TagT>;

template <class ReaderT, class TagT>
Algorithm::Status PrefetchLooper<ReaderT, TagT>::execute()
{
  if (reader->is_prefetching())
    return Status::SkipToNext;
  else
    return Status::Continue;
}
