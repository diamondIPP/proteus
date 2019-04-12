#!/bin/sh
#
# run noisescan for telescope and dut

set -ex

. ./run_common.sh

pt-noisescan ${flags} -g ${datasetdir}/geometry-initial.toml \
  ${data} ${output_prefix}chain-noisescan
