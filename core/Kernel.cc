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

  void do_connect(Pipeline& pipeline)
  {
    pipe_ = &pipeline;
    connect(pipeline);
  }

  virtual int rawTag() const; // for identification

protected:
  Pipeline& pipe() const { return *pipe_; }

private:
  Pipeline* pipe_;
};

int Node::rawTag() const
{
  throw std::runtime_error("rawTag not implemented for this node");
}

class Tool : public Node {
};

class Algorithm : public Node {
public:
  enum class Status { Continue, SkipToNext, EndOfFile };

  virtual void load(const std::vector<std::string>& inFiles) { };
  virtual Status execute() = 0;
  virtual void finalize(Pipeline& pipeline) { };
  virtual bool isReader() const { return false; } // "reader" algs need special treatment
};

Algorithm::Status vetoIf(bool cond)
{
  return cond ? Algorithm::Status::SkipToNext : Algorithm::Status::Continue;
}

class Pipeline {
public:
  Pipeline() = default;
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

  template <class Alg, class T> // T should be integral
  Alg* getAlg(T tag);

  template <class Tool>
  Tool* getTool(Pred<Tool> pred = std::nullopt);

  template <class Tool, class T>
  Tool* getTool(T tag);


  TFile* makeOutFile(const char* path, const char* name = DefaultFile, bool reopen=false);
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

  // Make sure outFileMap is declared BEFORE algVec/toolVec etc.
  // to ensure that files are still open during alg/tool/etc destructors
  std::map<std::string, std::unique_ptr<TFile>> outFileMap;

  PtrVec<Algorithm> algVec;
  std::set<const Algorithm*> runningReaders;
  PtrVec<Tool> toolVec;

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
    return thing.rawTag() == tag;
  };
  return getThing<Thing, BaseThing>(vec, pred);
}

template <class Alg>
Alg* Pipeline::getAlg(Pred<Alg> pred)
{
  return getThing(algVec, pred);
}

template <class Alg, class T>
Alg* Pipeline::getAlg(T tag)
{
  return getThing<Alg>(algVec, int(tag));
}

template <class Tool>
Tool* Pipeline::getTool(Pred<Tool> pred)
{
  return getThing(toolVec, pred);
}

template <class Tool, class T>
Tool* Pipeline::getTool(T tag)
{
  return getThing<Tool>(toolVec, int(tag));
}

TFile* Pipeline::makeOutFile(const char* path, const char* name, bool reopen)
{
  const auto it = outFileMap.find(name);
  if (it != outFileMap.end()) {
    if (reopen)
      outFileMap.erase(it);
    else
      throw std::runtime_error(Form("file %s already opened", name));
  }

  const auto& ptr = outFileMap[name] = std::make_unique<TFile>(path, "RECREATE");
  return ptr.get();
}

TFile* Pipeline::getOutFile(const char* name)
{
  return outFileMap[name].get();
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
  if (inFiles.size() > 1)
    throw std::runtime_error("Need to implement notification of algs when file changes");

  inFilePaths = inFiles;

  for (const auto& alg : algVec)
    alg->load(inFiles);

  for (const auto& alg : algVec)
    alg->do_connect(*this);

  for (const auto& tool : toolVec)
    tool->do_connect(*this);

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
}

// -----------------------------------------------------------------------------

// SimpleAlg/PureAlg<ReaderT> assume ReaderT { _& getData(); bool ready(); }
template <class ReaderT>
using algdata_t = std::decay_t<decltype(((ReaderT*)0)->getData())>;

template <class ReaderT>
using algfunc_t = Algorithm::Status(const algdata_t<ReaderT> &);

template <class ReaderT, class TagT = int>
class SimpleAlg : public Algorithm {
public:
  using Pred = ::Pipeline::Pred<ReaderT>;

  SimpleAlg() {};
  SimpleAlg(Pipeline::Pred<ReaderT> pred);
  SimpleAlg(TagT tag);

  void connect(Pipeline& pipeline) override;
  Algorithm::Status execute() override;

  virtual Algorithm::Status consume(const algdata_t<ReaderT>& data) = 0;

  virtual int rawTag() const override { return int(*tag_); }
  TagT tag() { return *tag_; }

protected:
  const ReaderT* reader = nullptr;

private:
  Pred pred_;
  std::optional<TagT> tag_;
};

template <class ReaderT, class TagT>
SimpleAlg<ReaderT, TagT>::SimpleAlg(Pred pred)
  : pred_(std::move(pred)) {}

template <class ReaderT, class TagT>
SimpleAlg<ReaderT, TagT>::SimpleAlg(TagT tag)
  : tag_(tag) {}

template <class ReaderT, class TagT>
void SimpleAlg<ReaderT, TagT>::connect(Pipeline &pipeline) {
  if (pred_) {
    reader = pipeline.getAlg<ReaderT>(pred_);
  } else if (tag_) {
    reader = pipeline.getAlg<ReaderT>(*tag_);
  } else {
    reader = pipeline.getAlg<ReaderT>();
  }
}

template <class ReaderT, class TagT>
Algorithm::Status SimpleAlg<ReaderT, TagT>::execute()
{
  if (reader->ready())
    return consume(reader->getData());

  else return Status::Continue;
}

// -----------------------------------------------------------------------------

// template <class ReaderT>
// using algfunc_t = std::function<Algorithm::Status (typename ReaderT::Data*)>;

template <class ReaderT, algfunc_t<ReaderT> func, class TagT = int>
class PureAlg : public SimpleAlg<ReaderT, TagT> {
public:
  Algorithm::Status consume(const algdata_t<ReaderT>& data) override
  {
    return func(data);
  };
};
