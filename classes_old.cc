// Can't use StateT as a template param because then Pipeline can't store a
// vector
template <class StreamT, class StateT = void>
class Algorithm0 {
  template <class T>
  using when_stateful = std::enable_if_t<!std::is_void<StateT>::value, T>;

public:
  virtual bool process(StreamT const& strm) = 0;
  virtual void finalize() = 0;
  when_stateful<StateT const&> getState() const;

protected:
  when_stateful<StateT> state;
};

