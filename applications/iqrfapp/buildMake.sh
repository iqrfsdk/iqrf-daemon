#!/bin/bash
# Script for building IQRF application on Linux machine

set -e
project=iqrfapp

#expected build dir structure
buildexp=build/Unix_Makefiles

currentdir=$PWD
builddir=./${buildexp}
LIB_DIRECTORY=${1:-../../..}

mkdir -p ${builddir}

#debug
if [ ! -z $2 ]
then
# user selected
        if [ $2 == "Debug" ]
        then
                debug=-DCMAKE_BUILD_TYPE=$2
        else
                debug=-DCMAKE_BUILD_TYPE="Debug"
        fi
else
# release by default
        debug=""
fi
echo ${debug}

#get path to iqrfd libs
iqrfd=../../daemon/${buildexp}
pushd ${iqrfd}
iqrfd=${PWD}
popd

#get path to cutils libs
cutils=${LIB_DIRECTORY}/cutils/${buildexp}
pushd ${cutils}
cutils=${PWD}
popd

#launch cmake to generate build environment
pushd ${builddir}
pwd
cmake -G "Unix Makefiles" -Dcutils_DIR:PATH=${cutils} -Diqrfd_DIR:PATH=${iqrfd} ${currentdir} ${debug}
popd

#build from generated build environment
cmake --build ${builddir}
