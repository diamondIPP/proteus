#!/bin/sh
#
# run dut alignment starting from a perfect alignment.

set -ex

source ./run_common.sh

pt-align ${flags} -u dut_residuals \
  -g ${datasetdir}/geometry.toml \
  ${data} ${output_prefix}fakealign_dut_residuals
pt-align ${flags} -u dut_localchi2 \
  -g ${datasetdir}/geometry.toml \
  ${data} ${output_prefix}fakealign_dut_localchi2
