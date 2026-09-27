# Makes the build folder runnable: cmake -DVCPKG_BIN=... -DGAME=... -DEXTRA=... -DDST=... -P postbuild.cmake
#   VCPKG_BIN  the triplet's bin/ (every DLL is copied: SDL3_mixer loads libxmp.dll at run time)
#   GAME       the original install: warblade.ico and the jukebox (Jukebox.exe, fmod.dll) are
#              copied, data/ is linked (junction)
#   EXTRA      more files to copy (the ASan runtime)
file(GLOB dlls "${VCPKG_BIN}/*.dll")
file(COPY ${dlls} ${EXTRA} "${GAME}/warblade.ico" "${GAME}/Jukebox.exe" "${GAME}/fmod.dll"
     DESTINATION "${DST}")
if(NOT EXISTS "${DST}/data")
    file(TO_NATIVE_PATH "${DST}/data" link)
    file(TO_NATIVE_PATH "${GAME}/data" target)
    execute_process(COMMAND cmd /c mklink /J "${link}" "${target}" OUTPUT_QUIET RESULT_VARIABLE r)
    if(r)
        message(FATAL_ERROR "could not link ${link} to ${target}")
    endif()
endif()
