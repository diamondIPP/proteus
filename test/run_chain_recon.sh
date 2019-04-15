#!/bin/sh
#
# run tracking and dut matching with geometry from proteus

set -ex

nalign=${NALIGN:-25000}
. ./run_common.sh

pt-recon ${flags} \
  -g ${output_prefix}chain-align_dut_fine-geo.toml \
  -m ${output_prefix}chain-noisescan-mask.toml \
  -s ${nalign} \
  ${data} ${output_prefix}chain-recon
