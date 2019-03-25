#!/bin/sh
#
# run examples for all configured datasets and recompute the expected output
# that can be checked later on, e.g. in the ci system.

paths="
unigetel_dummy/ebeam005_positron_nparticles01_inc
unigetel_dummy/ebeam005_positron_nparticles01_upr
unigetel_dummy/ebeam120_pionp_nparticles01_inc
unigetel_dummy/ebeam120_pionp_nparticles01_upr
unigetel_dummy/ebeam180_pionp_nparticles01_inc
unigetel_dummy/ebeam180_pionp_nparticles01_inc_xygamma
unigetel_dummy/ebeam180_pionp_nparticles01_upr
"

IFS='
'
for path in ${paths}; do
  ./run_recon.sh ${path} --no-progress
  ./root-checker --generate \
    output/${path}/recon-hists.root \
    ${path}/expected-recon.txt
done
