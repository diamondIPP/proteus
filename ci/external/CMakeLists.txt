# external dependencies that can not be included directly w/ the code

cmake_minimum_required(VERSION 2.8.12)

project(proteus-external NONE)

include(ExternalProject)

ExternalProject_Add(
  eudaq
  GIT_REPOSITORY "https://github.com/eudaq/eudaq.git"
  GIT_TAG "v2.0.0"
  CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
             "-DEUDAQ_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
             "-DEUDAQ_BUILD_EXECUTABLE=off"
             "-DEUDAQ_BUILD_GUI=off"
             "-DEUDAQ_BUILD_STDEVENT_MONITOR=off"
             "-DEUDAQ_LIBRARY_BUILD_LCIO=off")
