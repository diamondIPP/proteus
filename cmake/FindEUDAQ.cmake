# Copyright 2017,2019 Moritz Kiehn <msmk@cern.ch>
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
# point to the EUDAQ root directory.
#
# The following variables will be defined::
#
#   EUDAQ_FOUND        - True if a EUDAQ installation was found
#   EUDAQ_INCLUDE_DIRS - The EUDAQ include directories
#   EUDAQ_LIBRARIES    - The EUDAQ library target(s)
#   EUDAQ_VERSION      - The EUDAQ version
#

# ensure EUDAQ_DIR and install directory are in search
list(APPEND CMAKE_PREFIX_PATH $ENV{EUDAQ_DIR} ${CMAKE_INSTALL_PREFIX})

# try to find eudaq v2 that already comes with a cmake config
find_package(eudaq QUIET CONFIG)

if(eudaq_FOUND)
  # cmake has found a config file for eudaq
  # EUDAQ_INCLUDE_DIRS, EUDAQ_LIBRARIES, eudaq_core target already set
  message(STATUS "Found EUDAQ_INCLUDE_DIRS=${EUDAQ_INCLUDE_DIRS}")
  message(STATUS "Found EUDAQ_LIBRARIES=${EUDAQ_LIBRARIES}")

  # set core library for common found check later
  set(EUDAQ_CORE_LIBRARY eudaq_core)

  # identify the core include directory and prune non-existing dirs
  set(EUDAQ_INCLUDE_DIR)
  set(_existing_incdirs)
  foreach(_dir IN LISTS EUDAQ_INCLUDE_DIRS)
    if(EXISTS "${_dir}" AND IS_DIRECTORY "${_dir}")
      list(APPEND _existing_incdirs ${_dir})
    endif()
    if(EXISTS "${_dir}/eudaq/Config.hh")
      set(EUDAQ_INCLUDE_DIR ${_dir})
    endif()
  endforeach()
  set(EUDAQ_INCLUDE_DIRS ${_existing_incdirs})

  # eudaq sets 'vX.Y.Z' version string; make cmake-compatible by stripping 'v'
  if(eudaq_VERSION)
    string(REGEX REPLACE "v?([0-9.]+)" "\\1" EUDAQ_VERSION "${eudaq_VERSION}")
    unset(eudaq_VERSION)
  else()
    # fallback; since we found EUDAQ via a config file, it must be version 2+
    set(EUDAQ_VERSION "2")
  endif()
else()
  # no config file found, try to find eudaq v1 manually
  find_library(
    EUDAQ_CORE_LIBRARY
    NAMES EUDAQ
    PATH_SUFFIXES lib)
  find_path(
    EUDAQ_INCLUDE_DIR
    NAMES eudaq/Config.hh eudaq/Platform.hh
    PATH_SUFFIXES include main/include)

  set(EUDAQ_INCLUDE_DIRS ${EUDAQ_INCLUDE_DIR})
  set(EUDAQ_LIBRARIES ${EUDAQ_CORE_LIBRARY})
  set(EUDAQ_VERSION "1")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  EUDAQ
  FOUND_VAR EUDAQ_FOUND
  REQUIRED_VARS EUDAQ_CORE_LIBRARY EUDAQ_INCLUDE_DIR
  VERSION_VAR EUDAQ_VERSION)

mark_as_advanced(EUDAQ_CORE_LIBRARY EUDAQ_INCLUDE_DIR)
