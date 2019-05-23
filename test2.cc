#include "classes2.cc"

#include <initializer_list>
#include <iostream>

template <int NChain = 1>
class SeqReader : public Algorithm {
public:
  SeqReader(std::initializer_list<const char*> chainNames);
  void initialize(Pipeline& pipeline,
                  const std::vector<std::string>& inFiles) override;
  Algorithm::Status execute() override;

protected:
  virtual void initBranches() { };

  TChain chains[NChain];
  int entry = 0;
};

template <int NChain>
SeqReader<NChain>::SeqReader(std::initializer_list<const char*> chainNames)
{
  for (int i = 0; i < NChain; ++i)
    new(&chains[i]) TChain(chainNames.begin()[i]);
}

template <int NChain>
void SeqReader<NChain>::initialize(Pipeline& _pipeline,
                                   const std::vector<std::string>& inFiles)
{
  for (int i = 0; i < NChain; ++i) {
    util::initChain(chains[i], inFiles);
    if (i > 0)
      chains[0].AddFriend(&chains[i]);
  }

  initBranches();
}

template <int NChain>
Algorithm::Status SeqReader<NChain>::execute()
{
  // std::cout << entry << std::endl;
  // std::cout << chains[0].GetEntry(entry) << std::endl;

  if (chains[0].GetEntry(entry) && entry < 100) { // XXX
    ++entry;
    return Status::Continue;
  } else
    return Status::EndOfFile;
}

class SingReader : public SeqReader<2> {
public:
  SingReader();
  void dump();
  Algorithm::Status execute() override; // XXX

  struct {
    // RecHeader
    Short_t detector;
    UInt_t triggerType, triggerTimeSec, triggerTimeNanoSec;
    Float_t energy;

    // CalibStats
    Int_t nHit;
    Float_t NominalCharge;
    Float_t Quadrant, MaxQ, MaxQ_2inchPMT, time_PSD, time_PSD1;
  } data;

private:
  void initBranches() override;
};

Algorithm::Status SingReader::execute()
{
  auto status = SeqReader::execute();
  if (status == Status::Continue)
    dump();
  return status;
}

SingReader::SingReader() :
  SeqReader{"/Event/Rec/AdSimple", "/Event/Data/CalibStats"} { }

void SingReader::initBranches()
{
  BR(detector);
  BR(triggerType); BR(triggerTimeSec); BR(triggerTimeNanoSec);
  BR(energy);

  BR(nHit); BR(NominalCharge);
  BR(Quadrant); BR(MaxQ); BR(MaxQ_2inchPMT); BR(time_PSD); BR(time_PSD1);
}

void SingReader::dump()
{
#define DU(b) std::cout << #b " = " << data.b << std::endl
  DU(detector);
  DU(triggerType); DU(triggerTimeSec); DU(triggerTimeNanoSec);
  DU(energy);

  DU(nHit); DU(NominalCharge);
  DU(Quadrant); DU(MaxQ); DU(MaxQ_2inchPMT); DU(time_PSD); DU(time_PSD1);
#undef DU
  std::cout << std::endl;
}

class DumperAlg : public Algorithm {
public:
  void initialize(Pipeline& pipeline,
                  const std::vector<std::string>& inFiles) override;
  Status execute() override;

private:
  const decltype(SingReader::data)* singData;
};

void DumperAlg::initialize(Pipeline& pipeline,
                           const std::vector<std::string>& _inFiles)
{
  singData = &pipeline.getAlg<SingReader>("SingReader").data;
}

Algorithm::Status DumperAlg::execute()
{
  std::cout << "e = " << singData->energy << ", nHit = " << singData->nHit << std::endl;
  return Status::Continue;
}

std::vector<std::string> deffiles()
{
  return {
          "/global/projecta/projectdirs/dayabay/data/dropbox/p17b/lz3.skim.6/recon.Neutrino.0021221.Physics.EH1-Merged.P17B-P._0001.root",
          "/global/projecta/projectdirs/dayabay/data/dropbox/p17b/lz3.skim.5/recon.Neutrino.0021221.Physics.EH1-Merged.P17B-P._0002.root"
  };
}

void runTest()
{
  Pipeline p;

  p.addAlg("SingReader", new SingReader); // XXX leak, use unique_ptr
  p.addAlg("Dumper", new DumperAlg);

  p.process(deffiles());
}