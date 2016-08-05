#!/bin/sh -ex
#
# get a copies of the raw data files

GET="rsync -ah --progress"

mkdir -p raw
$GET \
    lxplus.cern.ch:/afs/cern.ch/work/f/fdibello/public/JTraining/cosmic_85_0875.root \
    raw/cosmic_000875.root
$GET \
    caribou@pccariboudaq:/data/rcedata/cern_sps_2016_07/raw/cosmic_001066/cosmic_001066.root \
    raw/cosmic_001066.root
