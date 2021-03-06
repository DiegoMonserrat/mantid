setlocal enableextensions enabledelayedexpansion
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WINDOWS SCRIPT TO DRIVE THE JENKINS BUILDS OF MANTID.
::
:: Notes:
::
:: WORKSPACE & JOB_NAME are environment variables that are set by Jenkins.
:: BUILD_THREADS & PARAVIEW_DIR should be set in the configuration of each slave.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: All node currently have PARAVIEW_DIR=3.98.1 and PARAVIEW_NEXT_DIR=4.3.1
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PARAVIEW_DIR=%PARAVIEW_NEXT_DIR%

set CMAKE_BIN_DIR=C:\Program Files (x86)\CMake 2.8\bin
"%CMAKE_BIN_DIR%\cmake.exe" --version
echo %sha1%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Get or update the third party dependencies
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cd %WORKSPACE%\Code
call fetch_Third_Party win64
cd %WORKSPACE%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Set up the location for local object store outside of the build and source
:: tree, which can be shared by multiple builds.
:: It defaults to a MantidExternalData directory within the USERPROFILE
:: directory. It can be overridden by setting the MANTID_DATA_STORE environment
:: variable.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if NOT DEFINED MANTID_DATA_STORE (
  set MANTID_DATA_STORE=%USERPROFILE%\MantidExternalData
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check job requirements from the name
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set CLEANBUILD=
set BUILDPKG=
if not "%JOB_NAME%" == "%JOB_NAME:clean=%" (
  set CLEANBUILD=yes
  set BUILDPKG=yes
)

if not "%JOB_NAME%" == "%JOB_NAME:pull_requests=%" (
  set BUILDPKG=yes
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Setup the build directory
:: For a clean build the entire thing is removed to guarantee it is clean. All
:: other build types are assumed to be incremental and the following items
:: are removed to ensure stale build objects don't interfere with each other:
::   - build/bin: if libraries are removed from cmake they are not deleted
::                   from bin and can cause random failures
::   - build/ExternalData/**: data files will change over time and removing
::                            the links helps keep it fresh
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set BUILD_DIR=%WORKSPACE%\build
if "%CLEANBUILD%" == "yes" (
  rmdir /S /Q %BUILD_DIR%
)

if EXIST %BUILD_DIR% (
  rmdir /S /Q %BUILD_DIR%\bin %BUILD_DIR%\ExternalData
) else (
  md %BUILD_DIR%
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Packaging options
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PACKAGE_DOCS=
if "%BUILDPKG%" == "yes" (
  set PACKAGE_DOCS=-DPACKAGE_DOCS=ON
)

cd %BUILD_DIR%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Clean up any artifacts from last build so that if it fails
:: they don't get archived again.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
del /Q *.exe

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check the required build configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set BUILD_CONFIG=
if not "%JOB_NAME%"=="%JOB_NAME:debug=%" (
  set BUILD_CONFIG=Debug
) else (
if not "%JOB_NAME%"=="%JOB_NAME:relwithdbg=%" (
  set BUILD_CONFIG=RelWithDbg
) else (
    set BUILD_CONFIG=Release
    ))
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Update the PATH so that we can find everything
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PATH=%WORKSPACE%\Code\Third_Party\lib\win64;%WORKSPACE%\Code\Third_Party\lib\win64\Python27;%WORKSPACE%\Code\Third_Party\lib\win64\mingw;%PARAVIEW_DIR%\bin\%BUILD_CONFIG%;%PATH%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
"%CMAKE_BIN_DIR%\cmake.exe" -G "Visual Studio 11 Win64" -DCONSOLE=OFF -DENABLE_CPACK=ON -DMAKE_VATES=ON -DParaView_DIR=%PARAVIEW_DIR% -DMANTID_DATA_STORE=!MANTID_DATA_STORE! -DUSE_PRECOMPILED_HEADERS=ON %PACKAGE_DOCS% ..\Code\Mantid
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
msbuild /nologo /m:%BUILD_THREADS% /nr:false /p:Configuration=%BUILD_CONFIG% Mantid.sln
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Remove the user properties file just in case anything polluted it
set USERPROPS=bin\%BUILD_CONFIG%\Mantid.user.properties
del %USERPROPS%
"%CMAKE_BIN_DIR%\ctest.exe" -C %BUILD_CONFIG% -j%BUILD_THREADS% --schedule-random --output-on-failure
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the install kit if required
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%BUILDPKG%" == "yes" (
  echo Building package
  :: Build offline documentation
  msbuild /nologo /nr:false /p:Configuration=%BUILD_CONFIG% docs/docs-qthelp.vcxproj

  :: Ignore errors as the exit code of msbuild is wrong here.
  :: It always marks the build as a failure even thought the MantidPlot exit
  :: code is correct!
  ::if ERRORLEVEL 1 exit /B %ERRORLEVEL%
  "%CMAKE_BIN_DIR%\cpack.exe" -C %BUILD_CONFIG% --config CPackConfig.cmake
)
