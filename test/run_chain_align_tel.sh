#!/bin/sh
#
# run telescope alignment

set -ex

nalign=${NALIGN:-25000}
source ./run_common.sh

pt-align ${flags} -u tel_correlations \
  -g ${datasetdir}/geometry-initial.toml \
  -m ${output_prefix}chain-noisescan-mask.toml \
  -n ${nalign} \
  ${data} ${output_prefix}chain-align_tel_coarse
pt-align ${flags} -u tel_residuals \
  -g ${output_prefix}chain-align_tel_coarse-geo.toml \
  -m ${output_prefix}chain-noisescan-mask.toml \
  -n ${nalign} \
  ${data} ${output_prefix}chain-align_tel_fine
