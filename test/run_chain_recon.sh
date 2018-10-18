#!/bin/sh
#
# run tracking and dut matching with geometry from proteus

set -ex

datadir=${DATADIR:-data}
nalign=${NALIGN:-25000}
dataset=$1; shift
flags=$@

mkdir -p output/${dataset}-chain

echo "=== using $(which pt-recon)"

pt-recon ${flags} \
  -g output/${dataset}-chain/align_dut_fine-geo.toml \
  -s ${nalign} \
  ${datadir}/${dataset}.root output/${dataset}-chain/recon
