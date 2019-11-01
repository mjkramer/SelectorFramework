#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include <TChain.h>
#include <TFile.h>

#include "Util.cc"

class Pipeline;

class Node {
public:
  Node() = default;
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;

  virtual ~Node() { };
  virtual void connect(Pipeline& pipeline) { };
};

class Tool : public Node {
};

class Algorithm : public Node {
public:
  enum class Status { Continue, SkipToNext, EndOfFile };

  virtual void load(const std::vector<std::string>& inFiles) { };
  virtual Status execute() = 0;
  virtual void finalize(Pipeline& pipeline) { };
  virtual bool isReader() { return false; } // "reader" algs need special treatment
  virtual int getTag() { return 0; } // for identification
};

Algorithm::Status vetoIf(bool cond)
{
  return cond ? Algorithm::Status::SkipToNext : Algorithm::Status::Continue;
}

class Pipeline {
public:
  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;

  template <class Alg, class... Args>
  Alg& makeAlg(Args&&... args);

  template <class Tool, class... Args>
  Tool& makeTool(Args&&... args);


  template <class Thing>
  using Pred = std::optional<std::function<bool(const Thing&)>>;

  template <class Alg>
  Alg* getAlg(Pred<Alg> pred = std::nullopt);

  template <class Alg>
  Alg* getAlg(int tag);

  template <class Tool>
  Tool* getTool(Pred<Tool> pred = std::nullopt);

  template <class Tool>
  Tool* getTool(int tag);


  void makeOutFile(const char* path, const char* name = DefaultFile);
  TFile* getOutFile(const char* name = DefaultFile);
  static constexpr const char* const DefaultFile = "";

  size_t inFileCount();
  TFile* inFile(size_t i = 0);

  void process(const std::vector<std::string>& inFiles);

  ~Pipeline();


private:
  template <class Thing>
  using PtrVec = std::vector<std::unique_ptr<Thing>>;

  template <class Thing, class BaseThing, class... Args>
  Thing& makeThing(PtrVec<BaseThing>& vec, Args&&... args);

  template <class Thing, class BaseThing>
  Thing* getThing(PtrVec<BaseThing>& vec, Pred<Thing> pred);

  template <class Thing, class BaseThing>
  Thing* getThing(PtrVec<BaseThing>& vec, int tag);

  PtrVec<Algorithm> algVec;
  std::set<const Algorithm*> runningReaders;
  PtrVec<Tool> toolVec;
  std::map<std::string, TFile*> outFileMap;

  std::vector<std::string> inFilePaths;
  std::map<std::string, TFile*> inFileHandles;
};

template <class Thing, class BaseThing, class... Args>
Thing& Pipeline::makeThing(PtrVec<BaseThing>& vec, Args&&... args)
{
  auto p = std::make_unique<Thing>(std::forward<Args>(args)...);
  Thing& thingRef = *p;
  vec.push_back(std::move(p));
  return thingRef;
}

template <class Alg, class... Args>
Alg& Pipeline::makeAlg(Args&&... args)
{
  auto& alg = makeThing<Alg>(algVec, std::forward<Args>(args)...);

  if (alg.isReader())
    runningReaders.insert(&alg);

  return alg;
}

template <class Tool, class... Args>
Tool& Pipeline::makeTool(Args&&... args)
{
  return makeThing<Tool>(toolVec, std::forward<Args>(args)...);
}

template <class Thing, class BaseThing>
Thing* Pipeline::getThing(PtrVec<BaseThing>& vec, Pred<Thing> pred)
{
  for (const auto& pThing : vec) {
    auto &thing = *pThing;          // https://stackoverflow.com/q/46494928
    if (typeid(thing) == typeid(Thing)) {
      auto castedPtr = dynamic_cast<Thing*>(pThing.get());
      if (!pred.has_value() || pred.value()(*castedPtr))
        return castedPtr;
    }
  }
  throw std::runtime_error(Form("getThing() couldn't find %s", typeid(Thing).name()));
}

template <class Thing, class BaseThing>
Thing* Pipeline::getThing(PtrVec<BaseThing>& vec, int tag)
{
  auto pred = [&](const Thing& thing) {
    return thing.getTag() == tag;
  };
  return getThing<Thing, BaseThing>(vec, pred);
}

template <class Alg>
Alg* Pipeline::getAlg(Pred<Alg> pred)
{
  return getThing(algVec, pred);
}

template <class Alg>
Alg* Pipeline::getAlg(int tag)
{
  return getThing<Alg>(algVec, tag);
}

template <class Tool>
Tool* Pipeline::getTool(Pred<Tool> pred)
{
  return getThing(toolVec, pred);
}

template <class Tool>
Tool* Pipeline::getTool(int tag)
{
  return getThing<Tool>(toolVec, tag);
}

void Pipeline::makeOutFile(const char* path, const char* name)
{
  outFileMap[name] = new TFile(path, "RECREATE");
}

TFile* Pipeline::getOutFile(const char* name)
{
  return outFileMap[name];
}

size_t Pipeline::inFileCount()
{
  return inFilePaths.size();
}

TFile* Pipeline::inFile(size_t i)
{
  const auto& path = inFilePaths.at(i);
  if (!inFileHandles.count(path))
    inFileHandles[path] = new TFile(path.c_str());
  return inFileHandles[path];
}

void Pipeline::process(const std::vector<std::string>& inFiles)
{
  inFilePaths = inFiles;

  for (const auto& alg : algVec)
    alg->load(inFiles);

  for (const auto& alg : algVec)
    alg->connect(*this);

  for (const auto& tool : toolVec)
    tool->connect(*this);

  while (true) {
    for (const auto& alg : algVec) {
      if (alg->isReader() && runningReaders.count(alg.get()) == 0)
        continue;

      const auto status = alg->execute();
      if (status == Algorithm::Status::SkipToNext)
        break;
      if (status == Algorithm::Status::EndOfFile)
        runningReaders.erase(alg.get());
    }

    if (runningReaders.size() == 0)
      break;
  }

  // For convenience, cd to the default output file
  if (outFileMap.find(DefaultFile) != outFileMap.end())
    getOutFile()->cd();

  for (const auto& alg : algVec)
    alg->finalize(*this);
}

Pipeline::~Pipeline()
{
  for (const auto& kv : outFileMap)
    kv.second->Close();
}

// -----------------------------------------------------------------------------

// SimpleAlg/PureAlg<ReaderT> assume ReaderT { struct {...} data; }

template <class ReaderT>
class SimpleAlg : public Algorithm {
public:
  void connect(Pipeline& pipeline) override;
  Algorithm::Status execute() override;

  virtual Algorithm::Status consume(const decltype(ReaderT::data)& data) = 0;

protected:
  const ReaderT* reader;
};

template <class ReaderT>
void SimpleAlg<ReaderT>::connect(Pipeline& pipeline)
{
  reader = pipeline.getAlg<ReaderT>();
}

template <class ReaderT>
Algorithm::Status SimpleAlg<ReaderT>::execute()
{
  if (reader->ready())
    return consume(reader->data);

  else return Status::Continue;
}

// -----------------------------------------------------------------------------

// template <class ReaderT>
// using algfunc_t = std::function<Algorithm::Status (typename ReaderT::Data*)>;

template <class ReaderT>
using algfunc_t = Algorithm::Status (const decltype(ReaderT::data)&);

template <class ReaderT, algfunc_t<ReaderT> func>
class PureAlg : public SimpleAlg<ReaderT> {
public:
  Algorithm::Status consume(const decltype(ReaderT::data)& data) override
  {
    return func(data);
  };
};
