#!/bin/sh
#
# run full reconstruction chain

set -e

./run_chain_noisescan.sh $@
./run_chain_align_tel.sh $@
./run_chain_align_dut.sh $@
./run_chain_recon.sh $@
