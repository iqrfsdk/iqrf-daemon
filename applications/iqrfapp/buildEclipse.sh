#!/bin/bash
# Script for building IQRF application on Linux machine

set -e

project=iqrfapp

#expected build dir structure
buildexp=build/Eclipse_CDT4-Unix_Makefiles

currentdir=$PWD
builddir=./${buildexp}
LIB_DIRECTORY=${1:-../../..}

mkdir -p ${builddir}

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
cmake -G "Eclipse CDT4 - Unix Makefiles" -Dcutils_DIR:PATH=${cutils} -Diqrfd_DIR:PATH=${iqrfd} ${currentdir} -DCMAKE_BUILD_TYPE=Debug
popd

#build from generated build environment
cmake --build ${builddir}
