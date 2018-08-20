#!/bin/sh
#
# run noisescan for telescope and dut

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

echo "=== using $(which pt-noisescan)"

pt-noisescan ${flags} ${datadir}/${dataset}.root output/${dataset}-noisescan