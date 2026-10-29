@echo off
setlocal enabledelayedexpansion

title Clipboard Paste - Uninstaller

:: --- STAGE 0: ELEVATION CHECK ---
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [INFO] Requesting Administrator privileges...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

set "TARGET_DIR=%APPDATA%\Adobe\CEP\extensions\Clipboard Paste"

echo ================================================================================
echo                     CLIPBOARD PASTE - UNINSTALLER
echo ================================================================================
echo.

if exist "%TARGET_DIR%" (
    echo Deleting extension files...
    rmdir /s /q "%TARGET_DIR%" >nul 2>&1
    echo  [OK] Extension removed from: %TARGET_DIR%
) else (
    echo  [INFO] Extension directory not found.
)

:: Clean CEP cache
for /d %%C in ("%LOCALAPPDATA%\Temp\cep_cache\ILST_*com.clipboardpaste.panel*") do (
    rmdir /s /q "%%C" >nul 2>&1
)
echo  [OK] CEP cache cleared.

:: Clean scripts from presets
for /d %%D in ("C:\Program Files\Adobe\Adobe Illustrator *") do (
    if exist "%%D\Presets" (
        for /d %%L in ("%%D\Presets\*") do (
            del /f /q "%%L\Scripts\Buffer_*.jsx" >nul 2>&1
            if exist "%%L\Scripts\Clipboard Paste" (
                rmdir /s /q "%%L\Scripts\Clipboard Paste" >nul 2>&1
            )
        )
    )
)
echo  [OK] Preset scripts removed.

echo.
echo ================================================================================
echo  UNINSTALLATION COMPLETE!
echo ================================================================================
echo.
pause
