#!/bin/sh
#
# run telescope alignment

set -ex

datadir=${DATADIR:-data}
nalign=${NALIGN:-25000}
dataset=$1; shift
flags=$@

mkdir -p output/${dataset}-chain

echo "=== using $(which pt-align)"

pt-align ${flags} -u tel_coarse \
  -m output/${dataset}-chain/noisescan-mask.toml \
  -n ${nalign} \
  ${datadir}/${dataset}.root output/${dataset}-chain/align_tel_coarse
pt-align ${flags} -u tel_fine \
  -g output/${dataset}-chain/align_tel_coarse-geo.toml \
  -m output/${dataset}-chain/noisescan-mask.toml \
  -n ${nalign} \
  ${datadir}/${dataset}.root output/${dataset}-chain/align_tel_fine
