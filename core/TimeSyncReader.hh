#pragma once

#include "Clock.hh"
#include "SyncReader.hh"
#include "Util.hh"

template <class TreeT>          // TreeT <: TreeBase
class TimeSyncReader : public SyncReader<TreeT> {
  static constexpr float DEFAULT_EPSILON_US = 10;

public:
  enum class ClockMode { ClockReader, ClockWriter };

  // being a template ctor, this must be inside the class definition
  template <class... DataArgs>
  TimeSyncReader(std::initializer_list<const char*> chainNames,
                 bool clockWriter = false,
                 DataArgs&&... data_args) :
    SyncReader<TreeT>(chainNames, std::forward<DataArgs>(data_args)...)
  {
    if (clockWriter) {
      setClockMode(ClockMode::ClockWriter);
    } else {
      setClockMode(ClockMode::ClockReader);
      // Reasonable defaults:
      setEpsilon_us(2000);
      setPrefetch_us(2000);
    }
  }

  void connect(Pipeline& pipeline) override;
  Algorithm::Status execute() override;

  bool prefetching() { return prefetching_; }

  virtual Time timeInTree() = 0;

  TimeSyncReader& setClockMode(ClockMode);
  TimeSyncReader& setPrefetch_us(float);
  TimeSyncReader& setEpsilon_us(float);

private:
  float epsilon_us = 0;
  float prefetch_us = 0;
  bool prefetching_ = false;
  ClockMode clockMode = ClockMode::ClockWriter;

  Clock* clock;
  Time beginTime;
};

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
