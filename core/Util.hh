#pragma once

#include <TChain.h>

#include <vector>
#include <string>

namespace util {

void initChain(TChain& chain, const std::vector<std::string>& inFiles);

template <typename T>
T clone(const T& parent, const char* name, const char* title = nullptr);

// If arg is '-', read list from stdin; otherwise return {arg}.
std::vector<std::string> parse_infile_arg(const char* arg);

} // namespace util

// -----------------------------------------------------------------------------

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

} // namespace relational

struct Time : relational::tag {
  UInt_t s, ns;

  Time() : s(0), ns(0) { }
  Time(UInt_t s_, UInt_t ns_) : s(s_), ns(ns_) { }

  float diff_us(const Time& other) const;

  Time shifted_us(float diff_us) const;

  bool operator<(const Time& other) const;

  bool operator==(const Time& other) const;
};

inline
float Time::diff_us(const Time& other) const
{
  // cast to enable signed arithmetic
  int s1 = s, ns1 = ns, s2 = other.s, ns2 = other.ns;
  return 1e6*(s1 - s2) + 1e-3*(ns1 - ns2);
}

inline
Time Time::shifted_us(float diff_us) const
{
  const int delta_s = 1e-6 * diff_us;
  const int delta_ns = 1e3 * (diff_us - 1e6*delta_s);

  UInt_t new_s = s + delta_s;
  int new_ns = ns + delta_ns;

  if (new_ns > int(1e9)) {
    new_ns -= int(1e9);
    new_s += 1;
  } else if (new_ns < 0) {
    new_ns += int(1e9);
    new_s -= 1;
  }

  return { new_s, UInt_t(new_ns) };
}

inline
bool Time::operator<(const Time& other) const
{
  return s < other.s || (s == other.s && ns < other.ns);
}

inline
bool Time::operator==(const Time& other) const
{
  return s == other.s && ns == other.ns;
}
