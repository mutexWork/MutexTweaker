@echo off
rem Сборка GUI MutexTweaker. Все промежуточные файлы складываются в build\obj.
setlocal
if not defined VCINSTALLDIR call "C:\Program1\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d C:\MutexTweakerGUI

if not exist ".\build\obj" mkdir ".\build\obj"
if not exist ".\build\bin" mkdir ".\build\bin"
del /q ".\build\obj\*.obj" 2>nul
del /q ".\build\obj\app.res" 2>nul

rc /nologo /fo ".\build\obj\app.res" ".\app.rc"
if errorlevel 1 goto fail

cl /nologo /utf-8 /DUNICODE /D_UNICODE /EHsc /W3 /MT /O2 ^
   /I".\MutexTweakerCore\include" ^
   /Fo".\build\obj\\" ^
   /Fe:".\build\bin\MutexTweakerGUI.exe" ^
   ".\src\main.cpp" ".\build\obj\app.res" ^
   /link /SUBSYSTEM:WINDOWS ^
   /MANIFEST:EMBED /MANIFESTINPUT:".\app.manifest" /MANIFESTUAC:NO ^
   /LIBPATH:".\MutexTweakerCore\lib" ^
   MutexTweaker.lib comctl32.lib user32.lib gdi32.lib gdiplus.lib shell32.lib advapi32.lib

if errorlevel 1 goto fail
if not exist ".\build\bin\MutexTweakerGUI.exe" goto fail

copy /Y ".\MutexTweakerCore\bin\MutexTweaker.dll" ".\build\bin\MutexTweaker.dll" >nul

echo BUILD_OK > ".\gui_status.txt"
goto end

:fail
echo BUILD_FAIL > ".\gui_status.txt"
:end
endlocal
