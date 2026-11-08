@echo off
setlocal
cd /d "%~dp0"
where cl.exe >nul 2>nul
if errorlevel 1 (
  if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
  ) else (
    echo Open the x64 Native Tools Command Prompt for Visual Studio, then run this file.
    exit /b 1
  )
)
cl /nologo /std:c++17 /EHsc controller_tests.cpp /Fecontroller_tests.exe /Focontroller_tests.obj
if errorlevel 1 exit /b 1
controller_tests.exe
exit /b %errorlevel%
