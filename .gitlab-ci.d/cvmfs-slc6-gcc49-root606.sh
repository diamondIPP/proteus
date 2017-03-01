# setup CVMFS-based environment w/ gcc 4.9 and ROOT 6.06

source /cvmfs/sft.cern.ch/lcg/contrib/gcc/4.9.3/x86_64-slc6/setup.sh
pushd .
cd  /cvmfs/sft.cern.ch/lcg/app/releases/ROOT/6.06.08/x86_64-slc6-gcc49-opt/root
source bin/thisroot.sh
popd
export PATH="/cvmfs/sft.cern.ch/lcg/contrib/CMake/3.2.3/Linux-x86_64/bin:${PATH}"
