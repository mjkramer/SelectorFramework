#pragma once

#include "Kernel.cc"

class SeqReader : public Algorithm {
public:
  SeqReader(std::initializer_list<const char*> chainNames);
  void load(const std::vector<std::string>& inFiles) override;
  Algorithm::Status execute() override;

  SeqReader& setMaxEvents(size_t n);

protected:
  virtual void initBranches() { };

  std::vector<std::unique_ptr<TChain>> chains;
  size_t entry = 0;
  size_t maxEvents = 0;
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

#define BR(branch)                                    \
  chains[0]->SetBranchStatus(#branch, true);          \
  chains[0]->SetBranchAddress(#branch, &data.branch)
