set(common_SRCS
    alignment/correlationsaligner.cpp
    alignment/residualsaligner.cpp
    analyzers/analyzer.cpp
    analyzers/basicefficiency.cpp
    analyzers/clusterinfo.cpp
    analyzers/correlations.cpp
#   analyzers/depiction.cpp
    analyzers/distances.cpp
#   analyzers/efficiency.cpp
    analyzers/eventinfo.cpp
    analyzers/eventprinter.cpp
    analyzers/hitinfo.cpp
    analyzers/noisescan.cpp
    analyzers/occupancy.cpp
    analyzers/trackinfo.cpp
    analyzers/residuals.cpp
#   converters/kartelconvert.cpp
    io/match.cpp
    io/rceroot.cpp
    io/reader.cpp
    io/writer.cpp
    mechanics/device.cpp
    mechanics/geometry.cpp
    mechanics/pixelmasks.cpp
    mechanics/sensor.cpp
    processors/applygeometry.cpp
    processors/applyregions.cpp
    processors/clusterizer.cpp
#   processors/eventdepictor.cpp
    processors/hitmapper.cpp
#   processors/largesynchronizer.cpp
    processors/matcher.cpp
    processors/processor.cpp
    processors/setupsensors.cpp
#   processors/synchronizer.cpp
    processors/trackfinder.cpp
    processors/trackfitter.cpp
    processors/tracking.cpp
    storage/cluster.cpp
    storage/event.cpp
    storage/hit.cpp
    storage/track.cpp
    storage/trackstate.cpp
    storage/plane.cpp
    utils/arguments.cpp
    utils/config.cpp
    utils/densemask.cpp
    utils/eventloop.cpp
    utils/logger.cpp
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

pt_add_tool(pt-align pt-align.cpp)
pt_add_tool(pt-cfg pt-cfg.cpp)
pt_add_tool(pt-match pt-match.cpp)
pt_add_tool(pt-noisescan pt-noisescan.cpp)
pt_add_tool(pt-track pt-track.cpp)
