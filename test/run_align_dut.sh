#!/bin/sh
#
# run dut alignment

set -ex

source ./run_common.sh

# rough align w/ correlations
pt-align ${flags} -u dut_correlations \
  -g ${datasetdir}/geometry-telescope.toml \
  ${data} ${output_prefix}align_dut_correlations
# fine align w/ multiple independent methods
pt-align ${flags} -u dut_residuals \
  -g ${output_prefix}align_dut_correlations-geo.toml \
  ${data} ${output_prefix}align_dut_residuals
pt-align ${flags} -u dut_localchi2 \
  -g ${output_prefix}align_dut_correlations-geo.toml \
  ${data} ${output_prefix}align_dut_localchi2
