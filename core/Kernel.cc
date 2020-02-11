#include "Kernel.hh"

#include "Util.hh"

#include <typeinfo>

void Node::do_connect(Pipeline& pipeline)
{
  pipe_ = &pipeline;
  connect(pipeline);
}

int Node::rawTag() const
{
  throw std::runtime_error("rawTag not implemented for this node");
}

// -----------------------------------------------------------------------------

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

  Algorithm* lastAlg = nullptr;

  auto isDoneReader = [&](const std::unique_ptr<Algorithm>& alg) {
    return alg->isReader() && runningReaders.count(alg.get()) == 0;
  };

  while (true) {
    for (const auto& alg : algVec) {
      if (isDoneReader(alg))
        continue;

      lastAlg = alg.get();

      const auto status = alg->execute();
      if (status == Algorithm::Status::SkipToNext)
        break;
      if (status == Algorithm::Status::EndOfFile)
        runningReaders.erase(alg.get());
    }

    for (const auto& alg : algVec) {
      if (isDoneReader(alg))
        continue;

      alg->postExecute();

      if (alg.get() == lastAlg)
        break;
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
