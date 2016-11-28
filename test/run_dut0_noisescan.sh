#!/bin/sh -ex
#
# run noisescan for the first device-under-test

RUN=$1; shift
FLAGS=$@ # e.g. -n 10000, to process only the first 10k events

rundir=$(printf "run%06d" $RUN)
rawfile=$(printf "raw/run%06d.root" $RUN)
prefix=$(printf "output/run%06d-" $RUN)

source build/activate.sh
mkdir -p output

pt-noisescan $FLAGS -d ${rundir}/device.toml \
  -c ${rundir}/configs/noisescan_dut0.toml \
  ${rawfile} ${prefix}dut0_noisescan
