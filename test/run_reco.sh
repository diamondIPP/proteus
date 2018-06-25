#!/bin/sh
#
# run tracking and dut matching with correct geometry

set -ex

dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

geo="geometry/${dataset}.toml"

echo "=== using $(which pt-track)"
echo "=== using $(which pt-match)"

mkdir -p output
pt-track -g ${geo} ${flags} data/${dataset}.root output/${dataset}-track
pt-match -g ${geo} ${flags} output/${dataset}-track-data.root output/${dataset}-match
