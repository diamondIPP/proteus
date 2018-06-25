#!/bin/sh
#
# run telescope alignment

set -ex

dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

echo "=== using $(which pt-align)"

pt-align ${flags} -u tel_coarse \
  data/${dataset}.root output/${dataset}-align_tel_coarse
pt-align ${flags} -u tel_fine -g output/${dataset}-align_tel_coarse-geo.toml \
  data/${dataset}.root output/${dataset}-align_tel_fine
