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

  // ready_ implies that event was published on last execution, in which case
  // we're ready to fetch a new one
  if (first || this->ready_) {
    // NB: SyncReader::execute sets ready_ to true
    const auto status = SyncReader<TreeT>::execute(); // fetch next event
    if (status == Algorithm::Status::EndOfFile) {
      prefetching_ = false;
      if (clockMode == ClockMode::ClockWriter)
        clock->signalTheEnd();
      return status;
    }
  }

  if (clockMode == ClockMode::ClockWriter) {
    clock->update(timeInTree());
  }

  else {                        // ClockReader
    const float dtClock_us = timeInTree().diff_us(clock->current());
    const float dtPrev_us = timeInTree().diff_us(prevTime);
    const float dtPrefetch_us = timeInTree().diff_us(prefetchStart);

    const bool foundGap = dtPrev_us > gapThreshold_us;
    const bool tooFarAhead = dtClock_us > leadtime_us;
    const bool notFarEnough = dtClock_us < leadtime_us/2;

    this->ready_ = true;

    if (first || notFarEnough || foundGap) {
      prefetching_ = true;
      prefetchStart = notFarEnough ? clock->current() : timeInTree();
    }

    else if (prefetching_) {
      if (timeInTree().diff_us(prefetchStart) > leadtime_us) {
        prefetching_ = false;
      }
    }

    else if (tooFarAhead) {
      this->ready_ = false;           // wait for ClockWriter to catch up
    }
  }

  prevTime = timeInTree();
  return Algorithm::Status::Continue;
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
