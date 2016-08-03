#!/bin/sh -ex
#
# run the processing chain for the reference telescope

CFG=configs/global.cfg
REF=configs/device-ref.cfg
RUN=1066
FLAGS="--numEvents 10000 --printLevel 1"

RAWFILE=$(realpath $(printf "raw/cosmic_%06d.root" $RUN))
RUNDIR=$(printf "run%06d" $RUN)
PREFIX=$(printf "trees/run%06d-" $RUN)

source build/activate.sh
cd $RUNDIR

cp -f comparison/unaligned-ref.dat alignment-ref.dat

# assume noise-free telescope; no noise mask
# Judith $FLAGS -c applyMask -i $RAWFILE -o ${PREFIX}ref.root -t $CFG -r $REF
# Judith $FLAGS -c noiseScan -i ${PREFIX}ref.root -t $CFG -r $REF
Judith $FLAGS -c applyMask -i $RAWFILE -o ${PREFIX}ref.root -t $CFG -r $REF
Judith $FLAGS -c process -i ${PREFIX}ref.root -o ${PREFIX}ref-p.root -t $CFG -r $REF
Judith $FLAGS -c coarseAlign -i ${PREFIX}ref-p.root -t $CFG -r $REF
Judith $FLAGS -c fineAlign -i ${PREFIX}ref-p.root -t $CFG -r $REF
Judith $FLAGS -c process -i ${PREFIX}ref.root -o ${PREFIX}ref-pa.root -t $CFG -r $REF
Judith $FLAGS -c analysis -i ${PREFIX}ref-pa.root -R ${PREFIX}ref-pa-analysis.root -t $CFG -r $REF
