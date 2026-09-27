@echo off
rem Runs a build from build.bat, from its own folder; works from any directory or shell.
rem   run.bat [release]   build\vs2026-release\warblade.exe
rem   run.bat debug       build\vs2026-debug\warblade.exe
rem   run.bat asan        build\vs2026-asan\warblade.exe; AddressSanitizer's report (stderr) goes
rem                       to build\vs2026-asan\asan.txt and is shown when the game exits
rem   run.bat ... x86     the 32-bit build (build\vs2026-x86-<config>\)
setlocal
set "B=release" & set "X="
for %%a in (%*) do (
  if /i "%%a"=="debug" set "B=debug"
  if /i "%%a"=="asan" set "B=asan"
  if /i "%%a"=="x86" set "X=x86-"
)
set "D=%~dp0build\vs2026-%X%%B%"
if not exist "%D%\warblade.exe" (echo %D%\warblade.exe not found: run build.bat %B% first & exit /b 1)
cd /d "%D%"
if not "%B%"=="asan" ("%D%\warblade.exe" & exit /b)
rem (a batch file waits for a GUI program to exit, unlike an interactive prompt)
"%D%\warblade.exe" 2> asan.txt
set "RC=%ERRORLEVEL%"
for %%f in (asan.txt) do if %%~zf gtr 0 (type asan.txt & echo. & echo ASan report saved in %D%\asan.txt & exit /b %RC%)
echo Exited with code %RC%, no ASan report.
exit /b %RC%
