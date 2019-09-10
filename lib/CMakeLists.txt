set(proteus_SOURCES
    alignment/correlationsaligner.cpp
    alignment/localchi2aligner.cpp
    alignment/residualsaligner.cpp
    analyzers/clusters.cpp
    analyzers/correlations.cpp
    analyzers/distances.cpp
    analyzers/efficiency.cpp
    analyzers/eventprinter.cpp
    analyzers/globaloccupancy.cpp
    analyzers/hits.cpp
    analyzers/noisescan.cpp
    analyzers/tracks.cpp
    analyzers/residuals.cpp
    io/match.cpp
    io/merger.cpp
    io/open.cpp
    io/rceroot.cpp
    io/timepix3.cpp
    loop/eventloop.cpp
    mechanics/device.cpp
    mechanics/geometry.cpp
    mechanics/pixelmasks.cpp
    mechanics/sensor.cpp
    processors/applylocaltransform.cpp
    processors/applyregions.cpp
    processors/clusterizer.cpp
    processors/hitmapper.cpp
    processors/matcher.cpp
    processors/setupsensors.cpp
    storage/cluster.cpp
    storage/event.cpp
    storage/hit.cpp
    storage/sensorevent.cpp
    storage/track.cpp
    storage/trackstate.cpp
    tracking/gblfitter.cpp
    tracking/propagation.cpp
    tracking/trackfinder.cpp
    tracking/setupfitter.cpp
    tracking/straightfitter.cpp
    utils/application.cpp
    utils/arguments.cpp
    utils/config.cpp
    utils/densemask.cpp
    utils/logger.cpp
    utils/root.cpp)
file(GLOB_RECURSE proteus_HEADERS LIST_DIRECTORIES false *.h)

# optional components
set(proteus_PRIVATE_DEFINITIONS)
set(proteus_PRIVATE_INCLUDE_DIRS)
set(proteus_PRIVATE_LIBRARIES)
if(PROTEUS_USE_EUDAQ)
  if(EUDAQ_VERSION VERSION_LESS "2")
    list(APPEND proteus_SOURCES io/eudaq1.cpp)
    list(APPEND proteus_PRIVATE_DEFINITIONS -DPT_USE_EUDAQ1)
  else()
    list(APPEND proteus_SOURCES io/eudaq2.cpp)
    list(APPEND proteus_PRIVATE_DEFINITIONS -DPT_USE_EUDAQ2)
  endif()
  list(APPEND proteus_PRIVATE_INCLUDE_DIRS ${EUDAQ_INCLUDE_DIRS})
  list(APPEND proteus_PRIVATE_LIBRARIES ${EUDAQ_LIBRARIES})
endif()

add_library(proteus STATIC ${proteus_SOURCES} ${proteus_HEADERS})
target_compile_definitions(
  proteus
  PRIVATE ${proteus_PRIVATE_DEFINITIONS})
# Eigen as SYSTEM include to suppress warnings
target_include_directories(
  proteus
  SYSTEM PUBLIC ${EIGEN_INCLUDE_DIRS}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS}
  PRIVATE ${proteus_PRIVATE_INCLUDE_DIRS})
target_link_libraries(
  proteus
  PUBLIC ROOT::Hist ROOT::Tree
  PRIVATE gbl ${proteus_PRIVATE_LIBRARIES})
