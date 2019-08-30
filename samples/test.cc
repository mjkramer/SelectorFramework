#include "../core/Kernel.cc"
#include "../core/SeqReader.cc"

#include <initializer_list>
#include <iostream>

class SingReader : public SeqReader {
public:
  SingReader();
  void dump() const;
  Algorithm::Status execute() override; // XXX

  struct Data {
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

void SingReader::dump() const
{
  std::cout << std::endl;
#define DU(b) std::cout << #b " = " << data.b << std::endl
  DU(detector);
  DU(triggerType); DU(triggerTimeSec); DU(triggerTimeNanoSec);
  DU(energy);

  DU(nHit); DU(NominalCharge);
  DU(Quadrant); DU(MaxQ); DU(MaxQ_2inchPMT); DU(time_PSD); DU(time_PSD1);
#undef DU
  std::cout << std::endl;
}

// ----------------------------------------------------------------------

class DumperAlg : public Algorithm {
public:
  void connect(Pipeline& pipeline) override;
  Status execute() override;

private:
  const decltype(SingReader::data)* singData;
};

void DumperAlg::connect(Pipeline& pipeline)
{
  singData = &pipeline.getAlg<SingReader>().data;
}

Algorithm::Status DumperAlg::execute()
{
  std::cout << "e = " << singData->energy << ", nHit = " << singData->nHit << "\n\n";
  return Status::Continue;
}

// ----------------------------------------------------------------------

class CrossTriggerAlg : public Algorithm {
public:
  void connect(Pipeline& pipeline) override;
  Status execute() override;

private:
  const SingReader::Data* singData;
};

void CrossTriggerAlg::connect(Pipeline& pipeline)
{
  singData = &pipeline.getAlg<SingReader>().data;
}

Algorithm::Status CrossTriggerAlg::execute()
{
  const UInt_t crossMask = 0x10000002;
  if ((singData->triggerType & crossMask) == crossMask) {
    std::cout << "Cross trigger, skipping!" << std::endl << std::endl;
    return Status::SkipToNext;
  }
  return Status::Continue;
}

// ----------------------------------------------------------------------

template <algfunc_t<SingReader> func>
using SingReaderFunc = PureAlg<SingReader, func>;

Algorithm::Status extraDumper(const SingReader::Data& data)
{
  std::cout << "NominalCharge = " << data.NominalCharge << "\n\n";
  return Algorithm::Status::Continue;
}

using ExtraDumperAlg = PureAlg<SingReader, extraDumper>;

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

  p.makeAlg<SingReader>().setMaxEvents(100);
  p.makeAlg<CrossTriggerAlg>();
  p.makeAlg<DumperAlg>();
  p.makeAlg<ExtraDumperAlg>();

  p.process(deffiles());
}
