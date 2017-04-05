cmake_minimum_required(VERSION 2.8.12)

project(Proteus CXX)

set(Proteus_VERSION v1.1.3)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

find_package(ROOT REQUIRED COMPONENT GenVector)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
# after setting CMAKE_CXX_FLAGS to be able to pick up existing -std=... flags
include(RequireCXX11)

include_directories(external/tinytoml/include)

add_subdirectory(doc)
add_subdirectory(src)

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
