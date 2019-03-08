#!/bin/sh
#
# run dut alignment

set -ex

nalign=${NALIGN:-25000}
source ./run_common.sh

pt-align ${flags} -u dut_correlations \
  -g ${output_prefix}chain-align_tel_fine-geo.toml \
  -m ${output_prefix}chain-noisescan-mask.toml \
  -n ${nalign} \
  ${data} ${output_prefix}chain-align_dut_coarse
pt-align ${flags} -u dut_residuals \
  -g ${output_prefix}chain-align_dut_coarse-geo.toml \
  -m ${output_prefix}chain-noisescan-mask.toml \
  -n ${nalign} \
  ${data} ${output_prefix}chain-align_dut_fine
