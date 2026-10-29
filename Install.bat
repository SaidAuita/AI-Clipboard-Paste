@echo off
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
echo  Как открыть панель в Adobe Illustrator:
echo  1. Перезапустите Adobe Illustrator (если был открыт).
echo  2. Откройте меню: Окно -> Расширения -> Clipboard Paste (Window -> Extensions -> Clipboard Paste).
echo.
echo  Горячие клавиши в активной панели:
echo  - [ C ] или [ 1 ] : Скопировать выделение в буфер (Шаг А)
echo  - [ F ] или [ 2 ] : Вставить в те же координаты (In Place)
echo  - [ V ] или [ 3 ] : Вставить по центру активного артборда
echo  - [ Tab ]         : Переключение вкладок (Буфер / Списки)
echo.
echo ================================================================================
echo.
pause
