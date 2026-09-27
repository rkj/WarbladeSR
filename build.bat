@echo off
rem Builds Warblade with VS2026 (MSVC) and vcpkg (SDL_PLAN.md Step 3). 64-bit by default.
rem   build.bat [release]   build\vs2026-release\warblade.exe
rem   build.bat debug       build\vs2026-debug\warblade.exe
rem   build.bat asan        build\vs2026-asan\warblade.exe (Debug, /fsanitize=address)
rem   build.bat ... x86     32-bit build in build\vs2026-x86-<config>\
rem SDL3 and its libraries are linked statically (triplet <arch>-windows-static, static CRT),
rem so warblade.exe needs no DLLs. The output folder is runnable: data\ is a junction to ..\game\data.
setlocal
set "CFG=Release" & set "ASAN=OFF" & set "B=release" & set "ARCH=x64"
for %%a in (%*) do (
  if /i "%%a"=="debug" (set "CFG=Debug" & set "B=debug")
  if /i "%%a"=="asan" (set "CFG=Debug" & set "ASAN=ON" & set "B=asan")
  if /i "%%a"=="x86" set "ARCH=x86"
)
if "%ARCH%"=="x86" set "B=x86-%B%"
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
for /f "usebackq delims=" %%i in (`vswhere -latest -version [18.0^,19.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%i"
if not defined VS (echo Visual Studio 2026 with the C++ workload was not found & exit /b 1)
call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" %ARCH% >nul || exit /b 1
cd /d "%~dp0"
set "TRIPLET=%ARCH%-windows-static"
rem A folder configured for another triplet (the old DLL build) is configured again.
if exist "build\vs2026-%B%\CMakeCache.txt" (
  findstr /x /c:"VCPKG_TARGET_TRIPLET:STRING=%TRIPLET%" "build\vs2026-%B%\CMakeCache.txt" >nul || (
    del "build\vs2026-%B%\CMakeCache.txt" "build\vs2026-%B%\build.ninja"
    rmdir /s /q "build\vs2026-%B%\CMakeFiles"
  )
)
if not exist "build\vs2026-%B%\build.ninja" (
  cmake -S . -B "build\vs2026-%B%" -G Ninja -DCMAKE_BUILD_TYPE=%CFG% -DWARBLADE_ASAN=%ASAN% ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=%TRIPLET% || exit /b 1
)
cmake --build "build\vs2026-%B%" || exit /b 1
