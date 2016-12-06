#!/bin/sh -ex
#
# run alignment for the telescope

RUN=$1; shift
FLAGS=$@ # e.g. -n 10000, to process only the first 10k events

rundir=$(printf "run%06d" $RUN)
rawfile=$(printf "raw/run%06d.root" $RUN)
prefix=$(printf "output/run%06d-" $RUN)

pt-align $FLAGS -d ${rundir}/device.toml \
  -c ${rundir}/configs/dut_align_coarse.toml \
  -g ${rundir}/geometry/tel_aligned.toml \
  ${rawfile} ${prefix}dut_align0
pt-align $FLAGS -d ${rundir}/device.toml \
  -c ${rundir}/configs/dut_align_fine.toml \
  -g ${prefix}dut_align0-geo.toml \
  ${rawfile} ${prefix}dut_align1
