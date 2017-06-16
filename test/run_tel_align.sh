#!/bin/sh -ex
#
# run alignment for the telescope

RUN=$1; shift
FLAGS=$@ # e.g. -n 10000, to process only the first 10k events

rundir=$(printf "run%06d" $RUN)
rawfile=$(printf "raw/run%06d.root" $RUN)
prefix=$(printf "output/run%06d-" $RUN)

echo "using $(which pt-align)"

pt-align $FLAGS \
  -d ${rundir}/device.toml -g ${rundir}/geometry/unaligned.toml \
  -c ${rundir}/analysis.toml -u tel_coarse \
  ${rawfile} ${prefix}tel_align0
pt-align $FLAGS \
  -d ${rundir}/device.toml -g ${prefix}tel_align0-geo.toml \
  -c ${rundir}/analysis.toml -u tel_fine \
  ${rawfile} ${prefix}tel_align1
