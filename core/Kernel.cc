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

TFile* Pipeline::makeOutFile(const char* path, const char* name, bool reopen,
                             const char* mode)
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

std::string Pipeline::inFilePath(size_t i)
{
  return inFilePaths.at(i);
}

TFile* Pipeline::inFile(size_t i)
{
  const auto& path = inFilePaths.at(i);
  if (!inFileHandles.count(path))
    inFileHandles[path] = new TFile(path.c_str());
  return inFileHandles[path];
}

void Pipeline::notifyFileChanged(const Algorithm* reader, size_t i)
{
  for (const auto& alg : algVec)
    alg->fileChanged(reader, i);

  for (const auto& tool : toolVec)
    tool->fileChanged(reader, i);
}

void Pipeline::process(const std::vector<std::string>& inFiles)
{
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
