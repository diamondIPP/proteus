#!/bin/sh -ex
#
# all test scripts for a run

./run_tel_noisescan.sh $@
./run_tel_align.sh $@
./run_dut0_noisescan.sh $@
./run_dut_align.sh $@
./run_track.sh $@
./run_match.sh $@
