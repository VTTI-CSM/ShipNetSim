@echo off
SETLOCAL EnableDelayedExpansion

REM Clean up the build directory
if exist build rmdir /s /q build

REM Create a new build directory and enter it
mkdir build
cd build

REM Run CMake to configure the project
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.4.2/msvc2019_64/lib/cmake"

REM Build the project with CMake
cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS%

REM Package the project with CPack and the IFW generator
cpack -G IFW -j %NUMBER_OF_PROCESSORS%

cd ..

ENDLOCAL
