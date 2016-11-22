cmake_minimum_required(VERSION 2.8.12)

project(Proteus CXX)

set(Proteus_VERSION v1.0.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

find_package(ROOT REQUIRED COMPONENT GenVector)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

include_directories(external/tinytoml/include)

add_subdirectory(doc)
add_subdirectory(src)

# activation script to use the build products
configure_file(activate.sh.in activate.sh @ONLY)
