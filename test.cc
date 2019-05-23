template <class DataT>
class TestStreamBase : public Stream<DataT> {
private:
  int loc = 0;
};

template <class DataT>
bool TestStreamBase<DataT>::readNext()
{
  if (loc < 10) {
    data.read(loc++)
    return true;
  } else
    return false;
}

struct TestDataBase {
  void read(int loc);
};

struct TestData : TestDataBase {
  int i;
};

void TestData::read(int loc)
{
  i = loc;
}

using TestStream = TestStreamBase<TestData>;

// ----------------------------------------------------------------------

class CrossTrigAlg : public Algorithm<TestStream> {
public:
  bool execute(const TestData& data) override;
};

bool CrossTrigAlg::execute(const TestData& data)
{
  return !(data.i == 5);
}

// ----------------------------------------------------------------------

bool flasher(const TestData& data)
{
  return !(data.i % 4 == 0);
}

using FlasherAlg = PureAlg<TestStream, flasher>;

// ----------------------------------------------------------------------

struct MuonState {
  int nMuons = 0;
};

class MuonAlg : public StatefulAlg<TestStream, MuonState> {
public:
  bool execute(const TestData& data) override;
};

bool MuonAlg::execute(const TestData& data)
{
  if (data.i % 2 == 0)
    ++state.nMuons;

  return true;
}

// ----------------------------------------------------------------------

class SinglesAlg : public Algorithm<TestStream> {
public:
  void initialize(const Pipeline& pipeline) override;
  bool execute(const TestData& data) override;

private:
  const MuonState* muonState;
};

void SinglesAlg::initialize(const Pipeline& pipeline)
{
  muonState = pipeline.getState<MuonState>("MuonTight");
}

bool SinglesAlg::execute(const TestData& data)
{
  if (muonState->nMuons % 2 == 1)
    std::cout << "Odd muons!" << std::endl;

  return true;
}

// ----------------------------------------------------------------------

int main()
{
  Pipeline<TestStream> pipeline;
  pipeline.addOutFile("main", "main.root");

  CrossTrigAlg cta(pipeline, "CrossTrig");
  MuonAlg ma(pipeline, "MuonTight");
  FlasherAlg fa(pipeline, "FlasherVeto");
  SinglesAlg sa(pipeline, "Singles");

  TestStream stream;
  pipeline.process(stream);

  return 0;
}
