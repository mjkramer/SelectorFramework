#include <map>
#include <memory>
#include <string>
#include <vector>

#include <TChain.h>
#include <TFile.h>

class Pipeline;

class Algorithm {
public:
  enum class Status { Continue, SkipToNext, EndOfFile };

  virtual ~Algorithm() { };

  virtual void load(const std::vector<std::string>& inFiles) { };
  virtual void connect(Pipeline& pipeline) { };
  virtual Status execute() = 0;
  virtual void finalize() { };
};

class Pipeline {
public:
  template <class Alg, class... Args>
  Alg& makeAlg(const char* name, Args&&... args);

  template <class Alg>
  Alg& addAlg(const char* name, std::unique_ptr<Alg>&& alg);

  void addOutFile(const char* name, const char* path);
  TFile* getOutFile(const char* name);

  template <class Alg>
  Alg& getAlg(const char* name);

  void process(const std::vector<std::string>& inFiles);

private:
  std::vector<std::unique_ptr<Algorithm>> algVec;
  std::map<std::string, Algorithm*> algMap;
  std::map<std::string, TFile*> outFileMap;
};

template <class Alg, class... Args>
Alg& Pipeline::makeAlg(const char* name, Args&&... args)
{
  auto p = std::unique_ptr<Alg>(new Alg(std::forward<Args>(args)...));
  return addAlg(name, std::move(p));
}

template <class Alg>
Alg& Pipeline::addAlg(const char* name, std::unique_ptr<Alg>&& alg)
{
  Alg* p = alg.get();
  algMap[name] = p;
  algVec.emplace_back(std::move(alg));
  return *p;
}

void Pipeline::addOutFile(const char* name, const char* path)
{
  outFileMap[name] = new TFile(path, "RECREATE");
}

TFile* Pipeline::getOutFile(const char* name)
{
  return outFileMap[name];
}

template <class Alg>
Alg& Pipeline::getAlg(const char* name)
{
  return *dynamic_cast<Alg*>(algMap[name]);
}

void Pipeline::process(const std::vector<std::string>& inFiles)
{
  for (const auto& alg : algVec)
    alg->load(inFiles);

  for (const auto& alg : algVec)
    alg->connect(*this);

  while (true) {
    for (const auto& alg : algVec) {
      const auto status = alg->execute();
      if (status == Algorithm::Status::SkipToNext)
        break;
      if (status == Algorithm::Status::EndOfFile)
        goto end;
    }
  }

 end:
  for (const auto& alg : algVec)
    alg->finalize();
}

namespace util {
  void initChain(TChain& chain, const std::vector<std::string>& inFiles)
  {
    for (const auto& f : inFiles)
      chain.Add(f.c_str());

    chain.SetMakeClass(true);
    chain.SetBranchStatus("*", false);
  }
}

#define BR(branch)                                    \
  chains[0]->SetBranchStatus(#branch, true);          \
  chains[0]->SetBranchAddress(#branch, &data.branch)
