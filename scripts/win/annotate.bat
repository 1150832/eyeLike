@echo off
setlocal

:: Navega para a raiz do projeto (uma pasta acima de onde o script esta)
cd %~dp0\..\..

:: Verifica se o argumento do video foi fornecido
if "%~1"=="" (
    echo === Annotation Tool ===
    echo Error: Missing arguments.
    echo Use: %0 ^<video_file^>
    echo Example: %0 testing\test_data\test1.mov
    exit /b 1
)

set "VIDEO_PATH=%~1"

:: Verifica se o ficheiro de video realmente existe
if not exist "%VIDEO_PATH%" (
    echo [ERRO] O video '%VIDEO_PATH%' nao foi encontrado!
    exit /b 1
)

:: Executa a ferramenta de anotacao
if exist "build\Release\annotationTool.exe" (
    .\build\Release\annotationTool.exe "%VIDEO_PATH%"
) else (
    echo [ERRO FATAL] O executavel annotationTool.exe nao foi encontrado.
    echo Por favor, compila o projeto primeiro usando scripts\cmakeBuild.bat
    exit /b 1
)

endlocal