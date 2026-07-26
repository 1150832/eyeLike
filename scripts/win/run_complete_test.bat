@echo off
setlocal enabledelayedexpansion

:: Navega para a raiz do projeto
cd %~dp0\..\..

:: Verifica se foram passados argumentos
if "%~1"=="" goto print_usage

set BATCH_MODE=false

:: ==========================================
:: MODO BATCH (--all)
:: ==========================================
if "%~1"=="--all" (
    set BATCH_MODE=true
    echo Iniciando modo BATCH TESTING...
    
    :: Trava de seguranca para evitar execucoes cegas
    if not exist "testing\test_data\video" (
        echo [ERRO FATAL] A pasta testing\test_data\video nao foi encontrada na raiz do projeto!
        goto end
    )
    
    echo A procurar videos recursivamente em testing\test_data\video\...
    
    set success_count=0
    set total_count=0
    
    :: Itera sobre os formatos suportados de forma RECURSIVA (/R) na pasta correta
    for /R "testing\test_data\video" %%V in (*.mov *.mp4 *.avi *.mkv) do (
        if exist "%%V" (
            set "DIRNAME=%%~dpV"
            set "FILENAME=%%~nV"
            set "GT_CHECK=%%~dpVground_truth_%%~nV.json"
            
            if exist "!GT_CHECK!" (
                set /a total_count+=1
                call :process_video "%%V"
                if !ERRORLEVEL! EQU 0 (
                    set /a success_count+=1
                )
            ) else (
                echo Ignorando %%V ^(Anotacoes nao encontradas^)
            )
        )
    )
    
    echo ===================================================
    echo BATCH TESTING CONCLUIDO
    echo Processados com sucesso: !success_count! / !total_count! videos anotados
    echo Todos os relatorios estao em test_reports\
    goto end
) else (
:: ==========================================
:: MODO SINGLE VIDEO
:: ==========================================
    set "VIDEO=%~1"
    if not exist "!VIDEO!" (
        echo [ERRO] O ficheiro de video nao foi encontrado: !VIDEO!
        exit /b 1
    )
    
    call :process_video "!VIDEO!"
    
    echo.
    echo ===================================================
    echo  Testes Concluidos!
    echo ===================================================
    goto end
)


:: ==========================================
:: FUNCAO DE UTILIZACAO
:: ==========================================
:print_usage
echo ===================================================
echo  Eye Tracking Quality Test Pipeline
echo ===================================================
echo Uso: %0 [video_file ^| --all]
echo.
echo Exemplos:
echo  %0 testing\test_data\video\mac\test1.mov  (Corre para um unico video)
echo  %0 --all                                  (Testa em batch TODOS os videos anotados)
exit /b 1


:: ==========================================
:: FUNCAO PARA PROCESSAR UM UNICO VIDEO
:: ==========================================
:process_video
set "VID=%~1"
set "DIR=%~dp1"
set "FNAME=%~n1"
set "GT=%DIR%ground_truth_%FNAME%.json"
set "PRED=%DIR%predictions_%FNAME%.csv"

echo ---------------------------------------------------
echo A processar: %VID%

:: 1. Verificar Ground Truth
if not exist "%GT%" (
    echo [AVISO] Ground truth nao encontrado para: %~nx1
    
    if "!BATCH_MODE!"=="false" (
        set /p response="Criar agora? (y/n) "
        if /i "!response!"=="y" (
            echo A abrir a ferramenta de anotacao...
            .\build\Release\annotationTool.exe "%VID%"
        ) else (
            echo A sair.
            exit /b 1
        )
    ) else (
        echo A ignorar video ^(Anota-o primeiro usando a annotation tool^).
        exit /b 1
    )
)

:: Se ainda assim nao existir GT apos a possivel anotacao
if not exist "%GT%" (
    echo [ERRO] Ground truth nao criado. A ignorar.
    exit /b 1
)

echo [OK] Ground truth confirmado.

:: 2. Gerar Previsoes (Headless Mode)
echo [^>] A gerar predicoes com eyeLike ^(Headless Mode^)...
.\build\Release\eyelike.exe -v "%VID%" -o "%PRED%" --headless

if %ERRORLEVEL% NEQ 0 (
    echo [ERRO] A predicao do eyeLike falhou.
    exit /b 1
)

echo [OK] Predicoes geradas.

:: 3. Correr os Testes de Qualidade
echo [^>] A correr os testes de qualidade...
call .\scripts\win\runTest.bat "%VID%"

if %ERRORLEVEL% NEQ 0 (
    echo [ERRO] Os testes de qualidade falharam.
    exit /b 1
)

exit /b 0


:: Fim do Script
:end
endlocal