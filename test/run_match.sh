#!/bin/sh -ex
#
# match telescope tracks and dut hits

RUN=$1; shift
FLAGS="-v $@" # e.g. -n 10000, to process only the first 10k events

rundir=$(printf "run%06d" $RUN)
rawfile=$(printf "raw/run%06d.root" $RUN)
prefix=$(printf "output/run%06d-" $RUN)

pt-match $FLAGS \
  -d ${rundir}/device.toml \
  -c ${rundir}/analysis.toml \
  ${prefix}track-data.root ${prefix}match
