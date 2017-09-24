#!/bin/bash
# Script for building IQRF daemon on Linux machine

set -e
project=daemon

#expected build dir structure
buildexp=build/Unix_Makefiles

LIB_DIRECTORY=${1:-../..}
currentdir=$PWD
builddir=./${buildexp}

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

#get path to clibcdc libs
clibcdc=${LIB_DIRECTORY}/clibcdc/${buildexp}
pushd ${clibcdc}
clibcdc=$PWD
popd

#get path to clibspi libs
clibspi=${LIB_DIRECTORY}/clibspi/${buildexp}
pushd ${clibspi}
clibspi=$PWD
popd

#get path to clibdpa libs
clibdpa=${LIB_DIRECTORY}/clibdpa/${buildexp}
pushd ${clibdpa}
clibdpa=$PWD
popd

#get path to cutils libs
cutils=${LIB_DIRECTORY}/cutils/${buildexp}
pushd ${cutils}
cutils=$PWD
popd

#launch cmake to generate build environment
pushd ${builddir}
pwd
cmake -G "Unix Makefiles" -Dclibcdc_DIR:PATH=${clibcdc} -Dclibspi_DIR:PATH=${clibspi} -Dclibdpa_DIR:PATH=${clibdpa} -Dcutils_DIR:PATH=${cutils} ${currentdir} ${debug}
popd

#build from generated build environment
cmake --build ${builddir}
