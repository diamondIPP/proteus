#!/bin/sh
#
# run telescope alignment

set -ex

source ./run_common.sh

# rough align w/ correlations
pt-align ${flags} -u tel_correlations_inner \
  -g ${datasetdir}/geometry-telescope_first_last.toml \
  ${data} ${output_prefix}align_tel_correlations
# fine align w/ multiple independent methods
pt-align ${flags} -u tel_residuals \
  -g ${output_prefix}align_tel_correlations-geo.toml \
  ${data} ${output_prefix}align_tel_residuals
pt-align ${flags} -u tel_localchi2 \
  -g ${output_prefix}align_tel_correlations-geo.toml \
  ${data} ${output_prefix}align_tel_localchi2
