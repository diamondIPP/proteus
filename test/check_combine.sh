#!/bin/sh
#
# check pt-combine functionality

set -ex

flags=$@ # e.g. --no-progress for ci-jobs

(
  cd unigetel_dummy
  pt-combine ${flags} -n 10000 \
    ../output/merged0.root \
    ../data/unigetel_dummy/ebeam012_nparticles01.root \
    ../data/unigetel_dummy/ebeam012_nparticles01.root
)

./root-checker output/merged0.root <<__END_OF_CONFIG__
  Event        entries 10000
  Plane0/Hits  entries 10000
  Plane1/Hits  entries 10000
  Plane2/Hits  entries 10000
  Plane3/Hits  entries 10000
  Plane4/Hits  entries 10000
  Plane5/Hits  entries 10000
  Plane6/Hits  entries 10000
  Plane7/Hits  entries 10000
  Plane8/Hits  entries 10000
  Plane9/Hits  entries 10000
  Plane10/Hits entries 10000
  Plane11/Hits entries 10000
  Plane12/Hits entries 10000
  Plane13/Hits entries 10000
__END_OF_CONFIG__

(
  cd unigetel_dummy
  pt-combine ${flags} -n 10000 \
    ../output/merged1.root \
    ../data/unigetel_dummy/ebeam120_nparticles02.root \
    ../data/unigetel_dummy/ebeam120_nparticles02.root
)

./root-checker output/merged1.root <<__END_OF_CONFIG__
  Event        entries 10000
  Plane0/Hits  entries 10000
  Plane1/Hits  entries 10000
  Plane2/Hits  entries 10000
  Plane3/Hits  entries 10000
  Plane4/Hits  entries 10000
  Plane5/Hits  entries 10000
  Plane6/Hits  entries 10000
  Plane7/Hits  entries 10000
  Plane8/Hits  entries 10000
  Plane9/Hits  entries 10000
  Plane10/Hits entries 10000
  Plane11/Hits entries 10000
  Plane12/Hits entries 10000
  Plane13/Hits entries 10000
__END_OF_CONFIG__
