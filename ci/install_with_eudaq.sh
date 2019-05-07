#!/bin/sh
#
# build and install proteus together with a minimal EUDAQ version

eudaq_version=$1; shift
install_dir=$1; shift
extra_cmake_args=$@

build_dir=$(mktemp --directory)
source_dir=$(pwd)
install_dir=$(realpath ${install_dir})
cmake_args=" \
  -GNinja \
  -DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=on \
  -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=on \
  -DCMAKE_INSTALL_PREFIX=${install_dir} \
  ${extra_cmake_args}"

mkdir -p ${build_dir}/eudaq ${build_dir}/proteus

# build eudaq
(
  cd ${build_dir}/eudaq
  rm -rf *
  cmake ${cmake_args} ${source_dir}/external/${eudaq_version}
  cmake --build . # installs automatically
)
# build proteus
(
  cd ${build_dir}/proteus
  rm -rf *
  cmake ${cmake_args} -DPROTEUS_USE_EUDAQ=on ${source_dir}
  cmake --build . -- install
)

