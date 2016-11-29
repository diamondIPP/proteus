#!/bin/sh -ex
#
# run the processing chain for the reference telescope

RUN=${1-1066}
DEV="device.toml"
FLAGS="" # e.g. -n 10000, to process only the first 10k events

RAWFILE=$(readlink -f $(printf "raw/run%06d.root" $RUN))
RUNDIR=$(printf "run%06d" $RUN)
PREFIX=$(printf "output/run%06d-" $RUN)

source build/activate.sh
cd $RUNDIR

mkdir -p output

pt-noisescan $FLAGS -d $DEV -c configs/noisescan_tel.toml $RAWFILE ${PREFIX}noisescan_tel
# TODO 2016-11-14 msmk: run alignment
pt-track $FLAGS -d $DEV -c configs/analysis.toml $RAWFILE ${PREFIX}unaligned
