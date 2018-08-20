#!/bin/sh
#
# run dut alignment

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

mkdir -p output/${dataset}

echo "=== using $(which pt-align)"

pt-align ${flags} -u dut_coarse -g geometry/${dataset}-telescope.toml \
  ${datadir}/${dataset}.root output/${dataset}/align_dut_coarse
pt-align ${flags} -u dut_fine -g output/${dataset}/align_dut_coarse-geo.toml \
  ${datadir}/${dataset}.root output/${dataset}/align_dut_fine
