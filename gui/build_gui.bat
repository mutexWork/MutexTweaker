@echo off
rem Build the MutexTweaker GUI. Intermediates go to build\obj, output to build\bin.
setlocal
cd /d "%~dp0"

call :find_vs
if not defined VCToolsInstallDir (
    echo [ERROR] Visual Studio C++ build tools not found.
    exit /b 1
)

if not exist "..\core\build\bin\MutexTweaker.dll" (
    echo Building core first...
    call "..\core\build_core.bat"
    if errorlevel 1 exit /b 1
)

if not exist "build\obj" mkdir "build\obj"
if not exist "build\bin" mkdir "build\bin"
del /q "build\obj\*.obj" 2>nul
del /q "build\obj\app.res" 2>nul

rc /nologo /fo "build\obj\app.res" "app.rc"
if errorlevel 1 exit /b 1

cl /nologo /utf-8 /DUNICODE /D_UNICODE /EHsc /W3 /MT /O2 ^
   /I"..\core\include" ^
   /Fo"build\obj\\" ^
   /Fe:"build\bin\MutexTweakerGUI.exe" ^
   "src\main.cpp" "build\obj\app.res" ^
   /link /SUBSYSTEM:WINDOWS ^
   /MANIFEST:EMBED /MANIFESTINPUT:"app.manifest" /MANIFESTUAC:NO ^
   /LIBPATH:"..\core\build\lib" ^
   MutexTweaker.lib comctl32.lib user32.lib gdi32.lib gdiplus.lib shell32.lib advapi32.lib
if errorlevel 1 exit /b 1
if not exist "build\bin\MutexTweakerGUI.exe" exit /b 1

copy /Y "..\core\build\bin\MutexTweaker.dll" "build\bin\MutexTweaker.dll" >nul
echo.
echo GUI built: "%~dp0build\bin\MutexTweakerGUI.exe"
endlocal
exit /b 0

:find_vs
if defined VCToolsInstallDir exit /b 0
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
)
if defined VSDIR if exist "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" (
    call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
    exit /b 0
)
if exist "C:\Program1\VC\Auxiliary\Build\vcvars64.bat" call "C:\Program1\VC\Auxiliary\Build\vcvars64.bat" >nul
exit /b 0
