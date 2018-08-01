#!/bin/sh
#
# run tracking and dut matching with geometry from proteus

set -ex

datadir=${DATADIR:-data}
dataset=$1; shift
flags=$@ # e.g. -n 10000, to process only the first 10k events

./run_noisescan_2.sh ${dataset} ${flags}
./run_align_tel_2.sh ${dataset} ${flags}
./run_align_dut_2.sh ${dataset} ${flags}
./run_reco_2.sh ${dataset} ${flags}

