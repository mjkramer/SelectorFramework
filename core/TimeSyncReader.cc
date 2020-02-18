#include "TimeSyncReader.hh"

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

  if (first && leadtime_us) {
    prefetching_ = true;
    prefetchStart = timeInTree();
  }

  Time ourTime = timeInTree();

  if (prefetching_) {
    this->ready_ = true;
    if (ourTime.diff_us(prefetchStart) > leadtime_us) {
      prefetching_ = false;
    }
  }

  if (clockMode == ClockMode::ClockReader) {
    Time globalTime = clock->current();

    // Global clock is catching up. Start publishing/fetching.
    if (ourTime.diff_us(globalTime) < leadtime_us || clock->atTheEnd())
      this->ready_ = true;

    // Global clock is perilously close. Loop till we're sufficiently ahead.
    if (leadtime_us && ourTime.diff_us(globalTime) < leadtime_us/2) {
      prefetching_ = true;
      prefetchStart = globalTime;
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
TimeSyncReader<TreeT>& TimeSyncReader<TreeT>::setLeadtime_us(float us)
{
  leadtime_us = us;
  return *this;
}

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
