#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
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
};

Algorithm::Status vetoIf(bool cond)
{
  return cond ? Algorithm::Status::SkipToNext : Algorithm::Status::Continue;
}

class Pipeline {
public:
  template <class Alg, class... Args>
  Alg& makeAlg(Args&&... args);

  template <class Tool, class... Args>
  Tool& makeTool(Args&&... args);


  template <class Thing>
  using Pred = std::optional<std::function<bool(const Thing&)>>;

  template <class Alg>
  Alg* getAlg(Pred<Alg> pred = std::nullopt);

  template <class Tool>
  Tool* getTool(Pred<Tool> pred = std::nullopt);


  void makeOutFile(const char* path, const char* name = DefaultFile);
  TFile* getOutFile(const char* name = DefaultFile);
  static constexpr const char* const DefaultFile = "";

  void process(const std::vector<std::string>& inFiles);

  ~Pipeline();


private:
  template <class Thing>
  using PtrVec = std::vector<std::unique_ptr<Thing>>;

  template <class Thing, class BaseThing, class... Args>
  Thing& makeThing(PtrVec<BaseThing>& vec, Args&&... args);

  template <class Thing, class BaseThing>
  Thing* getThing(PtrVec<BaseThing>& vec, Pred<Thing> pred);


  PtrVec<Algorithm> algVec;
  PtrVec<Tool> toolVec;
  std::map<std::string, TFile*> outFileMap;
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
  return makeThing<Alg>(algVec, std::forward<Args>(args)...);
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

template <class Alg>
Alg* Pipeline::getAlg(Pred<Alg> pred)
{
  return getThing(algVec, pred);
}

template <class Tool>
Tool* Pipeline::getTool(Pred<Tool> pred)
{
  return getThing(toolVec, pred);
}

void Pipeline::makeOutFile(const char* path, const char* name)
{
  outFileMap[name] = new TFile(path, "RECREATE");
}

TFile* Pipeline::getOutFile(const char* name)
{
  return outFileMap[name];
}

void Pipeline::process(const std::vector<std::string>& inFiles)
{
  for (const auto& alg : algVec)
    alg->load(inFiles);

  for (const auto& alg : algVec)
    alg->connect(*this);

  for (const auto& tool : toolVec)
    tool->connect(*this);

  while (true) {
    bool atEnd = false;

    for (const auto& alg : algVec) {
      const auto status = alg->execute();
      if (status == Algorithm::Status::SkipToNext)
        break;
      if (status == Algorithm::Status::EndOfFile)
        // Don't exit right away; run remaining algs first
        atEnd = true;
    }

    if (atEnd)
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
  const decltype(ReaderT::data)* data;
};

template <class ReaderT>
void SimpleAlg<ReaderT>::connect(Pipeline& pipeline)
{
  data = &pipeline.getAlg<ReaderT>()->data;
}

template <class ReaderT>
Algorithm::Status SimpleAlg<ReaderT>::execute()
{
  return consume(*data);
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
