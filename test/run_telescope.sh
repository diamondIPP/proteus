#!/bin/bash -ex
#
# run the processing chain for the reference telescope

RUN=${1-1066}
FLAGS="-d device.toml" # e.g. -n 10000, to process 10k events

RAWFILE=$(readlink -f $(printf "raw/run%06d.root" $RUN))
RUNDIR=$(printf "run%06d" $RUN)
PREFIX=$(printf "output/run%06d-" $RUN)

source build/activate.sh
cd $RUNDIR

mkdir -p output

# determine noise masks
pt-noisescan $FLAGS -c configs/noisescan_tel.toml $RAWFILE ${PREFIX}noisescan_tel
# clusterize and find tracks
pt-track $FLAGS -c configs/analysis.toml $RAWFILE ${PREFIX}unaligned
# TODO 2016-11-14 msmk: run alignment
