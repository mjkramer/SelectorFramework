#pragma once

#include "BaseIO.hh"
#include "Kernel.hh"
#include "Util.hh"

#include <TChain.h>

#include <iostream>
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

  virtual void postReadCallback() { };

protected:
  BranchManager mgr;
  std::vector<std::unique_ptr<TChain>> chains;
  size_t entry = 0;
  size_t maxEvents = 0;
  size_t reportInterval = 0;
  bool ready_ = false;
  size_t iFile = 0;
};

template <class TreeT>
void SyncReader<TreeT>::load(const std::vector<std::string>& inFiles)
{
  for (size_t i = 0; i < chains.size(); ++i) {
    util::initChain(*chains[i], inFiles);
    if (i > 0)
      chains[0]->AddFriend(chains[i].get());
  }

  mgr.tree = chains[0].get();
  data.setManager(&mgr);
  data.initBranches();
}

template <class TreeT>
Algorithm::Status SyncReader<TreeT>::execute()
{
  bool proceed = maxEvents == 0 || entry < maxEvents;
  if (proceed && chains[0]->GetEntry(entry)) {
    if (reportInterval && entry % reportInterval == 0)
      std::cout << "---------- Event " << Form("%7zu", entry) << " ----------" << std::endl;

    size_t current = chains[0]->GetTreeNumber();
    if (current != iFile) {
      iFile = current;
      pipe().notifyFileChanged(this, current);
    }

    ++entry;
    ready_ = true;
    postReadCallback();
    return Status::Continue;
  } else {
    ready_ = false;
    return Status::EndOfFile;
  }
}

template <class TreeT>
SyncReader<TreeT>& SyncReader<TreeT>::setMaxEvents(size_t n)
{
  maxEvents = n;
  return *this;
}

template <class TreeT>
SyncReader<TreeT>& SyncReader<TreeT>::setReportInterval(size_t n)
{
  reportInterval = n;
  return *this;
}
