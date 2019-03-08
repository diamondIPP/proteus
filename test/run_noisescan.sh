#!/bin/sh
#
# run noisescan for telescope and dut

set -ex

source ./run_common.sh

pt-noisescan ${flags} -g ${datasetdir}/geometry.toml \
  ${data} ${output_prefix}noisescan
