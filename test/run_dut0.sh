#!/bin/bash -ex
#
# run the processing chain for the first dut
# WARNING assumes that run_telescope.sh has been sucessfully called

RUN=${1-1066}
DEV="device.toml"
FLAGS="" # e.g. -n 10000, to process 10k events

RAWFILE=$(readlink -f $(printf "raw/run%06d.root" $RUN))
RUNDIR=$(printf "run%06d" $RUN)
PREFIX=$(printf "output/run%06d-" $RUN)

source build/activate.sh
cd $RUNDIR

mkdir -p output

pt-noisescan $FLAGS -d $DEV -c configs/noisescan_dut0.toml $RAWFILE ${PREFIX}noisescan_dut0
# TODO 2016-11-14 msmk: run alignment
# TODO 2016-11-18 msmk: run analysis
