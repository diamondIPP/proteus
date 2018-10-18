#!/bin/sh
#
# run dut alignment

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

mkdir -p output/${dataset}

echo "=== using $(which pt-align)"

pt-align ${flags} -u dut_correlations \
  -g geometry/${dataset}-telescope.toml \
  ${datadir}/${dataset}.root output/${dataset}/align_dut_correlations
# align the device-under-test independently using multiple methods
pt-align ${flags} -u dut_residuals \
  -g output/${dataset}/align_dut_correlations-geo.toml \
  ${datadir}/${dataset}.root output/${dataset}/align_dut_residuals
pt-align ${flags} -u dut_localchi2 \
  -g output/${dataset}/align_dut_correlations-geo.toml \
  ${datadir}/${dataset}.root output/${dataset}/align_dut_localchi2
