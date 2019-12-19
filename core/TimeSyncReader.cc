#pragma once

#include "Clock.cc"
#include "SyncReader.cc"
#include "Util.cc"

template <class TreeT>          // TreeT <: TreeBase
class TimeSyncReader : public SyncReader<TreeT> {
  static constexpr float DEFAULT_EPSILON_US = 10;

public:
  enum class ClockMode { ClockReader, ClockWriter };

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

template <class TreeT>
void TimeSyncReader<TreeT>::connect(Pipeline& pipeline)
{
  clock = pipeline.getTool<Clock>();

  if (clockMode == ClockMode::ClockWriter)
    clock->registerWriter(this);

  SyncReader<TreeT>::connect(pipeline);
}


template <class TreeT>
Algorithm::Status TimeSyncReader<TreeT>::execute()
{
  const bool first = this->entry == 0;
  bool fetch = false;

  if (first)
    fetch = true;

  // Event was published on last execution cycle
  if (this->ready())
    fetch = true;

  if (fetch) {
    auto status = SyncReader<TreeT>::execute();
    if (status == Algorithm::Status::EndOfFile) {
      prefetching_ = false;
      if (clockMode == ClockMode::ClockWriter)
        clock->signalTheEnd();
      return status;
    }

    // Don't publish event until we verify that clock writer is "caught up"
    // which we check in the "if ClockReader" block"
    // (SyncReader::execute sets ready_ to true)
    this->ready_ = false;
  }

  if (first && prefetch_us) {
    prefetching_ = true;
    beginTime = timeInTree();
  }

  Time ourTime = timeInTree();

  if (prefetching()) {
    this->ready_ = true;
    if (ourTime.diff_us(beginTime) > prefetch_us) {
      prefetching_ = false;
    }
  }

  if (clockMode == ClockMode::ClockReader) {
    Time globalTime = clock->current();

    // Global clock is catching up. Start publishing/fetching.
    if (ourTime.diff_us(globalTime) < epsilon_us || clock->atTheEnd())
      this->ready_ = true;

    // Global clock is perilously close. Loop till we're sufficiently ahead.

    if (prefetch_us && ourTime.diff_us(globalTime) < epsilon_us/2) {
      prefetching_ = true;
      // beginTime = timeInTree();
      beginTime = globalTime;
    }
  }

  else {                      // ClockWriter
    clock->update(ourTime);
    this->ready_ = true;
  }

  return Algorithm::Status::Continue;
}

template <class TreeT>
TimeSyncReader<TreeT>& TimeSyncReader<TreeT>::setClockMode(ClockMode mode)
{
  clockMode = mode;
  return *this;
}

template <class TreeT>
TimeSyncReader<TreeT>& TimeSyncReader<TreeT>::setPrefetch_us(float pf_us)
{
  prefetch_us = pf_us;
  return *this;
}

template <class TreeT>
TimeSyncReader<TreeT>& TimeSyncReader<TreeT>::setEpsilon_us(float eps_us)
{
  epsilon_us = eps_us;
  return *this;
}

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
  if (reader->prefetching())
    return Status::SkipToNext;
  else
    return Status::Continue;
}
