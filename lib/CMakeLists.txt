set(SOURCES
    analyzers/analyzer.cpp
    analyzers/basicefficiency.cpp
    analyzers/clusters.cpp
    analyzers/correlations.cpp
#   analyzers/depiction.cpp
    analyzers/distances.cpp
#   analyzers/efficiency.cpp
    analyzers/events.cpp
    analyzers/eventprinter.cpp
    analyzers/hits.cpp
    analyzers/tracks.cpp
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
    storage/cluster.cpp
    storage/event.cpp
    storage/hit.cpp
    storage/track.cpp
    storage/trackstate.cpp
    storage/plane.cpp
    tracking/trackfinder.cpp
    tracking/straightfitter.cpp
    tracking/tracking.cpp
    utils/application.cpp
    utils/arguments.cpp
    utils/config.cpp
    utils/densemask.cpp
    utils/eventloop.cpp
    utils/logger.cpp)
file(GLOB_RECURSE HEADERS LIST_DIRECTORIES false *.h)

add_library(pt-lib STATIC ${SOURCES} ${HEADERS})
target_include_directories(pt-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS})
target_link_libraries(pt-lib PUBLIC ${ROOT_LIBRARIES})
