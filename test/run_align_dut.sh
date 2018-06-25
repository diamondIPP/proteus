#!/bin/sh
#
# run dut alignment

set -ex

dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

echo "=== using $(which pt-align)"

pt-align ${flags} -u dut_coarse -g geometry/${dataset}-telescope.toml \
  data/${dataset}.root output/${dataset}-align_dut_coarse
pt-align ${flags} -u dut_fine -g output/${dataset}-align_dut_coarse-geo.toml \
  data/${dataset}.root output/${dataset}-align_dut_fine
