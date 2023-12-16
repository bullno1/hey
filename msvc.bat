@echo off

mkdir msvc
cd msvc
cmake ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -D CMAKE_SYSTEM_VERSION=10.0.22000.0 ^
    ..