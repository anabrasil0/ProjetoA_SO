@echo off
taskkill /f /im simulador.exe 2>nul
del %TEMP%\simulador.exe 2>nul
g++ -std=c++17 -o %TEMP%\simulador.exe main.cpp Config.cpp Task.cpp CPU.cpp Simulador.cpp SRTFScheduler.cpp PRIOPScheduler.cpp PRIOPEnvScheduler.cpp GanttLog.cpp GanttChart.cpp MutexSim.cpp Evento.cpp
if %errorlevel% neq 0 (
    echo.
    echo Erro na compilacao. Verifique os erros acima.
    pause
    exit /b 1
)
echo Compilado com sucesso!
echo.
cd /d "%~dp0"
%TEMP%\simulador.exe
