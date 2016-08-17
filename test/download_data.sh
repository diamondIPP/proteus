#!/bin/sh -ex
#
# get copies of the raw data files

get()
{
    source=https://unigetb.web.cern.ch/unigetb/tbdata/raw/$1
    target=$2

    curl -o ${target} ${source}
}

get cosmic_85_0875.root raw/run000875.root
get cosmic_001066.root raw/run001066.root
(cd raw; sha256sum --check sha256sum.txt)
