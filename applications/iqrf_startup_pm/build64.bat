set project=iqrf_startup_pm

rem //expected build dir structure
set buildexp=build\\Visual_Studio_14_2015\\x64

set currentdir=%cd%
set builddir=.\\%buildexp%

mkdir %builddir%

rem //get path to clibcdc libs
set clibcdc=..\\..\\libs\\clibcdc\\%buildexp%
pushd %clibcdc%
set clibcdc=%cd%
popd

rem //get path to clibspi libs
set clibspi=..\\..\\libs\\clibspi\\%buildexp%
pushd %clibspi%
set clibspi=%cd%
popd

rem //get path to clibdpa libs
set clibdpa=..\\..\\libs\\clibdpa\\%buildexp%
pushd %clibdpa%
set clibdpa=%cd%
popd

rem //get path to cutils libs
set cutils=..\\..\\libs\\cutils\\%buildexp%
pushd %cutils%
set cutils=%cd%
popd

rem //get path to iqrfd libs
set iqrfd=..\\..\\daemon\\%buildexp%
pushd %iqrfd%
set iqrfd=%cd%
popd

rem //launch cmake to generate build environment
pushd %builddir%
cmake -G "Visual Studio 14 2015 Win64" -Dclibdpa_DIR:PATH=%clibdpa% -Dcutils_DIR:PATH=%cutils% -Dclibcdc_DIR:PATH=%clibcdc% -Dclibspi_DIR:PATH=%clibspi% -Diqrfd_DIR:PATH=%iqrfd% %currentdir%
popd

rem //build from generated build environment
cmake --build %builddir%
