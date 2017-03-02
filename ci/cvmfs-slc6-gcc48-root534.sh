# setup CVMFS-based environment w/ gcc 4.8 and ROOT 5.34

source /cvmfs/sft.cern.ch/lcg/contrib/gcc/4.8.4/x86_64-slc6/setup.sh
pushd .
cd  /cvmfs/sft.cern.ch/lcg/app/releases/ROOT/5.34.36/x86_64-slc6-gcc48-opt/root
source bin/thisroot.sh
popd
export PATH="/cvmfs/sft.cern.ch/lcg/contrib/CMake/3.2.3/Linux-x86_64/bin:${PATH}"
