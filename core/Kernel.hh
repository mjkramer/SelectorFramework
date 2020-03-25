#pragma once

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <set>
#include <string>
#include <vector>

#include <TFile.h>

class Pipeline;


class Node {
public:
  Node() = default;
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;

  virtual ~Node() { };
  virtual void connect(Pipeline& pipeline) { };

  void do_connect(Pipeline& pipeline);

  virtual int rawTag() const; // for identification

protected:
  Pipeline& pipe() const { return *pipe_; }

private:
  Pipeline* pipe_;
};

// -----------------------------------------------------------------------------

class Tool : public Node {
};

// -----------------------------------------------------------------------------

class Algorithm : public Node {
public:
  enum class Status { Continue, SkipToNext, EndOfFile };

  virtual void load(const std::vector<std::string>& inFiles) { };
  virtual Status execute() = 0;
  virtual void postExecute() { };
  virtual void finalize(Pipeline& pipeline) { };
  virtual bool isReader() const { return false; } // "reader" algs need special treatment
};

// -----------------------------------------------------------------------------

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
  using Pred = std::function<bool(const Thing&)>;

  template <class Alg>
  Alg* getAlg(Pred<Alg> pred = nullptr);

  template <class Alg, class T> // T should be integral
  Alg* getAlg(T tag);

  template <class Tool>
  Tool* getTool(Pred<Tool> pred = nullptr);

  template <class Tool, class T>
  Tool* getTool(T tag);

  TFile* makeOutFile(const char* path, const char* name = DefaultFile, bool reopen=false,
                     const char* mode = "RECREATE");
  TFile* getOutFile(const char* name = DefaultFile);
  static constexpr const char* const DefaultFile = "";

  size_t inFileCount();
  TFile* inFile(size_t i = 0);

  void process(const std::vector<std::string>& inFiles);

  ~Pipeline() {};

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
    auto castedPtr = dynamic_cast<Thing*>(pThing.get());
    if (castedPtr) {
      if (!pred || pred(*castedPtr))
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

// -----------------------------------------------------------------------------

inline
Algorithm::Status vetoIf(bool cond)
{
  return cond ? Algorithm::Status::SkipToNext : Algorithm::Status::Continue;
}
