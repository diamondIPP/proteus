set(SOURCES
    analyzers/analyzer.cpp
    analyzers/clusters.cpp
    analyzers/correlations.cpp
    analyzers/distances.cpp
    analyzers/efficiency.cpp
    analyzers/events.cpp
    analyzers/eventprinter.cpp
    analyzers/hits.cpp
    analyzers/tracks.cpp
    analyzers/residuals.cpp
    io/match.cpp
    io/merger.cpp
    io/rceroot.cpp
    io/timepix3.cpp
    io/reader.cpp
    mechanics/device.cpp
    mechanics/geometry.cpp
    mechanics/pixelmasks.cpp
    mechanics/sensor.cpp
    processors/applygeometry.cpp
    processors/applyregions.cpp
    processors/clusterizer.cpp
    processors/hitmapper.cpp
    processors/matcher.cpp
    processors/processor.cpp
    processors/setupsensors.cpp
    storage/cluster.cpp
    storage/event.cpp
    storage/hit.cpp
    storage/sensorevent.cpp
    storage/track.cpp
    storage/trackstate.cpp
    tracking/propagation.cpp
    tracking/trackfinder.cpp
    tracking/straightfitter.cpp
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
