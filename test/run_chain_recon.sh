#!/bin/sh
#
# run tracking and dut matching with geometry from proteus

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events
nalign=10000


mkdir -p output/${dataset}-chain

echo "=== using $(which pt-recon)"

pt-recon -g output/${dataset}-chain/align_dut_fine-geo.toml \
  -s ${nalign} ${flags} \
  ${datadir}/${dataset}.root output/${dataset}-chain/recon

