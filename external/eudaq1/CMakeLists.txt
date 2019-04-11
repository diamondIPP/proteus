# build minimal EUDAQ 1.x as an external project

cmake_minimum_required(VERSION 2.8.12)

project(proteus-external-eudaq1 NONE)

include(ExternalProject)
ExternalProject_Add(
  eudaq1
  GIT_REPOSITORY "https://github.com/eudaq/eudaq.git"
  GIT_TAG "v1.9.1"
  CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
             "-DINSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
             "-DBUILD_TESTING=off"
             "-DBUILD_gui=off"
             "-DBUILD_ni=off"
             "-DBUILD_onlinemon=off"
             "-DBUILD_runsplitter=off")
