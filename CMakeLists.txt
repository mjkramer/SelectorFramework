cmake_minimum_required(VERSION 3.12)
project(SelectorFramework LANGUAGES CXX)

set(SELECTOR_FRAMEWORK_HEADERS
    Assert.hh
    BaseIO.hh
    Clock.hh
    ConfigTool.hh
    DirectReader.hh
    EventBuf.hh
    Kernel.hh
    PrefetchLooper.hh
    RingBuf.hh
    SimpleAlg.hh
    SyncReader.hh
    TimeSyncReader.hh
    TreeWriter.hh
    Util.hh
    )

add_library(SelectorFramework SHARED
    core/BaseIO.cc
    core/Clock.cc
    core/ConfigTool.cc
    core/Kernel.cc
    core/Util.cc
    )
target_include_directories(SelectorFramework PUBLIC core)

ROOT_GENERATE_DICTIONARY(Dict_SelectorFramework "${SELECTOR_FRAMEWORK_HEADERS}" 
                         MODULE SelectorFramework
                         )
target_link_libraries(SelectorFramework PUBLIC ROOT::Core ROOT::Hist ROOT::Tree)
