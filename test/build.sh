#!/bin/sh -ex
#
# build a fresh copy of the software

rm -rf build
mkdir build
(cd build && cmake ../..)
cmake --build build
