#!/bin/sh
#
# run telescope alignment starting from a perfect alignment.

set -ex

source ./run_common.sh

pt-align ${flags} -u tel_residuals \
  -g ${datasetdir}/geometry.toml \
  ${data} ${output_prefix}fakealign_tel_residuals
pt-align ${flags} -u tel_localchi2 \
  -g ${datasetdir}/geometry.toml \
  ${data} ${output_prefix}fakealign_tel_localchi2
