#!/bin/bash -ex
#
# run the processing chain for the reference telescope

RUN=${1-1066}
CFG=configs/global.cfg
REF=configs/device-ref.toml
# FLAGS="--numEvents 10000 --printLevel 1"
# FLAGS="--numEvents 100000"
FLAGS=

RAWFILE=$(readlink -f $(printf "raw/run%06d.root" $RUN))
RUNDIR=$(printf "run%06d" $RUN)
PREFIX=$(printf "output/run%06d-" $RUN)

source build/activate.sh
cd $RUNDIR

cp -f geometry/unaligned.toml alignment-ref.toml
mkdir -p output

# assume noise-free telescope; no noise mask
proteus $FLAGS -c applyMask -i $RAWFILE -o ${PREFIX}ref.root -t $CFG -r $REF
proteus $FLAGS -c process -i ${PREFIX}ref.root -o ${PREFIX}ref-unaligned.root -t $CFG -r $REF -R ${PREFIX}ref-unaligned-hists.root
proteus $FLAGS -c coarseAlign -i ${PREFIX}ref-unaligned.root -t $CFG -r $REF
cp alignment-ref.toml alignment-ref-coarse.toml
# TODO 2016-09-16 msmk: fine alignment does not work w/ roi cut on data
# proteus $FLAGS -c fineAlign -i ${PREFIX}ref-unaligned.root -t $CFG -r $REF
# cp alignment-ref.dat alignment-ref-fineAlign.dat
proteus $FLAGS -c process -i ${PREFIX}ref.root -o ${PREFIX}ref-aligned.root -t $CFG -r $REF -R ${PREFIX}ref-aligned-hists.root
