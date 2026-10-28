@echo off
rem Full release build: core -^> GUI -^> installer.
setlocal
cd /d "%~dp0"

call "core\build_core.bat"
if errorlevel 1 goto fail

call "gui\build_gui.bat"
if errorlevel 1 goto fail

set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" (
    echo [WARN] Inno Setup 6 not found - installer skipped.
    goto ok
)
"%ISCC%" "gui\build_installer.iss"
if errorlevel 1 goto fail
echo.
echo Installer: "%~dp0gui\Output\MutexTweaker_Setup.exe"
goto ok

:fail
echo.
echo BUILD FAILED
exit /b 1

:ok
endlocal
exit /b 0
