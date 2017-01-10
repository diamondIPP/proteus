#!/bin/sh -ex
#
# run telescope tracking

RUN=$1; shift
FLAGS=$@ # e.g. -n 10000, to process only the first 10k events

rundir=$(printf "run%06d" $RUN)
rawfile=$(printf "raw/run%06d.root" $RUN)
prefix=$(printf "output/run%06d-" $RUN)

pt-track $FLAGS \
  -d ${rundir}/device.toml \
  -c ${rundir}/analysis.toml \
  ${rawfile} ${prefix}track
