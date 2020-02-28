#pragma once

#include "Clock.hh"
#include "PrefetchLooper.hh"
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

  virtual Time timeInTree() = 0;

  bool is_prefetching() { return prefetching; };

protected:
  float leadtime_us = DEFAULT_LEADTIME_US;
  float gapThreshold_us = DEFAULT_GAP_THRESHOLD_US;

private:
  bool prefetching = false;
  ClockMode clockMode = ClockMode::ClockWriter;

  Clock* clock;
  Time prefetchStart;
  Time prevTime;
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

  // ready_ implies that event was published on last execution, in which case
  // we're ready to fetch a new one
  if (first || this->ready_) {
    // NB: SyncReader::execute sets ready_ to true
    const auto status = SyncReader<TreeT>::execute(); // fetch next event
    if (status == Algorithm::Status::EndOfFile) {
      prefetching = false;
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
      prefetching = true;
      prefetchStart = notFarEnough ? clock->current() : timeInTree();
    }

    else if (prefetching) {
      if (dtPrefetch_us > leadtime_us) {
        prefetching = false;
      }
    }

    else if (tooFarAhead and not clock->atTheEnd()) {
      this->ready_ = false;           // wait for ClockWriter to catch up
    }
  }

  prevTime = timeInTree();
  return Algorithm::Status::Continue;
}
