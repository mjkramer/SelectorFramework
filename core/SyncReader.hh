#pragma once

#include "BaseIO.hh"
#include "Kernel.hh"

#include <TChain.h>

#include <memory>
#include <vector>

template <class TreeT>          // TreeT <: TreeBase
class SyncReader : public Algorithm {
public:
  using Data = TreeT;

  // being a template ctor, this must be inside the class definition
  template <class... DataArgs>
  SyncReader(std::initializer_list<const char*> chainNames, DataArgs&&... data_args) :
    data(std::forward<DataArgs>(data_args)...),
    mgr(BranchManager::IOMode::IN)
  {
    for (const auto name : chainNames)
      chains.emplace_back(std::make_unique<TChain>(name));
  }

  void load(const std::vector<std::string>& inFiles) override;
  Algorithm::Status execute() override;

  bool ready() const { return ready_; }
  bool isReader() const override { return true; }

  SyncReader& setMaxEvents(size_t n);
  SyncReader& setReportInterval(size_t n);

  TreeT data;
  const Data& getData() const { return data; }

protected:
  BranchManager mgr;
  std::vector<std::unique_ptr<TChain>> chains;
  size_t entry = 0;
  size_t maxEvents = 0;
  size_t reportInterval = 0;
  bool ready_ = false;
};

