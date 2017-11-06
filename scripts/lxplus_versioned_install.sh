#!/bin/sh -e
#
# build and install proteus in a versioned directory on lxplus
#
# must be called from source directory with the install base directory
# as the first argument

install_base=$(readlink -f $1)
source_dir=${PWD}
build_dir=$(mktemp -d)
version=$(git describe --dirty)

source ${source_dir}/scripts/lxplus_platform.sh

install_dir=${install_base}/${version}/${platform}

echo "=== using sources from ${source_dir}"
echo "=== using LCG release ${lcg_version} on platform ${platform}"
echo "=== building in ${build_dir}"
echo "=== installing to ${install_dir}"

(
  cd ${build_dir}
  source ${lcg_view}/setup.sh
  cmake \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=${install_dir} \
    ${source_dir}
  cmake --build . -- install
  # construct setup script
  setup=${install_dir}/setup.sh
  cat ${source_dir}/scripts/lxplus_platform.sh > ${setup}
  echo "" >> ${setup}
  echo "# activate lcg release" >> ${setup}
  echo "source ${lcg_view}/setup.sh" >> ${setup}
  echo "" >> ${setup}
  echo "# activate proteus installation" >> ${setup}
  cat ${install_dir}/activate.sh >> ${setup}
)

rm -rf ${build_dir}
