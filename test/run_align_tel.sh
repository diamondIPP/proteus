#!/bin/sh
#
# run telescope alignment

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

mkdir -p output/${dataset}

echo "=== using $(which pt-align)"

pt-align ${flags} -u tel_correlations_inner \
  -g geometry/${dataset}-telescope_first_last.toml \
  ${datadir}/${dataset}.root output/${dataset}/align_tel_correlations
# align the telescope independently using multiple methods
pt-align ${flags} -u tel_residuals \
  -g output/${dataset}/align_tel_correlations-geo.toml \
  ${datadir}/${dataset}.root output/${dataset}/align_tel_residuals
pt-align ${flags} -u tel_localchi2 \
  -g output/${dataset}/align_tel_correlations-geo.toml \
  ${datadir}/${dataset}.root output/${dataset}/align_tel_localchi2
