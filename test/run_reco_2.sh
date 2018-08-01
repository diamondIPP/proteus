#!/bin/sh
#
# run tracking and dut matching with geometry from proteus

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

#geo="geometry/${dataset}.toml"

mkdir -p output/${dataset}-2

echo "=== using $(which pt-track)"
echo "=== using $(which pt-match)"

pt-track -g output/${dataset}-2/${dataset}-align_dut_fine-geo.toml ${flags} ${datadir}/${dataset}.root output/${dataset}-2/${dataset}-track

pt-match -g output/${dataset}-2/${dataset}-align_dut_fine-geo.toml ${flags} output/${dataset}-2/${dataset}-track-data.root output/${dataset}-2/${dataset}-match
