#!/bin/sh -ex
#
# run alignment for the telescope

RUN=$1; shift
FLAGS=$@ # e.g. -n 10000, to process only the first 10k events

rundir=$(printf "run%06d" $RUN)
rawfile=$(printf "raw/run%06d.root" $RUN)
prefix=$(printf "output/run%06d-" $RUN)

pt-align $FLAGS \
  -d ${rundir}/device.toml -g ${rundir}/geometry/tel_aligned.toml \
  -c ${rundir}/analysis.toml -u dut_coarse \
  ${rawfile} ${prefix}dut_align0
pt-align $FLAGS \
  -d ${rundir}/device.toml -g ${prefix}dut_align0-geo.toml \
  -c ${rundir}/analysis.toml -u dut_fine \
  ${rawfile} ${prefix}dut_align1
