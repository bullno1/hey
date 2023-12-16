@echo off

PATH=%PATH%;%CD%\cmake

for /f "usebackq tokens=*" %%i in (`vswhere -latest -prerelease -property installationPath`) do (
  set VS_DIR=%%i
)

call "%VS_DIR%\VC\Auxiliary\Build\vcvarsall.bat" x64 10.0.22000.0

mkdir .build
cd .build
cmake ^
    -G "Ninja" ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
    -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
    ..
cmake --build .