set project=clibudp

rem //expected build dir structure
set buildexp=build\\Visual_Studio_14_2015\\x64

set currentdir=%cd%
set builddir=.\\%buildexp%

mkdir %builddir%

rem //get path to clibcdc libs
set clibcdc=..\\..\\clibcdc\\%buildexp%
pushd %clibcdc%
set clibcdc=%cd%
popd

rem //get path to clibspi libs
set clibspi=..\\..\\clibspi\\%buildexp%
pushd %clibspi%
set clibspi=%cd%
popd

rem //get path to clibdpa libs
set clibdpa=..\\..\\clibdpa\\%buildexp%
pushd %clibdpa%
set clibdpa=%cd%
popd

rem //get path to cutils libs
set cutils=..\\..\\cutils\\%buildexp%
pushd %cutils%
set cutils=%cd%
popd

rem //launch cmake to generate build environment
pushd %builddir%
cmake -G "Visual Studio 14 2015 Win64" -Dclibdpa_DIR:PATH=%clibdpa% -Dcutils_DIR:PATH=%cutils% -Dclibcdc_DIR:PATH=%clibcdc% -Dclibspi_DIR:PATH=%clibspi% %currentdir%
popd

rem //build from generated build environment
cmake --build %builddir%
