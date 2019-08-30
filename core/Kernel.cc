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

class Algorithm {
public:
  enum class Status { Continue, SkipToNext, EndOfFile };

  virtual ~Algorithm() { };

  virtual void load(const std::vector<std::string>& inFiles) { };
  virtual void connect(Pipeline& pipeline) { };
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

  template <class Alg>
  Alg& addAlg(std::unique_ptr<Alg>&& alg);

  void addOutFile(const char* name, const char* path);
  TFile* getOutFile(const char* name);

  template <class Alg>
  Alg* getAlg(std::optional<std::function<bool(const Alg&)>> pred = std::nullopt);

  void process(const std::vector<std::string>& inFiles);

private:
  std::vector<std::unique_ptr<Algorithm>> algVec;
  std::map<std::string, TFile*> outFileMap;
};

template <class Alg, class... Args>
Alg& Pipeline::makeAlg(Args&&... args)
{
  auto p = std::unique_ptr<Alg>(new Alg(std::forward<Args>(args)...));
  return addAlg(std::move(p));
}

template <class Alg>
Alg& Pipeline::addAlg(std::unique_ptr<Alg>&& alg)
{
  Alg* p = alg.get();
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
Alg* Pipeline::getAlg(std::optional<std::function<bool(const Alg&)>> pred)
{
  for (const std::unique_ptr<Algorithm>& pAlg : algVec) {
    auto &alg = *pAlg;          // https://stackoverflow.com/q/46494928
    if (typeid(alg) == typeid(Alg)) {
      auto castedAlgPtr = dynamic_cast<Alg*>(pAlg.get());
      if (!pred.has_value() || pred.value()(*castedAlgPtr))
        return castedAlgPtr;
    }
  }
  throw std::runtime_error(Form("getAlg() couldn't find %s", typeid(Alg).name()));
}

void Pipeline::process(const std::vector<std::string>& inFiles)
{
  for (const auto& alg : algVec)
    alg->load(inFiles);

  for (const auto& alg : algVec)
    alg->connect(*this);

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

  for (const auto& alg : algVec)
    alg->finalize(*this);
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
