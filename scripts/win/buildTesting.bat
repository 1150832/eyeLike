@echo off
setlocal enabledelayedexpansion

:: Navega para a raiz do projeto
cd %~dp0\..\..

echo ===================================================
echo  Building Eye Tracking Testing Tools (Windows)
echo ===================================================
echo.

:: 1. Validar tamanho dos ficheiros (Substituto nativo para wc -c)
set ANNO_SIZE=0
if exist "testing\src\annotationTool.cpp" (
    for %%I in ("testing\src\annotationTool.cpp") do set ANNO_SIZE=%%~zI
)

set TEST_SIZE=0
if exist "testing\src\testEyeTracking.cpp" (
    for %%I in ("testing\src\testEyeTracking.cpp") do set TEST_SIZE=%%~zI
)

if %ANNO_SIZE% LSS 1000 (
    echo [ERRO] annotationTool.cpp e demasiado pequeno ^(%ANNO_SIZE% bytes^)
    echo Caminho verificado: testing\src\annotationTool.cpp
    echo Por favor, garante que o ficheiro existe e tem conteudo.
    exit /b 1
)

if %TEST_SIZE% LSS 1000 (
    echo [ERRO] testEyeTracking.cpp e demasiado pequeno ^(%TEST_SIZE% bytes^)
    echo Caminho verificado: testing\src\testEyeTracking.cpp
    echo Por favor, garante que o ficheiro existe e tem conteudo.
    exit /b 1
)

echo [OK] Ficheiros fonte validados.
echo.

:: 2. Configurar o diretorio build
if not exist build mkdir build
cd build

echo A correr a configuracao do CMake a partir da raiz...
cmake -G "Visual Studio 18 2026" -T v143 -A x64 -D OpenCV_DIR="C:/opencv/build" -D CMAKE_TOOLCHAIN_FILE="C:\Users\rmarques\vcpkg\scripts\buildsystems\vcpkg.cmake" ..
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERRO] A configuracao do CMake falhou.
    exit /b 1
)

:: 3. Compilar especificamente os alvos de teste (Substituto do make -j4)
echo.
echo A compilar as Testing Tools...

cmake --build . --config Release --target annotationTool
if %ERRORLEVEL% NEQ 0 goto build_fail

cmake --build . --config Release --target testEyeTracking
if %ERRORLEVEL% NEQ 0 goto build_fail

echo.
echo ===================================================
echo   [OK] Compilacao Concluida com Sucesso! 
echo ===================================================
echo.
echo Executaveis criados na pasta:
echo   %CD%\Release\annotationTool.exe
echo   %CD%\Release\testEyeTracking.exe
echo.
goto end

:build_fail
echo.
echo [ERRO] A compilacao falhou.
exit /b 1

:end
cd ..
endlocal