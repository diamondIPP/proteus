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
# Find a EUDAQ installation. Searches common system directories by default.
# This can be changed by setting the ``EUDAQ_DIR`` environment variable to
# point to the EUDAQ root directory. For EUDAQ 1 this should be the path to
# the source directory and for EUDAQ 2 the path to the installation prefix.
#
# The following variables will be defined::
#
#   EUDAQ_FOUND     - True if the EUDAQ installation was found
#   EUDAQ_VERSION   - The EUDAQ version, at the moment either 1 or 2
#   EUDAQ_LIBRARIES - The EUDAQ library target
#
# Include directories are handled automatically via target properties.

find_path(
  EUDAQ_INCLUDE_DIR
  NAMES eudaq/Config.hh eudaq/Platform.hh
  PATH_SUFFIXES include main/include
  PATHS ENV EUDAQ_DIR)
find_library(
  EUDAQ_LIBRARY
  NAMES EUDAQ eudaq_core
  PATH_SUFFIXES lib
  PATHS ENV EUDAQ_DIR)

if(EXISTS "${EUDAQ_INCLUDE_DIR}/eudaq/Config.hh")
  file(
    STRINGS "${EUDAQ_INCLUDE_DIR}/eudaq/Config.hh" _eudaq_version_raw
    REGEX "^#define[ \t]PACKAGE_VERSION[ \t].*$"
    LIMIT_COUNT 1)
  string(
    REGEX REPLACE "^#define[ \t]PACKAGE_VERSION[ \t]+\"v(.*)\"$" "\\1"
    EUDAQ_VERSION "${_eudaq_version_raw}")
else()
  set(EUDAQ_VERSION "1")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  EUDAQ
  FOUND_VAR EUDAQ_FOUND
  REQUIRED_VARS EUDAQ_LIBRARY EUDAQ_INCLUDE_DIR
  VERSION_VAR EUDAQ_VERSION)

if(EUDAQ_FOUND)
  if(NOT TARGET EUDAQ::EUDAQ)
    add_library(EUDAQ::EUDAQ UNKNOWN IMPORTED)
    set_target_properties(
      EUDAQ::EUDAQ PROPERTIES
      IMPORTED_LOCATION "${EUDAQ_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${EUDAQ_INCLUDE_DIR}")
  endif()
  set(EUDAQ_LIBRARIES "EUDAQ::EUDAQ")
endif()

mark_as_advanced(EUDAQ_INCLUDE_DIR EUDAQ_LIBRARY)
