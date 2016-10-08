set project=clibudp

rem //expected build dir structure
set buildexp=build\\Visual_Studio_12_2013\\x64

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

rem //launch cmake to generate build environment
pushd %builddir%
cmake -G "Visual Studio 12 2013 Win64" -Dclibcdc_DIR:PATH=%clibcdc% -Dclibspi_DIR:PATH=%clibspi% %currentdir%
popd

rem //build from generated build environment
cmake --build %builddir%
