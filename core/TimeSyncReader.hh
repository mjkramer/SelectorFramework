#pragma once

#include "Clock.hh"
#include "SyncReader.hh"
#include "Util.hh"

enum class ClockMode { ClockReader, ClockWriter };

template <class TreeT>          // TreeT <: TreeBase
class TimeSyncReader : public SyncReader<TreeT> {
  static constexpr float DEFAULT_LEADTIME_US = 2000;
  static constexpr float DEFAULT_GAP_THRESHOLD_US = 10e6;

public:
  // being a template ctor, this must be inside the class definition
  template <class... DataArgs>
  TimeSyncReader(std::initializer_list<const char*> chainNames,
                 ClockMode clockMode = ClockMode::ClockReader,
                 DataArgs&&... data_args) :
    SyncReader<TreeT>(chainNames, std::forward<DataArgs>(data_args)...),
    clockMode(clockMode) {}

  void connect(Pipeline& pipeline) override;
  Algorithm::Status execute() override;

  bool prefetching() { return prefetching_; }

  virtual Time timeInTree() = 0;

protected:
  float leadtime_us = DEFAULT_LEADTIME_US;
  float gapThreshold_us = DEFAULT_GAP_THRESHOLD_US;

private:
  bool prefetching_ = false;
  ClockMode clockMode = ClockMode::ClockWriter;

  Clock* clock;
  Time prefetchStart;
  Time prevTime;
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
