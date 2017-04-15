set project=iqrfapp

rem //expected build dir structure
set buildexp=build\\Visual_Studio_14_2015\\x86

set currentdir=%cd%
set builddir=.\\%buildexp%

mkdir %builddir%

rem //get path to iqrfd libs
set iqrfd=..\\..\\daemon\\%buildexp%
pushd %iqrfd%
set iqrfd=%cd%
popd

rem //get path to cutils libs
set cutils=..\\..\\..\\cutils\\%buildexp%
pushd %cutils%
set cutils=%cd%
popd

rem //launch cmake to generate build environment
pushd %builddir%
cmake -G "Visual Studio 14 2015" -Dcutils_DIR:PATH=%cutils% -Diqrfd_DIR:PATH=%iqrfd% %currentdir%
popd

rem //build from generated build environment
cmake --build %builddir%
