cmake_minimum_required(VERSION 3.7)

project(Proteus VERSION 1.4.0 LANGUAGES CXX)

option(PROTEUS_ENABLE_DOC "Enable the documentation build" OFF)
option(PROTEUS_USE_EUDAQ "Build EUDAQ file reader" OFF)

# build as release if nothing else was requested
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# dependencies
set(EIGEN_PREFER_EXPORTED_EIGEN_CMAKE_CONFIGURATION ON)
find_package(Eigen 3.2.9 REQUIRED)
find_package(ROOT 6.10 REQUIRED COMPONENTS Hist Tree)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS}")
if(PROTEUS_USE_EUDAQ)
  # always use the FindEUDAQ module provided in the repo
  find_package(EUDAQ MODULE REQUIRED)
endif()

# after setting CMAKE_CXX_FLAGS to be able to pick up existing -std=... flags
include(RequireCXX14)

# which source files to clang-format
file(GLOB_RECURSE
  CHECK_CXX_SOURCE_FILES
  lib/*.[tch]pp lib/*.h
  exe/*.[tch]pp exe/*.h)
include("cmake/clang-cpp-checks.cmake")

include_directories(external/tinytoml/include)

if(PROTEUS_ENABLE_DOC)
  add_subdirectory(doc)
endif()
add_subdirectory(external/gbl)
add_subdirectory(lib)
add_subdirectory(exe)

# activation script to use the build directory directly
set(BASEDIR ${PROJECT_BINARY_DIR})
configure_file(cmake/activate.sh.in activate.sh @ONLY)
# activation script to be installed to the install directory
set(BASEDIR ${CMAKE_INSTALL_PREFIX})
configure_file(cmake/activate.sh.in DONOTUSE-activate.sh @ONLY)
install(
  FILES ${CMAKE_BINARY_DIR}/DONOTUSE-activate.sh
  DESTINATION .
  RENAME activate.sh)
