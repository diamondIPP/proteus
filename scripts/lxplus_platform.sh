#!/bin/sh
#
# find the lxplus LCG platform
#
# sets the `platform`, `lcg_version`, and `lcg_view` variables

if [ "$(cat /etc/redhat-release | grep 'Scientific Linux CERN SLC release 6')" ]; then
  platform="x86_64-slc6-gcc62-opt"
elif [ "$(cat /etc/centos-release | grep 'CentOS Linux release 7')" ]; then
  platform="x86_64-centos7-gcc62-opt"
else
  echo "Unknown OS" 1>&2
  exit 1
fi

lcg_version=LCG_88
lcg_view=/cvmfs/sft.cern.ch/lcg/views/${lcg_version}/${platform}
