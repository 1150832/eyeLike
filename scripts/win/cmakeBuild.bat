@echo off
echo ===================================================
echo  BUILD EYELIKE - WINDOWS (MSVC)
echo ===================================================

:: Volta para a raiz do projeto caso o script seja corrido de dentro da pasta scripts
cd %~dp0\..\..

if not exist build mkdir build
cd build

echo.
echo [1/2] Gerando a solucao CMake para Visual Studio 2022...
cmake -G "Visual Studio 18 2026" -T v143 -A x64 -D OpenCV_DIR="C:/opencv/build" -D CMAKE_TOOLCHAIN_FILE="C:\Users\rmarques\vcpkg\scripts\buildsystems\vcpkg.cmake" ..
echo.
echo [2/2] Compilando o executavel (Release)...
cmake --build . --config Release

echo.
echo ===================================================
echo  BUILD COMPLETE!
echo ===================================================
pause