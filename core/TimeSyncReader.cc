#pragma once

#include "Clock.cc"
#include "SyncReader.cc"
#include "Util.cc"

template <class TreeT>          // TreeT <: TreeBase
class TimeSyncReader : public SyncReader<TreeT> {
  static constexpr float DEFAULT_EPSILON_US = 10;

public:
  enum class ClockMode { ClockReader, ClockWriter };

  using SyncReader<TreeT>::SyncReader;

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
      return status;
    }

    // Don't publish event until we verify that singles tree is "caught up"
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
    if (ourTime.diff_us(globalTime) < epsilon_us)
      this->ready_ = true;

    // Global clock is perilously close. Loop till we're sufficiently ahead.

    if (prefetch_us && ourTime.diff_us(globalTime) < epsilon_us/2)
      prefetching_ = true;
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

// Use this to skip over downstream algs while upstream reader is still
// prefetching (e.g. to delay IBD selection until we've got a buffer of muons)
template <class ReaderT>
class PrefetchLooper : public Algorithm {
public:
  void connect(Pipeline& p) override;
  Algorithm::Status execute() override;

private:
  ReaderT* reader;
};

template <class ReaderT>
void PrefetchLooper<ReaderT>::connect(Pipeline& p)
{
  reader = p.getAlg<ReaderT>();
}

template <class ReaderT>
Algorithm::Status PrefetchLooper<ReaderT>::execute()
{
  if (reader->prefetching())
    return Status::SkipToNext;
  else
    return Status::Continue;
}
