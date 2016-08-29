set(Common_SRC
    analyzers/baseanalyzer.cpp
    analyzers/clusterinfo.cpp
    analyzers/configanalyzers.cpp
    analyzers/correlation.cpp
    analyzers/depiction.cpp
    analyzers/dualanalyzer.cpp
    analyzers/dutcorrelation.cpp
    analyzers/dutdepiction.cpp
    analyzers/eventinfo.cpp
    analyzers/exampledualanalyzer.cpp
    analyzers/examplesingleanalyzer.cpp
    analyzers/hitinfo.cpp
    analyzers/matching.cpp
    analyzers/occupancy.cpp
    analyzers/singleanalyzer.cpp
    analyzers/syncfluctuation.cpp
    analyzers/trackinfo.cpp
    analyzers/dutresiduals.cpp
    analyzers/efficiency.cpp
    analyzers/residuals.cpp
    converters/kartelconvert.cpp
    exception.cpp
    loopers/analysis.cpp
    loopers/applymask.cpp
    loopers/coarsealign.cpp
    loopers/coarsealigndut.cpp
    loopers/configloopers.cpp
    loopers/examplelooper.cpp
    loopers/looper.cpp
    loopers/synchronize.cpp
    loopers/analysisdut.cpp
    loopers/finealign.cpp
    loopers/finealigndut.cpp
    loopers/noisescan.cpp
    loopers/processevents.cpp
    mechanics/configmechanics.cpp
    mechanics/device.cpp
    mechanics/noisemask.cpp
    mechanics/alignment.cpp
    mechanics/sensor.cpp
    processors/eventdepictor.cpp
    processors/largesynchronizer.cpp
    processors/synchronizer.cpp
    processors/clustermaker.cpp
    processors/configprocessors.cpp
    processors/processors.cpp
    processors/trackmaker.cpp
    processors/trackmatcher.cpp
    storage/cluster.cpp
    storage/event.cpp
    storage/hit.cpp
    storage/track.cpp
    storage/plane.cpp
    storage/storageio.cpp)

add_library(pt-common STATIC ${Common_SRC})
target_include_directories(pt-common PUBLIC ${ROOT_INCLUDE_DIRS})
target_link_libraries(pt-common PUBLIC ${ROOT_LIBRARIES})

add_executable(proteus main.cpp configparser.cpp inputargs.cpp)
target_link_libraries(proteus pt-common)
