@echo off
setlocal

REM Go to parent of script directory
cd /d "%~dp0.."

echo ================================
echo Step 1 Running rexglue migrate
echo ================================
rexglue migrate --app_root .
if errorlevel 1 (
    echo ERROR: migrate failed
    pause
    exit /b 1
)

echo ================================
echo Step 2 Configure
echo ================================
cmake --preset win-amd64-relwithdebinfo
if errorlevel 1 (
    echo ERROR: configure failed
    pause
    exit /b 1
)

echo ================================
echo Step 3 Run codegen
echo ================================
cmake --build --preset win-amd64-relwithdebinfo --target retip_codegen
if errorlevel 1 (
    echo ERROR: codegen failed
    pause
    exit /b 1
)

echo ================================
echo Step 4 Restore original files
echo ================================
if exist CMakeLists.txt.bak (
    copy /y CMakeLists.txt.bak CMakeLists.txt >nul
) else (
    echo WARNING: CMakeLists.txt.bak not found
)

if exist src\main.cpp.bak (
    copy /y src\main.cpp.bak src\main.cpp >nul
) else (
    echo WARNING: src\main.cpp.bak not found
)

echo ================================
echo Step 5 Reconfigure
echo ================================
cmake --preset win-amd64-relwithdebinfo
if errorlevel 1 (
    echo ERROR: reconfigure failed
    pause
    exit /b 1
)
pause