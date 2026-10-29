@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   Adobe Illustrator Clipboard Doctor - Build Script
echo ========================================================
echo.

cd /d "%~dp0"

echo [1/3] Cleaning previous builds...
if exist "build" rmdir /s /q "build"
mkdir "build"

echo [2/3] Building Self-Contained Standalone Single-File (.exe)...
dotnet publish -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -p:EnableCompressionInSingleFile=true -o "build\standalone"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile Standalone build!
    exit /b %errorlevel%
)
move /Y "build\standalone\AI-ClipboardDoctor.exe" "build\AI-ClipboardDoctor.exe" >nul
del /q "build\standalone\*.pdb" >nul 2>&1
rmdir /s /q "build\standalone" >nul 2>&1

echo [3/3] Building Ultralight Framework-Dependent (.exe)...
dotnet publish -c Release -r win-x64 --self-contained false -p:PublishSingleFile=true -o "build\lite"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile Lite build!
    exit /b %errorlevel%
)
move /Y "build\lite\AI-ClipboardDoctor.exe" "build\AI-ClipboardDoctor-Lite.exe" >nul
move /Y "build\lite\AI-ClipboardDoctor.runtimeconfig.json" "build\AI-ClipboardDoctor-Lite.runtimeconfig.json" >nul 2>&1
del /q "build\lite\*.pdb" >nul 2>&1
rmdir /s /q "build\lite" >nul 2>&1

echo [*] Copying scripts...
if not exist "build\scripts" mkdir "build\scripts"
copy /Y "scripts\AI_RescueCopy.jsx" "build\scripts\AI_RescueCopy.jsx" >nul

echo.
echo ========================================================
echo   Build Successful!
echo   Outputs in: %~dp0build\
echo   - Standalone: build\AI-ClipboardDoctor.exe
echo   - Lite:       build\AI-ClipboardDoctor-Lite.exe
echo ========================================================
pause
