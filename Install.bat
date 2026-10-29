@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

title Clipboard Paste v1.0.12 - Installer
cd /d "%~dp0"

echo ================================================================================
echo                     CLIPBOARD PASTE v1.0.12 - INSTALLER
echo               In-Place Duplicator Extension for Adobe Illustrator
echo ================================================================================
echo.

echo [1/3] Enabling Adobe CEP PlayerDebugMode in Windows Registry...
for /L %%V in (7,1,18) do (
    reg add "HKCU\Software\Adobe\CSXS.%%V" /v PlayerDebugMode /t REG_SZ /d 1 /f >nul 2>&1
)
echo  [OK] PlayerDebugMode enabled for CSXS.7 through CSXS.18.
echo.

echo [2/3] Clearing old CEP cache...
for /d %%C in ("%LOCALAPPDATA%\Temp\cep_cache\ILST_*com.clipboardpaste*") do (
    rmdir /s /q "%%C" >nul 2>&1
)
echo  [OK] CEP cache cleared.
echo.

echo [3/3] Installing extension to Adobe CEP directory...
set "TARGET_DIR=%APPDATA%\Adobe\CEP\extensions\Clipboard Paste"

if not exist "%APPDATA%\Adobe\CEP\extensions" (
    mkdir "%APPDATA%\Adobe\CEP\extensions" >nul 2>&1
)

if exist "%TARGET_DIR%" (
    rmdir /s /q "%TARGET_DIR%" >nul 2>&1
)
mkdir "%TARGET_DIR%" >nul 2>&1

:: Copy CEP extension files
xcopy /E /I /Y "CSXS" "%TARGET_DIR%\CSXS\" >nul
xcopy /E /I /Y "css" "%TARGET_DIR%\css\" >nul
xcopy /E /I /Y "icons" "%TARGET_DIR%\icons\" >nul
xcopy /E /I /Y "js" "%TARGET_DIR%\js\" >nul
xcopy /E /I /Y "jsx" "%TARGET_DIR%\jsx\" >nul
xcopy /E /I /Y "actions" "%TARGET_DIR%\actions\" >nul
copy /Y "index.html" "%TARGET_DIR%\" >nul
copy /Y "cp_copy.html" "%TARGET_DIR%\" >nul
copy /Y "cp_place.html" "%TARGET_DIR%\" >nul
copy /Y "cp_paste.html" "%TARGET_DIR%\" >nul
copy /Y ".debug" "%TARGET_DIR%\" >nul
copy /Y "CHANGELOG.md" "%TARGET_DIR%\" >nul
copy /Y "PROJECT_STATUS.md" "%TARGET_DIR%\" >nul

echo  [OK] Extension installed safely to:
echo       %TARGET_DIR%
echo.

echo ================================================================================
echo  INSTALLATION COMPLETE!
echo ================================================================================
echo.
echo  How to open extension in Adobe Illustrator:
echo  1. Restart Adobe Illustrator (if it is currently running).
echo  2. Open menu: Window -^> Extensions -^> Clipboard Paste.
echo.
echo  Shortcuts in active panel:
echo  - [ C ] or [ 1 ]  : Copy selection to buffer (Step A)
echo  - [ F ] or [ 2 ]  : Paste into same coordinates (In Place)
echo  - [ V ] or [ 3 ]  : Paste to center of active artboard
echo  - [ Tab ]         : Switch tabs (Buffer / Lists)
echo.
echo ================================================================================
echo.
pause
