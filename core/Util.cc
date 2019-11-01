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

// https://stackoverflow.com/a/29269216/6055997
namespace relational {
  struct tag {};

  template <typename T>
  bool operator!= (T const& lhs, T const& rhs) { return !(lhs == rhs); }

  template <typename T>
  bool operator> (T const& lhs, T const& rhs) { return rhs < lhs; }
  template <typename T>
  bool operator<= (T const& lhs, T const& rhs) { return !(rhs < lhs); }
  template <typename T>
  bool operator>= (T const& lhs, T const& rhs) { return !(lhs < rhs); }
}

struct Time : relational::tag {
  UInt_t s, ns;

  Time() : s(0), ns(0) { }
  Time(UInt_t s_, UInt_t ns_) : s(s_), ns(ns_) { }

  float diff_us(const Time& other) const
  {
    // cast to enable signed arithmetic
    int s1 = s, ns1 = ns, s2 = other.s, ns2 = other.ns;
    return 1e6*(s1 - s2) + 1e-3*(ns1 - ns2);
  }

  Time shifted_us(float diff_us)
  {
    int diff_ns = int(1000 * diff_us);
    bool carry = ns + diff_ns > 1'000'000'000;
    unsigned int new_s = carry ? s + 1 : s;
    unsigned int new_ns = carry ? ns + diff_ns - 1'000'000'000 : ns + diff_ns;
    return { new_s, new_ns };
  }

  bool operator<(const Time& other) const
  {
    return s < other.s || (s == other.s && ns < other.ns);
  }

  bool operator==(const Time& other) const
  {
    return s == other.s && ns == other.ns;
  }
};
