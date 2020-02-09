// Include me from the main.cc for your analysis.

// Get rid of me once we have a Makefile that directly calls g++ and rootcling
// because then we can just glob all the source files instead of listing them
// individually.

#include "core/BaseIO.cc"
#include "core/Clock.cc"
#include "core/ConfigTool.cc"
#include "core/EventBuf.cc"
#include "core/Kernel.cc"
#include "core/SimpleAlg.cc"
#include "core/SyncReader.cc"
#include "core/TimeSyncReader.cc"
#include "core/TreeWriter.cc"
#include "core/Util.cc"
