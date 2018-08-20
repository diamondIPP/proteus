#!/bin/sh
#
# run tracking and dut matching with geometry from proteus

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events
nalign=10000

#geo="geometry/${dataset}.toml"

mkdir -p output/${dataset}-chain

echo "=== using $(which pt-track)"
echo "=== using $(which pt-match)"

pt-track -g output/${dataset}-chain/align_dut_fine-geo.toml \
  -s ${nalign} ${flags} \
  ${datadir}/${dataset}.root output/${dataset}-chain/track

pt-match -g output/${dataset}-chain/align_dut_fine-geo.toml \
  -s ${nalign} ${flags} \
  output/${dataset}-chain/track-data.root \
  output/${dataset}-chain/match
