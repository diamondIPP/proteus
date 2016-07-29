#!/bin/sh -x
#
# build a fresh copy of the software; set up clean data directories

rm -rf build
mkdir build
(cd build && cmake ../..)
cmake --build build
