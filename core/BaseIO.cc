#include "BaseIO.hh"

#include <TFile.h>
#include <TDataType.h>          // hack

#include <type_traits>

// ----------------------------------------------------------------------

// HACK HACK HACK
// copied from TTree.cxx
// get rid of me when switch to ROOT 6.19?
char DataTypeToChar(EDataType datatype)
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
