#!/bin/sh -ex
#
# run the noisescan for the telescope

RUN=$1; shift
FLAGS="-v $@" # e.g. -n 10000, to process only the first 10k events

rundir=$(printf "run%06d" $RUN)
rawfile=$(printf "raw/run%06d.root" $RUN)
prefix=$(printf "output/run%06d-" $RUN)

pt-noisescan $FLAGS \
  -d ${rundir}/device.toml \
  -c ${rundir}/noisescan.toml -u tel \
  ${rawfile} ${prefix}tel_noisescan
