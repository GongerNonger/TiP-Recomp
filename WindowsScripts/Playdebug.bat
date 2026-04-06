@echo off
setlocal

cd /d "%~dp0.."

echo Closing retip.exe if running...
taskkill /f /im retip.exe >nul 2>&1

echo Copying retip.exe, PDB, and DLLs to the batch file's directory...
copy /y "out\build\win-amd64-relwithdebinfo\retip.exe" .
copy /y "out\build\win-amd64-relwithdebinfo\retip.pdb" . 2>nul
copy /y "out\build\win-amd64-relwithdebinfo\*.dll" .

if not exist retip.exe (
    echo ERROR: Failed to copy retip.exe to %cd%
    pause
    exit /b 1
)

echo Locating Visual Studio installation...
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do set VS_PATH=%%i

if not defined VS_PATH (
    echo ERROR: Could not find Visual Studio installation via vswhere.
    pause
    exit /b 1
)

echo Launching retip.exe under Visual Studio debugger...
"%VS_PATH%\Common7\IDE\devenv.exe" /debugexe retip.exe --gpu_allow_invalid_fetch_constants=true --enable_console=false --scribble_heap=true --vsync=off --fullscreen=true --video_mode_refresh_rate=164

pause