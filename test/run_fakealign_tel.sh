#!/bin/sh
#
# run telescope alignment starting from a perfect alignment.

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

mkdir -p output/${dataset}

echo "=== using $(which pt-align)"

pt-align ${flags} -u tel_fine \
  -g geometry/${dataset}.toml \
  ${datadir}/${dataset}.root output/${dataset}/fakealign_tel_fine
