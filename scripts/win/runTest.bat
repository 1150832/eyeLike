@echo off
echo ===================================================
echo  INICIANDO NODE DE RASTREIO (WINDOWS)
echo ===================================================

cd %~dp0\..\..

if exist build\Release\eyelike.exe (
    echo A iniciar eyelike.exe...
    .\build\Release\eyelike.exe
) else (
    echo [ERRO FATAL] O executavel nao foi encontrado.
    echo Por favor, corre o script cmakeBuild.bat primeiro.
)

pause