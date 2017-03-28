project=iqrfapp

#expected build dir structure
buildexp=build/Unix_Makefiles

currentdir=$PWD
builddir=./${buildexp}
mkdir -p ${builddir}

#get path to iqrfd libs
iqrfd=../../daemon/${buildexp}
pushd ${iqrfd}
iqrfd=${PWD}
popd

#get path to cutils libs
cutils=../../lib/cutils/${buildexp}
pushd ${cutils}
cutils=${PWD}
popd

#launch cmake to generate build environment
pushd ${builddir}
pwd
cmake -G "Unix Makefiles" -Dcutils_DIR:PATH=${cutils} -Diqrfd_DIR:PATH=${iqrfd} ${currentdir}
popd

#build from generated build environment
cmake --build ${builddir}
