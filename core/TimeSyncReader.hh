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
