#!/bin/sh -x
#
# run the processing chain for the reference telescope

CFG=configs/global.cfg
REF=configs/ref.cfg
RAWFILE=raw/cosmic_85_0875.root
RUNDIR=runs/run000875
PREFIX="${RUNDIR}/run00875-"

JUDITH="Judith --numEvents 10000 --printLevel 1"

source build/activate.sh
rm -rf $RUNDIR
mkdir -p $RUNDIR
cp -f comparison/alignment-ref.dat alignment

$JUDITH -c applyMask -i $RAWFILE -o ${PREFIX}ref.root -t $CFG -r $REF
$JUDITH -c noiseScan -i ${PREFIX}ref.root -t $CFG -r $REF
$JUDITH -c applyMask -i $RAWFILE -o ${PREFIX}ref.root -t $CFG -r $REF
$JUDITH -c process -i ${PREFIX}ref.root -o ${PREFIX}ref-p.root -t $CFG -r $REF
$JUDITH -c coarseAlign -i ${PREFIX}ref-p.root -t $CFG -r $REF
$JUDITH -c fineAlign -i ${PREFIX}ref-p.root -t $CFG -r $REF
$JUDITH -c process -i ${PREFIX}ref.root -o ${PREFIX}ref-pa.root -t $CFG -r $REF
$JUDITH -c analysis -i ${PREFIX}ref-pa.root -R ${PREFIX}ref-pa-analysis.root -t $CFG -r $REF
