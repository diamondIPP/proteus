#!/bin/sh
#
# run examples for all configured datasets and recompute the expected output
# that can be checked later on, e.g. in the ci system.

datasets="
unigetel_dummy/ebeam012_nparticles01
unigetel_dummy/ebeam120_nparticles01
unigetel_dummy/ebeam120_nparticles01_xygamma
unigetel_dummy/ebeam120_nparticles02
"

IFS='
'
for path in ${datasets}; do
  setup=$(echo ${path} | cut -f1 -d/)
  dataset=$(echo ${path} | cut -f2 -d/)
  ./run_recon.sh ${setup} ${dataset} --no-progress
  ./root-checker --generate \
    output/${path}/recon-hists.root \
    ${path}/expected-recon.txt
done
