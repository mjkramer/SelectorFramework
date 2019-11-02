#pragma once

#include "TTree.h"
#include "TFile.h"
#include "TDataType.h"          // hack

#include "Kernel.cc"

#include <type_traits>

// ROOT 6.19 should support automagic std::array branches; for 6.18, we use hack:
static char DataTypeToChar(EDataType datatype);

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

template <typename T>
static std::enable_if_t<std::is_enum_v<T>, void>
do_branch(TTree* tree, const char* name, T* ptr)
{
  tree->Branch(name, reinterpret_cast<std::underlying_type_t<T>*>(ptr));
}

template <typename T>
static std::enable_if_t<!std::is_enum_v<T>, void>
do_branch(TTree* tree, const char* name, T* ptr)
{
  tree->Branch(name, ptr);
}

template <typename T>
void BranchManager::branch(const char* name, T* ptr)
{
  if (mode == IOMode::IN) {
    tree->SetBranchStatus(name, true);
    tree->SetBranchAddress(name, ptr); // will this work for std::array in 6.19?
  }

  else {
    // this should work in 6.19 even if T is a std::array
    do_branch(tree, name, ptr);
  }
}

// this should be unnecessary in 6.19
// TODO: Support arrays of scoped enums (using underlying_type)?
template <typename T, std::size_t N>
void BranchManager::branch(const char* name, std::array<T, N>* arrptr,
                           const char* len_branch)
{
  if (mode == IOMode::IN) {
    tree->SetBranchStatus(name, true);
    tree->SetBranchAddress(name, arrptr->data());
  }

  else {
    const EDataType datatype = TDataType::GetType(typeid(T));
    const char typecode = DataTypeToChar(datatype);

    // this logic here (for BR_VARLEN) is one reason we don't want to scrap all
    // of this when 6.19 is out
    const char* len = len_branch ? len_branch : Form("%lu", N);
    const char* leaflist = Form("%s[%s]/%c", name, len, typecode);
    tree->Branch(name, arrptr->data(), leaflist);
  }
}

class TreeBase {
public:
  virtual void initBranches() = 0;
  void setManager(BranchManager* mgr) { _mgr = mgr; }

protected:
  BranchManager* _mgr = nullptr;
};

#define BR(name) _mgr->branch(#name, &name)
#define BR_NAMED(varname, brname) _mgr->branch(brname, &varname)
#define BR_VARLEN(varname_arr, varname_len) \
  _mgr->branch(#varname_arr, &varname_arr, #varname_len)

// ----------------------------------------------------------------------

// HACK HACK HACK
// copied from TTree.cxx
// get rid of me when switch to ROOT 6.19?
static char DataTypeToChar(EDataType datatype)
{
  // Return the leaflist 'char' for a given datatype.
 
  switch(datatype) {
  case kChar_t:     return 'B';
  case kUChar_t:    return 'b';
  case kBool_t:     return 'O';
  case kShort_t:    return 'S';
  case kUShort_t:   return 's';
  case kCounter:
  case kInt_t:      return 'I';
  case kUInt_t:     return 'i';
  case kDouble_t:   return 'D';
  case kDouble32_t: return 'd';
  case kFloat_t:    return 'F';
  case kFloat16_t:  return 'f';
  case kLong_t:     return 0; // unsupported
  case kULong_t:    return 0; // unsupported?
  case kchar:       return 0; // unsupported
  case kLong64_t:   return 'L';
  case kULong64_t:  return 'l';
 
  case kCharStar:   return 'C';
  case kBits:       return 0; //unsupported
 
  case kOther_t:
  case kNoType_t:
  default:
    // return 0;
    throw std::runtime_error("DataTypeToChar: Unrecognized type");
  }
}
