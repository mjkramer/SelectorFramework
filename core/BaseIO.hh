#pragma once

#include <TTree.h>

struct BranchManager;

class TreeBase {
public:
  virtual void initBranches() = 0;
  void setManager(BranchManager* mgr) { _mgr = mgr; }

protected:
  BranchManager* _mgr = nullptr;
};

#define BR(name) _mgr->branch(#name, &name)
#define BR_NAMED(varname, brname) _mgr->branch(brname, &varname)
#define BR_VARLEN(varname_arr, varname_len)               \
  _mgr->branch(#varname_arr, &varname_arr, #varname_len)

// deputy assistant to the regional supervisor
struct BranchManager {
  enum class IOMode { IN, OUT };

  BranchManager(IOMode mode) : mode(mode) {}

  template <typename T>
  void branch(const char* name, T* ptr);

  template <typename T, std::size_t N>
  void branch(const char* name, std::array<T, N>* arrptr,
              const char* len_branch = nullptr);

  const IOMode mode;
  TTree* tree = nullptr;
};
