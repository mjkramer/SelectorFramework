template <class DataT>
struct Stream {
  bool readNext();
  DataT data;
};

template <class StreamT>
class Algorithm {
public:
  Algorithm(Pipeline<StreamT>& pipeline, const char* name);
  virtual void initialize(const Pipeline& pipeline) { };
  // virtual bool execute(const StreamT& stream) = 0;
  virtual bool execute(const decltype(StreamT::data)& data) = 0;
  virtual void finalize() { };
};

template <class StreamT>
Algorithm::Algorithm(Pipeline<StreamT>& pipeline, const char* name)
{
  pipeline.addAlg(this, name);
}

template <class StreamT, bool (*F)(const decltype(StreamT::data)&)>
class PureAlg : public Algorithm<StreamT> {
public:
  bool execute(const decltype(StreamT::data)& data) override
  {
    return F(data);
  }
};

template <class StreamT, class StateT>
class StatefulAlg : public Algorithm<StreamT> {
protected:
  StateT state;
};

template <class StreamT>
class Pipeline {
public:
  void addAlg(Algorithm<StreamT>* alg, const char* name);
  void addOutFile(const char* name, const char* path);
  void getOutFile();

  template <class StateT>
  const StateT* getState(const char* algName) const;

  void process(StreamT& stream);

private:
  std::vector<Algorithm<StreamT>*> algVec_;
  std::map<std::string, Algorithm<StreamT>*> algMap_;
  std::map<std::string, void*> outFileMap_;
};

template <class StreamT>
void Pipeline<StreamT>::addAlg(Algorithm<StreamT>* alg, const char* name)
{
  algVec_.push_back(alg);
  algMap_[name] = alg;
}

template <class StreamT>
void Pipeline<StreamT>::addOutFile(const char* name, const char* path)
{
  outFileMap_[name] = nullptr;
}

template <class StreamT>
void* Pipeline<StreamT>::getOutFile(const char* name)
{
  return outFileMap_[name];
}

template <class StreamT, class StateT>
const StateT* Pipeline<StreamT>::getState<StateT>(const char* algName)
{
  const Algorithm<StreamT>* alg = algMap_[algName];
  return &dynamic_cast<StateFulAlg<StreamT, StateT>*>(alg)->state;
}

template <class StreamT>
void Pipline<StreamT>::process(StreamT& stream)
{
  for (const auto& alg : algVec_)
    alg.initialize();

  while (stream.readNext()) {
    for (const auto& alg : algVec_) {
      if (!alg.execute(stream.data))
        break;
    }
  }

  for (const auto& alg : algVec_)
    alg.finalize();
}
