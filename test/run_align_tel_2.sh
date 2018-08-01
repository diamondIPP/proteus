#!/bin/sh
#
# run telescope alignment

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events
nalign=10000

mkdir -p output/${dataset}-2

echo "=== using $(which pt-align)"

pt-align ${flags} -u tel_coarse \
  -m output/${dataset}-2/${dataset}-noisescan-mask.toml \
  -n ${nalign} \
  ${datadir}/${dataset}.root output/${dataset}-2/${dataset}-align_tel_coarse

pt-align ${flags} -u tel_fine -g output/${dataset}-2/${dataset}-align_tel_coarse-geo.toml \
  -m output/${dataset}-2/${dataset}-noisescan-mask.toml \
  -n ${nalign} \
  ${datadir}/${dataset}.root output/${dataset}-2/${dataset}-align_tel_fine
