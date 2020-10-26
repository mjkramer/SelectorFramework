cmake_minimum_required(VERSION 3.15)
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
                         MODULE SelectorFramework OPTIONS -interpreteronly
                         )

target_include_directories(SelectorFramework SYSTEM PUBLIC "${ROOT_INCLUDE_DIRS}" )
target_link_libraries(SelectorFramework PUBLIC ROOT::Core ROOT::Hist ROOT::Tree)
target_compile_options(SelectorFramework PUBLIC "-Wno-deprecated-volatile")

set_property(TARGET SelectorFramework PROPERTY SELECTOR_HEADERS "${SELECTOR_FRAMEWORK_HEADERS}")
