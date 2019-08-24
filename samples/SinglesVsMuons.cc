#include <TH1F.h>

#include "../core/Kernel.cc"
#include "../core/SeqReader.cc"
#include "../core/RingBuf.cc"
#include "../core/Util.cc"

using Status = Algorithm::Status;
using namespace util;

// -----------------------------------------------------------------------------

UInt_t N_MUONS = 1000;          // how far back to remember
UInt_t ISOLATION_US = 200;      // set to 0 to disable multiplicity cut
UInt_t MUON_SEP_US = 10;        // min separation between 2 distinct muons

// -----------------------------------------------------------------------------

class SingReader : public SeqReader {
public:
  SingReader() :
    SeqReader{"/Event/Rec/AdSimple", "/Event/Data/CalibStats"} {};

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

void SingReader::initBranches()
{
  BR(detector);
  BR(triggerType); BR(triggerTimeSec); BR(triggerTimeNanoSec);
  BR(energy);

  BR(nHit); BR(NominalCharge);
  BR(Quadrant); BR(MaxQ); BR(MaxQ_2inchPMT); BR(time_PSD); BR(time_PSD1);
}

// -----------------------------------------------------------------------------

Status crossTriggerAlg(const SingReader::Data* data)
{
  const UInt_t crossMask = 0x10000002;
  bool cross = (data->triggerType & crossMask) == crossMask;

  return vetoIf(cross);
}

using CrossTriggerAlg = PureAlg<SingReader, crossTriggerAlg>;

// -----------------------------------------------------------------------------

Status flasherAlg(const SingReader::Data* data)
{
#define SQ(x) pow(x, 2)
  const auto& e = *data;
  bool flasher = SQ(e.Quadrant) + SQ(e.MaxQ / 0.45) > 1
    || 4*SQ(1 - e.time_PSD) + 1.8*SQ(1 - e.time_PSD1) > 1
    || e.MaxQ_2inchPMT > 100;

  return vetoIf(flasher);
#undef SQ
}

using FlasherAlg = PureAlg<SingReader, flasherAlg>;

// -----------------------------------------------------------------------------

class MuonAlg : public SimpleAlg<SingReader> {
public:
  MuonAlg() : muons(N_MUONS) {}

  Status execute() override;

  RingBuf<Time> muons;

private:
  bool isMuon();
};

Status MuonAlg::execute()
{
  if (isMuon()) {
    const Time t(data->triggerTimeSec, data->triggerTimeNanoSec);
    const float sep = t.diff_us(muons.top());
    if (sep > MUON_SEP_US) {
      muons.put(t);
      return Status::SkipToNext;
    }
  }

  return Status::Continue;
}

bool MuonAlg::isMuon()
{
  if (data->detector == 7)
    return false;                    // ignore RPC

  if (data->detector == 5 || data->detector == 6)
    return data->nHit > 12;          // WP muon

  return data->NominalCharge > 3000; // AD muon
}

// -----------------------------------------------------------------------------

class SinglesVsMuonsAlg : public Algorithm {
public:
  SinglesVsMuonsAlg(const char* outFileName);

  void connect(Pipeline& pipeline) override;
  Status execute() override;
  void finalize(Pipeline& pipeline) override;

private:
  bool isPmp();
  bool isDly();
  TH1F newHist(const char* name, const char* title);
  void fill(TH1F& h, Time t);

  TH1F hDlyAll, hDlyLoose, hDlyTight;

  Time lastPmpTime[4];
  Time lastDlyTime[4];
  bool gapToDlyBeforeLastDly[4]{};
  bool gapToPmpBeforeLastDly[4]{};

  const SingReader::Data* data;
  const RingBuf<Time>* muons;

  std::string outFileName;
};

SinglesVsMuonsAlg::SinglesVsMuonsAlg(const char* outFileName) :
  hDlyAll(newHist("hDlyAll", "Delayed-like triggers")),
  hDlyLoose(newHist("hDlyLoose", "Delayed-like triggers [loose cut]")),
  hDlyTight(newHist("hDlyTight", "Delayed-like triggers [tight cut]")),
  outFileName(outFileName) {}

TH1F SinglesVsMuonsAlg::newHist(const char* name, const char* title)
{
  // XXX plot vs log of time instead of time itself?
  TH1F h(name, title, 200, 0, 2000);
  h.SetXTitle("Time to any muon [#mus]");
  return h;
}

void SinglesVsMuonsAlg::connect(Pipeline& pipeline)
{
  data = &pipeline.getAlg<SingReader>().data;
  muons = &pipeline.getAlg<MuonAlg>().muons;
}

void SinglesVsMuonsAlg::finalize(Pipeline& pipeline)
{
  pipeline.getOutFile(outFileName)->cd();

  hDlyAll.Write();
  hDlyLoose.Write();
  hDlyTight.Write();
}

bool SinglesVsMuonsAlg::isPmp()
{
  return data->energy > 0.7 && data->energy < 12;
}

bool SinglesVsMuonsAlg::isDly()
{
  return data->energy > 6 && data->energy < 12;
}

void SinglesVsMuonsAlg::fill(TH1F& h, Time t)
{
  for (const auto& muonTime : *muons) {
    const float dt_us = t.diff_us(muonTime);
    h.Fill(dt_us);
  }
}

Status SinglesVsMuonsAlg::execute()
{
  const size_t idet = data->detector - 1;
  const Time t(data->triggerTimeSec, data->triggerTimeNanoSec);

  if (isDly()) {
    fill(hDlyAll, t);

    auto isLongEnough =
      [&](Time prevTime) {
        return t.diff_us(prevTime) > ISOLATION_US;
      };

    const bool gapToDlyBeforeThis = isLongEnough(lastDlyTime[idet]);
    const bool gapToPmpBeforeThis = isLongEnough(lastPmpTime[idet]);

    if (gapToDlyBeforeThis && gapToDlyBeforeLastDly[idet])
      fill(hDlyLoose, lastDlyTime[idet]);

    if (gapToDlyBeforeThis && gapToPmpBeforeLastDly[idet])
      fill(hDlyTight, lastDlyTime[idet]);

    lastDlyTime[idet] = t;
    gapToDlyBeforeLastDly[idet] = gapToDlyBeforeThis;
    gapToPmpBeforeLastDly[idet] = gapToPmpBeforeThis;
  }

  if (isPmp()) {                // NOT else if!
    lastPmpTime[idet] = t;
  }

  return Status::Continue;
}

void run(const std::vector<std::string>& files, int maxEvents=0)
{
  Pipeline p;

  p.addOutFile("resultsFile", "results.root");

  p.makeAlg<SingReader>().setMaxEvents(maxEvents);
  p.makeAlg<CrossTriggerAlg>();
  p.makeAlg<MuonAlg>();
  p.makeAlg<FlasherAlg>();

  p.makeAlg<SinglesVsMuonsAlg>("resultsFile");

  p.process(files);
}

std::vector<std::string> deffiles()
{
  return {
          "/global/projecta/projectdirs/dayabay/data/dropbox/p17b/lz3.skim.6/recon.Neutrino.0021221.Physics.EH1-Merged.P17B-P._0001.root",
          "/global/projecta/projectdirs/dayabay/data/dropbox/p17b/lz3.skim.5/recon.Neutrino.0021221.Physics.EH1-Merged.P17B-P._0002.root"
  };
}
