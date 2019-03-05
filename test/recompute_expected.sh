#!/bin/sh
#
# run examples for all configured datasets and recompute the expected output
# that can be checked later on, e.g. in the ci system.

datasets="
  unigetel_ebeam012_nparticles01
  unigetel_ebeam120_nparticles01
  unigetel_ebeam120_nparticles01_xygamma
  unigetel_ebeam120_nparticles02
"

for dataset in ${datasets}; do
  ./run_recon.sh ${dataset}
  ./root-checker --generate \
    output/${dataset}/recon-hists.root \
    expected/${dataset}-recon.txt
done
