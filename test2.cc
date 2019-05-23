#include "classes2.cc"

// template <int NChain = 1>
// class SeqReader : public Algorithm {
// public:
//   SeqReader()
// }

class RecReader : public Algorithm {
public:
  RecReader();
  void initialize(const Pipeline& pipeline,
                  const std::vector<std::string>& inFiles) override;
  Algorithm::Status execute() override;

  struct {
    // RecHeader
    Short_t detector;
    UInt_t triggerType, triggerTimeSec, triggerTimeNanoSec;
    Float_t energy;

    // CalibStats
    Float_t nHit, NominalCharge;
    Float_t Quadrant, MaxQ, MaxQ_2inchPMT, time_PSD, time_PSD1;
  } data;

private:
  TChain chainRec, chainCS;
  int entry = 0;
};

RecReader::RecReader() :
  chainRec("/Event/Rec/AdSimple"),
  chainCS("/Event/Data/CalibStats") { }

void RecReader::initialize(const Pipeline& _pipeline,
                           const std::vector<std::string>& inFiles)
{
  util::initChain(chainRec, inFiles);
  util::initChain(chainCS, inFiles);

  chainRec.AddFriend(&chainCS);

#define THIS_CHAIN chainRec
#define THIS_DATA data
  BR(detector);
  BR(triggerType); BR(triggerTimeSec); BR(triggerTimeNanoSec);
  BR(energy);

  BR(nHit); BR(NominalCharge);
  BR(Quadrant); BR(MaxQ); BR(MaxQ_2inchPMT); BR(time_PSD); BR(time_PSD1);
#undef THIS_CHAIN
#undef THIS_DATA
}

Algorithm::Status RecReader::execute()
{
  if (chainRec.GetEntry(entry)) {
    ++entry;
    return Algorithm::Status::Continue;
  } else
    return Algorithm::Status::EndOfFile;
}
