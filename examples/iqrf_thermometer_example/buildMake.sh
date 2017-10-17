#!/bin/bash
# Script for building IQRF startup application on Linux machine

set -e

project=iqrf_thermometer_example

#expected build dir structure
buildexp=build/Unix_Makefiles

currentdir=$PWD
builddir=./${buildexp}
LIB_DIRECTORY=${1:-../../../}

mkdir -p ${builddir}

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

#get path to iqrfd libs
iqrfd=../../daemon/${buildexp}
pushd ${iqrfd}
iqrfd=${PWD}
popd

#launch cmake to generate build environment
pushd ${builddir}
pwd
cmake -G "Unix Makefiles" -Dclibcdc_DIR:PATH=${clibcdc} -Dclibspi_DIR:PATH=${clibspi} -Dclibdpa_DIR:PATH=${clibdpa} -Dcutils_DIR:PATH=${cutils} -Diqrfd_DIR:PATH=${iqrfd} ${currentdir} -DCMAKE_BUILD_TYPE=Debug
popd

#build from generated build environment
cmake --build ${builddir}
