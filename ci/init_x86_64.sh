#!/bin/bash 

#######################################################
# Init file for gitlab-ci using a slc6 docker image   #
# Original by CLICdp (https://gitlab.cern.ch/CLICdp/) #
#######################################################

if [ "$( cat /etc/*-release | grep Scientific )" ]; then
    OS=slc6
elif [ "$( cat /etc/*-release | grep CentOS )" ]; then
    OS=centos7
else
    echo "UNKNOWN OS"
    exit 1
fi

if [ -z ${GCC_VERSION} -o -z ${ROOT_VERSION} ]; then
   echo "You have to provide GCC_VERSION(e.g. 4.8.5) and ROOT_VERSION(e.g. 5.34.36)"
   exit 1
fi

if [ -z ${BUILD_TYPE} ]; then
    BUILD_TYPE=opt
fi

GCC_VER=`echo ${GCC_VERSION} | sed -e 's/\.//g' | cut -c 1-2`

# General variables
CLICREPO=/cvmfs/clicdp.cern.ch
CERNREPO=/cvmfs/sft.cern.ch

GCCREPOS="$CERNREPO/lcg/external/gcc $CLICREPO/compilers/gcc"
ROOTREPOS="$CERNREPO/lcg/app/releases/ROOT $CLICREPO/software/ROOT"

BUILD_FLAVOUR=x86_64-${OS}-gcc${GCC_VER}-${BUILD_TYPE}


#--------------------------------------------------------------------------------
#     GCC
#--------------------------------------------------------------------------------

FOUND=0
for repo in $GCCREPOS; do
    GCCSOURCE=$repo/${GCC_VERSION}/x86_64-${OS}
    if [ -d $GCCSOURCE ]; then
        source $GCCSOURCE/setup.sh
        echo "GCC found in $GCCSOURCE"
        FOUND=1
        break
    fi
done
if [ $FOUND -eq 0 ]; then
    echo "GCC version not found in $GCCREPOS"
    exit 1
fi

#--------------------------------------------------------------------------------
#     CMake
#--------------------------------------------------------------------------------

export CMAKE_HOME=${CLICREPO}/software/CMake/3.5.2/${BUILD_FLAVOUR}
export PATH=${CMAKE_HOME}/bin:$PATH

#--------------------------------------------------------------------------------
#     Python
#--------------------------------------------------------------------------------

export PYTHONDIR=${CLICREPO}/software/Python/2.7.12/${BUILD_FLAVOUR}
export PATH=$PYTHONDIR/bin:$PATH
export LD_LIBRARY_PATH=$PYTHONDIR/lib:$LD_LIBRARY_PATH

#--------------------------------------------------------------------------------
#     ROOT
#--------------------------------------------------------------------------------

FOUND=0
for repo in $ROOTREPOS; do
    rootpath=$repo/$ROOT_VERSION/$BUILD_FLAVOUR
    if [ -d $rootpath ]; then
        source $rootpath/root/bin/thisroot.sh
        echo "ROOT found in $rootpath"
        FOUND=1
        break
    fi
done
if [ $FOUND -eq 0 ]; then
    echo "ROOT version not found in $GCCREPOS"
    exit 1
fi

#--------------------------------------------------------------------------------
#     Boost
#--------------------------------------------------------------------------------

export BOOST_ROOT=${CLICREPO}/software/Boost/1.61.0/${BUILD_FLAVOUR}
export LD_LIBRARY_PATH="${BOOST_ROOT}/lib:$LD_LIBRARY_PATH"

