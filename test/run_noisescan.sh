#!/bin/sh
#
# run noisescan for telescope and dut

set -ex

dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

echo "=== using $(which pt-noisescan)"

pt-noisescan ${flags} data/${dataset}.root output/${dataset}-noisescan
