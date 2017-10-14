# Copyright 2017 Moritz Kiehn <msmk@cern.ch>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

#.rst:
# FindEUDAQ
# ---------
#
# Finds a EUDAQ 2.0 installation.
# Searches common system directories by default. This can be changed by
# setting the ``EUDAQ_DIR`` environment variable to the root directory of the
# installation.
#
# This will define the following variables::
#
#   EUDAQ_FOUND   - True if the EUDAQ installation was found
#
# and the following imported targets::
#
#   EUDAQ::Core  - The EUDAQ core library
#

find_path(
  EUDAQ_INCLUDE_DIR
  NAMES eudaq/Platform.hh
  PATH_SUFFIXES include
  PATHS ENV EUDAQ_DIR)
find_library(
  EUDAQ_CORE_LIBRARY
  NAMES eudaq_core
  PATH_SUFFIXES lib
  PATHS ENV EUDAQ_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  EUDAQ
  FOUND_VAR EUDAQ_FOUND
  REQUIRED_VARS
    EUDAQ_CORE_LIBRARY
    EUDAQ_INCLUDE_DIR)

if(EUDAQ_FOUND AND NOT TARGET EUDAQ::Core)
  add_library(EUDAQ::Core UNKNOWN IMPORTED)
  set_target_properties(EUDAQ::Core PROPERTIES
    IMPORTED_LOCATION "${EUDAQ_CORE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${EUDAQ_INCLUDE_DIR}")
endif()

mark_as_advanced(
  EUDAQ_INCLUDE_DIR
  EUDAQ_CORE_LIBRARY)
