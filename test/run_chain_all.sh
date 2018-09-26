#!/bin/sh
#
# run tracking and dut matching with geometry from proteus

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

./run_chain_noisescan.sh ${dataset} ${flags}
./run_chain_align_tel.sh ${dataset} ${flags}
./run_chain_align_dut.sh ${dataset} ${flags}
./run_chain_recon.sh ${dataset} ${flags}

