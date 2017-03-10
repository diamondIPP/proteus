set(common_SRCS
    alignment/correlationsaligner.cpp
    alignment/residualsaligner.cpp
    analyzers/analyzer.cpp
    analyzers/baseanalyzer.cpp
    analyzers/basicefficiency.cpp
    analyzers/clusterinfo.cpp
    analyzers/configanalyzers.cpp
    analyzers/correlations.cpp
    analyzers/depiction.cpp
    analyzers/distances.cpp
    analyzers/dualanalyzer.cpp
    analyzers/dutcorrelation.cpp
    analyzers/dutdepiction.cpp
    analyzers/eventinfo.cpp
    analyzers/eventprinter.cpp
    analyzers/exampledualanalyzer.cpp
    analyzers/examplesingleanalyzer.cpp
    analyzers/hitinfo.cpp
    analyzers/matchexporter.cpp
    analyzers/matching.cpp
    analyzers/noisescan.cpp
    analyzers/occupancy.cpp
    analyzers/singleanalyzer.cpp
    analyzers/syncfluctuation.cpp
    analyzers/trackinfo.cpp
    analyzers/dutresiduals.cpp
    analyzers/efficiency.cpp
    analyzers/residuals.cpp
    converters/kartelconvert.cpp
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
    mechanics/device.cpp
    mechanics/geometry.cpp
    mechanics/pixelmasks.cpp
    mechanics/sensor.cpp
    processors/applygeometry.cpp
    processors/applyregions.cpp
    processors/clustermaker.cpp
    processors/clusterizer.cpp
    processors/configprocessors.cpp
    processors/eventdepictor.cpp
    processors/hitmapper.cpp
    processors/largesynchronizer.cpp
    processors/matcher.cpp
    processors/processor.cpp
    processors/processors.cpp
    processors/setupsensors.cpp
    processors/synchronizer.cpp
    processors/trackfinder.cpp
    processors/trackfitter.cpp
    processors/tracking.cpp
    processors/trackmaker.cpp
    storage/cluster.cpp
    storage/event.cpp
    storage/hit.cpp
    storage/track.cpp
    storage/trackstate.cpp
    storage/plane.cpp
    storage/storageio.cpp
    utils/arguments.cpp
    utils/config.cpp
    utils/configparser.cpp
    utils/densemask.cpp
    utils/eventloop.cpp
    utils/logger.cpp
    utils/statistics.cpp
    application.cpp)
file(GLOB_RECURSE common_HDRS LIST_DIRECTORIES false *.h)

add_library(pt-common STATIC ${common_SRCS} ${common_HDRS})
target_include_directories(pt-common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS})
target_link_libraries(pt-common PUBLIC ${ROOT_LIBRARIES})

function(pt_add_tool name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} pt-common)
  install(TARGETS ${name} RUNTIME DESTINATION bin)
endfunction()

pt_add_tool(proteus main.cpp inputargs.cpp)
pt_add_tool(pt-align pt-align.cpp)
pt_add_tool(pt-cfg pt-cfg.cpp)
pt_add_tool(pt-match pt-match.cpp)
pt_add_tool(pt-noisescan pt-noisescan.cpp)
pt_add_tool(pt-track pt-track.cpp)
