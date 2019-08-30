#pragma once

#include <iostream>

#include "Kernel.cc"

class SeqReader : public Algorithm {
public:
  SeqReader(std::initializer_list<const char*> chainNames);
  void load(const std::vector<std::string>& inFiles) override;
  Algorithm::Status execute() override;

  SeqReader& setMaxEvents(size_t n);
  SeqReader& setReportInterval(size_t n);

protected:
  virtual void initBranches() { };
  void initBranch(const char* name, void* ptr);

  std::vector<std::unique_ptr<TChain>> chains;
  size_t entry = 0;
  size_t maxEvents = 0;
  size_t reportInterval = 0;
};

SeqReader::SeqReader(std::initializer_list<const char*> chainNames)
{
  for (const auto name : chainNames)
    chains.emplace_back(util::make_unique<TChain>(name));
}

void SeqReader::load(const std::vector<std::string>& inFiles)
{
  for (size_t i = 0; i < chains.size(); ++i) {
    util::initChain(*chains[i], inFiles);
    if (i > 0)
      chains[0]->AddFriend(chains[i].get());
  }

  initBranches();
}

Algorithm::Status SeqReader::execute()
{
  bool proceed = maxEvents == 0 || entry < maxEvents;
  if (proceed && chains[0]->GetEntry(entry)) {
    if (reportInterval && entry % reportInterval == 0)
      std::cout << "---------- Event " << Form("%7zu", entry) << " ----------" << std::endl;
    ++entry;
    return Status::Continue;
  } else
    return Status::EndOfFile;
}

SeqReader& SeqReader::setMaxEvents(size_t n)
{
  maxEvents = n;
  return *this;
}

SeqReader& SeqReader::setReportInterval(size_t n)
{
  reportInterval = n;
  return *this;
}

void SeqReader::initBranch(const char* name, void* ptr)
{
  chains[0]->SetBranchStatus(name, true);
  chains[0]->SetBranchAddress(name, ptr);
}

#define BR(branch) initBranch(#branch, &data.branch)
