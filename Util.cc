#pragma once

#include <TChain.h>
#include <memory>

namespace util {
  void initChain(TChain& chain, const std::vector<std::string>& inFiles)
  {
    for (const auto& f : inFiles)
      chain.Add(f.c_str());

    chain.SetMakeClass(true);
    chain.SetBranchStatus("*", false);
  }

  template <class T, class... Args>
  std::unique_ptr<T> make_unique(Args&&... args)
  {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }

  template <typename T>
  T clone(const T& parent, const char* name, const char* title = nullptr)
  {
    T child = parent;
    child.SetName(name);
    if (title)
      child.SetTitle(title);
    return child;
  }
}

struct Time {
  UInt_t s, ns;

  Time() : s(0), ns(0) { }
  Time(UInt_t s_, UInt_t ns_) : s(s_), ns(ns_) { }

  float diff_us(const Time& other) const
  {
    return 1e6*(s - other.s) + 1e-3*(ns - other.ns);
  }
};
