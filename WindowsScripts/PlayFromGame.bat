@echo off
setlocal

set BUILD_EXE=C:\Users\Administrator\Downloads\TiP-Recomp\out\build\win-amd64-relwithdebinfo\retip.exe
set GAME_DIR=C:\Users\Administrator\Downloads\reTiP\reTiP\Game
set GAME_EXE=%GAME_DIR%\reTiP.exe

if not exist "%BUILD_EXE%" (
    echo ERROR: Build output not found at %BUILD_EXE%
    echo Run Rebuild.bat first.
    pause
    exit /b 1
)

echo Copying fresh build into Game folder...
copy /y "%BUILD_EXE%" "%GAME_EXE%"
if errorlevel 1 (
    echo ERROR: Could not copy build to Game folder. Is the game still running?
    pause
    exit /b 1
)

cd /d "%GAME_DIR%"
echo Starting reTiP.exe from %GAME_DIR% ...
"%GAME_EXE%" --gpu_allow_invalid_fetch_constants=true --enable_console=false --scribble_heap=true --vsync=off --fullscreen=true --video_mode_refresh_rate=164

echo Game exited.
pause
