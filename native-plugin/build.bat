@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

title Building Clipboard Paste Native Plugin (.aip)
cd /d "%~dp0"

echo ================================================================================
echo         CLIPBOARD PASTE - NATIVE C++ PLUGIN (.aip) BUILD SCRIPT
echo ================================================================================
echo.

:: 1. Locate MSVC Environment
set "VCVARS="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VCVARS%"=="" (
    echo [ERROR] Visual Studio 2022 C++ x64 Build Environment not found!
    echo Please ensure VS 2022 BuildTools with C++ workload is installed.
    pause
    exit /b 1
)

echo [*] Initializing MSVC x64 Environment...
call "%VCVARS%" >nul
if errorlevel 1 (
    echo [ERROR] Failed to initialize vcvars64.bat!
    pause
    exit /b 1
)

echo  [OK] MSVC x64 Environment Ready.
echo.

:: 2. Prepare Output Directory
set "OUT_DIR=%~dp0build\x64"
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%" >nul 2>&1

:: 3. Compile Windows Resource (.rc)
echo [*] Compiling resource script (ClipboardPaste.rc)...
rc.exe /nologo /fo "%OUT_DIR%\ClipboardPaste.res" "resources\ClipboardPaste.rc"
if errorlevel 1 (
    echo [ERROR] Resource compilation failed!
    pause
    exit /b 1
)
echo  [OK] Resource compiled.
echo.

:: 4. Compile and Link C++ .aip (DLL x64) using Adobe Illustrator SDK
if not defined SDK_ROOT (
    if defined ILLUSTRATOR_SDK (
        set "SDK_ROOT=%ILLUSTRATOR_SDK%"
    ) else if exist "c:\_CODE\illustrator\SDK\AI_2026_SDK_Win\Adobe Illustrator 2026 SDK" (
        set "SDK_ROOT=c:\_CODE\illustrator\SDK\AI_2026_SDK_Win\Adobe Illustrator 2026 SDK"
    ) else if exist "%~dp0..\..\SDK\AI_2026_SDK_Win\Adobe Illustrator 2026 SDK" (
        set "SDK_ROOT=%~dp0..\..\SDK\AI_2026_SDK_Win\Adobe Illustrator 2026 SDK"
    )
)

if not exist "%SDK_ROOT%" (
    echo [ERROR] Adobe Illustrator SDK directory not found!
    echo Please set ILLUSTRATOR_SDK environment variable to the SDK root folder.
    pause
    exit /b 1
)

set "SDK_INCLUDES=/I"%SDK_ROOT%\illustratorapi\illustrator" /I"%SDK_ROOT%\illustratorapi\illustrator\actions" /I"%SDK_ROOT%\illustratorapi\pica_sp" /I"%SDK_ROOT%\illustratorapi\ate" /I"%SDK_ROOT%\samplecode\common\includes""

echo [*] Compiling C++ source files into ClipboardPaste.aip...
cl.exe /nologo /std:c++17 /O2 /MD /EHsc /W3 /LD ^
    /DWIN_ENV=1 /DWIN32 /D_WINDOWS ^
    %SDK_INCLUDES% ^
    "src\PluginMain.cpp" ^
    "src\ClipboardPastePlugin.cpp" ^
    "src\BufferManager.cpp" ^
    "src\ConfigManager.cpp" ^
    "src\Suites.cpp" ^
    "%SDK_ROOT%\illustratorapi\illustrator\IAIUnicodeString.cpp" ^
    "%SDK_ROOT%\illustratorapi\illustrator\IAIArtboards.cpp" ^
    "%OUT_DIR%\ClipboardPaste.res" ^
    User32.lib Gdi32.lib Shell32.lib ^
    /Fo"%OUT_DIR%\\" ^
    /Fe"%OUT_DIR%\ClipboardPaste.aip" ^
    /link /DLL /MACHINE:X64 /SUBSYSTEM:WINDOWS /EXPORT:PluginMain

if errorlevel 1 (
    echo.
    echo [ERROR] Compilation failed!
    exit /b 1
)

echo.
echo ================================================================================
echo  [SUCCESS] Native Plugin Created:
echo  %OUT_DIR%\ClipboardPaste.aip
echo ================================================================================
echo.
echo To install into Adobe Illustrator, copy 'ClipboardPaste.aip' into:
echo   C:\Program Files\Adobe\Adobe Illustrator [Version]\Plug-ins\
echo.
