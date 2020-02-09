#include "Util.hh"

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

} // namespace util

