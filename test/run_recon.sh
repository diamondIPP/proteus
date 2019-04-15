#!/bin/sh
#
# run tracking and dut matching with correct geometry

set -ex

. ./run_common.sh

pt-recon ${flags} -g ${datasetdir}/geometry.toml \
  ${data} ${output_prefix}recon

if test -e ${datasetdir}/expected-recon.txt; then
  ./root-checker ${output_prefix}recon-hists.root ${datasetdir}/expected-recon.txt
fi
