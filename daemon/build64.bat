set project=clibudp

rem //expected build dir structure
set buildexp=build\\Visual_Studio_14_2015\\x64

set currentdir=%cd%
set builddir=.\\%buildexp%
set libsdir=..\\..

mkdir %builddir%

rem //get path to clibcdc libs
set clibcdc=%libsdir%\\clibcdc\\%buildexp%
pushd %clibcdc%
set clibcdc=%cd%
popd

rem //get path to clibspi libs
set clibspi=%libsdir%\\clibspi\\%buildexp%
pushd %clibspi%
set clibspi=%cd%
popd

rem //get path to clibdpa libs
set clibdpa=%libsdir%\\clibdpa\\%buildexp%
pushd %clibdpa%
set clibdpa=%cd%
popd

rem //get path to cutils libs
set cutils=%libsdir%\\cutils\\%buildexp%
pushd %cutils%
set cutils=%cd%
popd

set ver=v1.0.0
set tms="%date% %time%"

rem //launch cmake to generate build environment
pushd %builddir%
cmake -G "Visual Studio 14 2015 Win64" -Dclibdpa_DIR:PATH=%clibdpa% -Dcutils_DIR:PATH=%cutils% -Dclibcdc_DIR:PATH=%clibcdc% -Dclibspi_DIR:PATH=%clibspi% -DDAEMON_VERSION:STRING=%ver% -DBUILD_TIMESTAMP:STRING=%tms% %currentdir%
popd

rem //build from generated build environment
cmake --build %builddir%
