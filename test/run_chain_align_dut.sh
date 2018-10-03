#!/bin/sh
#
# run dut alignment

set -ex

datadir=${DATADIR:-data}
nalign=${NALIGN:-25000}
dataset=$1; shift
flags=$@

mkdir -p output/${dataset}-chain

echo "=== using $(which pt-align)"

pt-align ${flags} -u dut_coarse \
  -g output/${dataset}-chain/align_tel_fine-geo.toml \
  -m output/${dataset}-chain/noisescan-mask.toml \
  -n ${nalign} \
  ${datadir}/${dataset}.root output/${dataset}-chain/align_dut_coarse
pt-align ${flags} -u dut_fine \
  -g output/${dataset}-chain/align_dut_coarse-geo.toml \
  -m output/${dataset}-chain/noisescan-mask.toml \
  -n ${nalign} \
  ${datadir}/${dataset}.root output/${dataset}-chain/align_dut_fine
