#!/bin/sh
#
# find the LCG platform for this host
#
# sets the `lcg_version`, `lcg_platform`, and `lcg_view` variables

lcg_version=LCG_91
if [ "$(cat /etc/redhat-release | grep 'Scientific Linux CERN SLC release 6')" ]; then
  lcg_platform="x86_64-slc6-gcc7-opt"
elif [ "$(cat /etc/centos-release | grep 'CentOS Linux release 7')" ]; then
  lcg_platform="x86_64-centos7-gcc7-opt"
else
  echo "Unknown OS" 1>&2
  exit 1
fi
lcg_view=/cvmfs/sft.cern.ch/lcg/views/${lcg_version}/${lcg_platform}
