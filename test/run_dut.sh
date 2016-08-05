#!/bin/sh -ex
#
# run the processing chain for the dut

CFG=configs/global.cfg
REF=configs/device-ref.cfg
DUT=configs/device-dut0.cfg
RUN=1066
# FLAGS="--numEvents 10000"
FLAGS=

RAWFILE=$(realpath $(printf "raw/cosmic_%06d.root" $RUN))
RUNDIR=$(printf "run%06d" $RUN)
PREFIX=$(printf "output/run%06d-" $RUN)

MACROS=$(realpath "../macros")

source build/activate.sh
cd $RUNDIR

cp -f comparison/unaligned-dut0.dat alignment-dut0.dat
mkdir -p output

Judith $FLAGS -c applyMask -i $RAWFILE -o ${PREFIX}dut0.root -t $CFG -r $DUT
Judith $FLAGS -c noiseScan -i ${PREFIX}dut0.root -t $CFG -r $DUT
Judith $FLAGS -c applyMask -i $RAWFILE -o ${PREFIX}dut0.root -t $CFG -r $DUT
# # extra mapping and selection for ccpd devices
root -l -q "${MACROS}/Regions.cc+(\"${PREFIX}dut0\",0,5,157,173,165,168)"
root -l -q "${MACROS}/mapping.cc+(\"${PREFIX}dut0_Stime.root\",\"${PREFIX}dut0_Stime-m.root\")"
# alignment and final analysis
Judith $FLAGS -c coarseAlignDUT -t $CFG -r $REF -d $DUT \
    -i ${PREFIX}ref-p.root -I ${PREFIX}dut0_Stime-m.root
Judith $FLAGS -c fineAlignDUT -t $CFG -r $REF -d $DUT \
    -i ${PREFIX}ref-p.root -I ${PREFIX}dut0_Stime-m.root
Judith $FLAGS -c analysisDUT -t $CFG -r $REF -d $DUT \
    -i ${PREFIX}ref-p.root -I ${PREFIX}dut0_Stime-m.root \
    -R ${PREFIX}dut0-analysis.root
