@echo off
rem Build the MutexTweaker core (MutexTweaker.dll).
setlocal
cd /d "%~dp0"

call :find_vs
if not defined VCToolsInstallDir (
    echo [ERROR] Visual Studio C++ build tools not found.
    echo Install "Desktop development with C++" and try again.
    exit /b 1
)

if exist build rmdir /s /q build
cmake -B build -G "NMake Makefiles"
if errorlevel 1 exit /b 1

pushd build
nmake
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" exit /b 1

echo.
echo Core built: "%~dp0build\bin\MutexTweaker.dll"
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
for %%d in (
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise"
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools"
) do (
    if exist "%%~d\VC\Auxiliary\Build\vcvars64.bat" (
        call "%%~d\VC\Auxiliary\Build\vcvars64.bat" >nul
        exit /b 0
    )
)
if exist "C:\Program1\VC\Auxiliary\Build\vcvars64.bat" call "C:\Program1\VC\Auxiliary\Build\vcvars64.bat" >nul
exit /b 0
