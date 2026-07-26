@echo off
setlocal enabledelayedexpansion

:: Navega para a raiz do projeto (assumindo que o script esta em scripts\win\)
cd %~dp0\..\..

if "%~1"=="" (
    echo === Summary of Results ^(testEyeTracking^) ===
    echo Uso: %0 ^<video_file^>
    exit /b 1
)

set "VIDEO_PATH=%~1"
set "DIRNAME=%~dp1"
set "FILENAME=%~n1"

set "GROUND_TRUTH=%DIRNAME%ground_truth_%FILENAME%.json"
set "PREDICTIONS=%DIRNAME%predictions_%FILENAME%.csv"

if not exist "%GROUND_TRUTH%" (
    echo [ERRO] Falta o ficheiro .json para %FILENAME%!
    echo Corre o run_complete_test.bat primeiro.
    exit /b 1
)
if not exist "%PREDICTIONS%" (
    echo [ERRO] Falta o ficheiro .csv para %FILENAME%!
    echo Corre o run_complete_test.bat primeiro.
    exit /b 1
)

:: Garante que a nova pasta de relatorios existe na raiz
if not exist "test_reports" mkdir "test_reports"

:: Invoca o motor de testes e passa os 3 argumentos necessarios
.\build\Release\testEyeTracking.exe "%VIDEO_PATH%" "%GROUND_TRUTH%" "%PREDICTIONS%"