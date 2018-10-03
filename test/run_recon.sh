#!/bin/sh
#
# run tracking and dut matching with correct geometry

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

mkdir -p output/${dataset}

geo="geometry/${dataset}.toml"

echo "=== using $(which pt-recon)"

pt-recon -g ${geo} ${flags} ${datadir}/${dataset}.root output/${dataset}/recon
