#include "Util.hh"

#include <cstring>
#include <iostream>

namespace util {

void initChain(TChain& chain, const std::vector<std::string>& inFiles)
{
  for (const auto& f : inFiles)
    chain.Add(f.c_str());

  chain.SetMakeClass(true);
  chain.SetBranchStatus("*", false);
}

template <typename T>
T clone(const T& parent, const char* name, const char* title)
{
  T child = parent;
  child.SetName(name);
  if (title)
    child.SetTitle(title);
  return child;
}

std::vector<std::string> parse_infile_arg(const char* arg)
{
  if (strcmp(arg, "-") != 0)
    return {arg};

  std::vector<std::string> result;
  std::string line;
  while (std::getline(std::cin, line))
    result.push_back(line);
  return result;
}

} // namespace util

