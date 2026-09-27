@echo off
rem Builds build\warblade.exe with Visual Studio 2008 (RTM, not SP1) and the prebuilt PTK engine
rem in lib\. The icon is taken from a Warblade install: set WARBLADE_GAME_DIR to its folder
rem (default: ..\game).
rem
rem The sources compile with the original's flags and link in a fixed order (sorted by path),
rem which keeps the original's code and data layout.
setlocal enabledelayedexpansion
cd /d "%~dp0"
if not defined VS90COMNTOOLS (echo Visual Studio 2008 was not found: VS90COMNTOOLS is not set & exit /b 1)
call "%VS90COMNTOOLS%..\..\VC\bin\vcvars32.bat" >nul || exit /b 1
if not defined WARBLADE_GAME_DIR set "WARBLADE_GAME_DIR=%~dp0..\game"
if not exist "%WARBLADE_GAME_DIR%\warblade.ico" (echo warblade.ico not found in "%WARBLADE_GAME_DIR%": set WARBLADE_GAME_DIR to your Warblade folder & exit /b 1)
if not exist build\obj mkdir build\obj

set "CFLAGS=/nologo /c /Od /ZI /RTC1 /MTd /EHsc /W3 /TP /D WIN32 /D _DEBUG /D _WINDOWS /I include"
set "OBJS="
for %%f in (
  src\audio\music.cpp ^
  src\audio\sound.cpp ^
  src\core\init.cpp ^
  src\core\main.cpp ^
  src\core\mathutil.cpp ^
  src\core\platform.cpp ^
  src\core\random.cpp ^
  src\core\strutil.cpp ^
  src\core\stubs.cpp ^
  src\data\globals.cpp ^
  src\data\static_init.cpp ^
  src\game\collide.cpp ^
  src\game\enemies.cpp ^
  src\game\gameflow.cpp ^
  src\game\gameover.cpp ^
  src\game\hud.cpp ^
  src\game\hurryup.cpp ^
  src\game\input.cpp ^
  src\game\items.cpp ^
  src\game\level.cpp ^
  src\game\levelobj.cpp ^
  src\game\levelstart.cpp ^
  src\game\mapobj.cpp ^
  src\game\player.cpp ^
  src\game\promotion.cpp ^
  src\game\stages\bonusround.cpp ^
  src\game\stages\gemdrop.cpp ^
  src\game\stages\memorystation.cpp ^
  src\game\stages\meteorstorm.cpp ^
  src\game\stages\shop.cpp ^
  src\game\warp.cpp ^
  src\gfx\explosions.cpp ^
  src\gfx\frames.cpp ^
  src\gfx\particles.cpp ^
  src\gfx\render.cpp ^
  src\gfx\resources.cpp ^
  src\gfx\stars.cpp ^
  src\gfx\text.cpp ^
  src\online\net.cpp ^
  src\online\news.cpp ^
  src\online\online.cpp ^
  src\profile\account.cpp ^
  src\profile\hiscore.cpp ^
  src\profile\savegame.cpp ^
  src\profile\settings.cpp ^
  src\profile\stats.cpp ^
  src\ui\controls.cpp ^
  src\ui\menu.cpp ^
  src\ui\screens.cpp ^
  src\ui\title.cpp ^
  src\ui\window.cpp
) do (
  cl %CFLAGS% /Fobuild\obj\%%~nf.obj /Fdbuild\obj\%%~nf.pdb %%f >build\obj\%%~nf.log || (type build\obj\%%~nf.log & exit /b 1)
  set "OBJS=!OBJS! build\obj\%%~nf.obj"
)

copy /y "%WARBLADE_GAME_DIR%\warblade.ico" build\warblade.ico >nul || exit /b 1
rc /i build /fo build\warblade.res res\warblade.rc || exit /b 1

rem PE options as in the original: base 0x400000, no relocations, no ASLR, no NX.
rem LNK4075 (/ZI objects under /INCREMENTAL:NO) is expected.
link /nologo /DEBUG /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /INCREMENTAL:NO /FIXED ^
  /BASE:0x400000 /DYNAMICBASE:NO /NXCOMPAT:NO /MACHINE:X86 /MANIFEST:NO ^
  /MAP:build\warblade.map /OUT:build\warblade.exe !OBJS! lib\ptk.obj build\warblade.res ^
  kernel32.lib user32.lib gdi32.lib advapi32.lib winmm.lib ws2_32.lib wininet.lib ^
  opengl32.lib shell32.lib ole32.lib lib\ddraw.lib lib\bass.lib || exit /b 1
echo Built build\warblade.exe
