# Warblade, SDL port (SDL_PLAN.md). VS2026 (MSVC, x64 or x86) + vcpkg; run build.bat rather than
# configuring by hand. The engine underneath include/sdlhelp.h is SDL3, SDL3_image and
# SDL3_mixer (src/core/sdl.c, src/core/sdl_audio.c), linked statically (vcpkg triplet
# x64-windows-static or x86-windows-static) with the static C runtime: warblade.exe needs no DLLs but Windows'.
cmake_minimum_required(VERSION 3.25)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")   # /MT, /MTd
project(warblade C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(WARBLADE_ASAN "Build with /fsanitize=address" OFF)
if(DEFINED ENV{WARBLADE_GAME_DIR})
    set(_game_dir "$ENV{WARBLADE_GAME_DIR}")
else()
    set(_game_dir "${CMAKE_SOURCE_DIR}/../game")
endif()
set(WARBLADE_GAME_DIR "${_game_dir}" CACHE PATH
    "The original install: data/ is linked into the output folder, the icon is copied from it")
if(NOT EXISTS "${WARBLADE_GAME_DIR}/warblade.ico")
    message(FATAL_ERROR "warblade.ico not found in ${WARBLADE_GAME_DIR}: "
                        "set WARBLADE_GAME_DIR to your Warblade folder")
endif()
# The exe's icon comes from the install (res/warblade.rc finds it in the build folder).
configure_file("${WARBLADE_GAME_DIR}/warblade.ico" "${CMAKE_BINARY_DIR}/res/warblade.ico" COPYONLY)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # tools/compile.py reads it

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
add_executable(warblade WIN32 ${GAME_SOURCES} res/warblade.rc)
set_property(SOURCE res/warblade.rc APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/res")
target_include_directories(warblade PRIVATE include)
target_compile_definitions(warblade PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(warblade PRIVATE /W3)
target_link_libraries(warblade PRIVATE
    SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer ZLIB::ZLIB)
# The game's entry point is a plain main() (GameMain); SDL needs no SDL_main.
target_link_options(warblade PRIVATE /ENTRY:mainCRTStartup)

if(WARBLADE_ASAN)
    # /RTC (in CMake's Debug flags) and ASan don't mix
    foreach(v CMAKE_C_FLAGS_DEBUG)
        string(REGEX REPLACE "/RTC[1csu]*" "" ${v} "${${v}}")
    endforeach()
    target_compile_options(warblade PRIVATE /fsanitize=address /Zi)
    target_link_options(warblade PRIVATE /INCREMENTAL:NO)
    get_filename_component(_cl_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        file(GLOB ASAN_DLLS "${_cl_dir}/clang_rt.asan*-x86_64.dll")
    else()
        file(GLOB ASAN_DLLS "${_cl_dir}/clang_rt.asan*-i386.dll")
    endif()
endif()

# Next to the exe: warblade.ico from the install, the ASan runtime, and a junction to its data/
# (cmake/postbuild.cmake).
add_custom_command(TARGET warblade POST_BUILD
    COMMAND ${CMAKE_COMMAND} "-DGAME=${WARBLADE_GAME_DIR}"
            "-DEXTRA=${ASAN_DLLS}" "-DDST=$<TARGET_FILE_DIR:warblade>"
            -P "${CMAKE_SOURCE_DIR}/cmake/postbuild.cmake"
    VERBATIM)
