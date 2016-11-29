#!/bin/sh -ex
#
# run the processing chain for the dut

CFG=configs/global.cfg
REF=configs/device-ref.cfg
DUT0=configs/device-dut0.cfg
DUT0M=configs/device-dut0_mapped.cfg
RUN=1066
# FLAGS="--numEvents 10000"
FLAGS=

RAWFILE=$(readlink -f $(printf "raw/run%06d.root" $RUN))
RUNDIR=$(printf "run%06d" $RUN)
PREFIX=$(printf "output/run%06d-" $RUN)

MACROS=$(realpath "../macros")

source build/activate.sh
cd $RUNDIR

# use prealigned geometry since alignment does currently not work
cp -f comparison/alignment-ref.dat alignment-ref.dat
cp -f comparison/alignment-dut0.dat alignment-dut0.dat
mkdir -p output

proteus $FLAGS -c applyMask -i $RAWFILE -o ${PREFIX}dut0.root -t $CFG -r $DUT0
proteus $FLAGS -c noiseScan -i ${PREFIX}dut0.root -t $CFG -r $DUT0
proteus $FLAGS -c applyMask -i $RAWFILE -o ${PREFIX}dut0.root -t $CFG -r $DUT0
# extra mapping and selection for ccpd devices
root -l -q "${MACROS}/Regions.cc+(\"${PREFIX}dut0\",0,5,157,173,165,168)"
root -l -q "${MACROS}/mapping.cc+(\"${PREFIX}dut0_Stime.root\",\"${PREFIX}dut0_Stime-m.root\")"
# alignment and final analysis

# TODO 2016-08-08 msmk:
# dut alignment and analysis does not work due to hard-coded
# region-of-interest cuts in the code base.
# needs to be updated when issues #11 and #12 are fixed.

# proteus $FLAGS -c coarseAlignDUT -t $CFG -r $REF -d $DUT \
#     -i ${PREFIX}ref-p.root -I ${PREFIX}dut0_Stime-m.root
# proteus $FLAGS -c fineAlignDUT -t $CFG -r $REF -d $DUT \
#     -i ${PREFIX}ref-p.root -I ${PREFIX}dut0_Stime-m.root
proteus $FLAGS -c process -t $CFG -r $DUT0M \
    -i ${PREFIX}dut0_Stime-m.root \
    -o ${PREFIX}dut0_Stime-p.root \
    -R ${PREFIX}dut0_Stime-p-hists.root
# proteus $FLAGS -c analysisDUT -t $CFG -r $REF -d $DUT0M \
#     -i ${PREFIX}ref-p.root \
#     -I ${PREFIX}dut0_Stime-p.root \
#     -R ${PREFIX}dut0_Stime-analysis.root
