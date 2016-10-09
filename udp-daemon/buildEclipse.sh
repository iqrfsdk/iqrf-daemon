project=clibudp

#expected build dir structure
buildexp=build/Eclipse_CDT4-Unix_Makefiles

currentdir=$PWD
builddir=./${buildexp}

mkdir -p ${builddir}

#get path to clibcdc libs
clibcdc=../../clibcdc/${buildexp}
pushd ${clibcdc}
clibcdc=$PWD
popd

#get path to clibspi libs
clibspi=../../clibspi/${buildexp}
pushd ${clibspi}
clibspi=$PWD
popd

#launch cmake to generate build environment
pushd ${builddir}
pwd
cmake -G "Eclipse CDT4 - Unix Makefiles" -Dclibcdc_DIR:PATH=${clibcdc} -Dclibspi_DIR:PATH=${clibspi} ${currentdir}
popd

#build from generated build environment
cmake --build ${builddir}

