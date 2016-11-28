#!/bin/sh -ex
#
# run alignment for the telescope

RUN=$1; shift
FLAGS=$@ # e.g. -n 10000, to process only the first 10k events

rundir=$(printf "run%06d" $RUN)
rawfile=$(printf "raw/run%06d.root" $RUN)
prefix=$(printf "output/run%06d-" $RUN)

source build/activate.sh
mkdir -p output

pt-align $FLAGS -d ${rundir}/device.toml \
  -c ${rundir}/configs/tel_align_coarse.toml \
  -g ${rundir}/geometry/unaligned.toml \
  ${rawfile} ${prefix}tel_align0
pt-align $FLAGS -d ${rundir}/device.toml \
  -c ${rundir}/configs/tel_align_fine.toml \
  -g ${prefix}tel_align0-geo.toml \
  ${rawfile} ${prefix}tel_align1
