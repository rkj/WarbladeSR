# Makes the build folder runnable: cmake -DGAME=... -DEXTRA=... -DDST=... -P postbuild.cmake
#   GAME       the original install: warblade.ico is copied, data/ is linked (junction). The
#              jukebox (Jukebox.exe, fmod.dll) is not: players use the one in their install.
#   EXTRA      more files to copy (the ASan runtime)
#   DST        the output folder
#
# SDL3 and its libraries are linked into warblade.exe (an x64- or x86-windows-static triplet), so there
# are no DLLs to copy. Older builds linked them dynamically; their DLLs, and the jukebox they
# copied, are removed.
set(stale
    SDL3.dll SDL3_image.dll SDL3_mixer.dll jpeg62.dll libpng16.dll libpng16d.dll z.dll zd.dll
    mpg123.dll libxmp.dll turbojpeg.dll spng.dll out123.dll syn123.dll Jukebox.exe fmod.dll)
foreach(name IN LISTS stale)
    file(REMOVE "${DST}/${name}")
endforeach()

file(COPY ${EXTRA} "${GAME}/warblade.ico" DESTINATION "${DST}")
if(NOT EXISTS "${DST}/data")
    file(TO_NATIVE_PATH "${DST}/data" link)
    file(TO_NATIVE_PATH "${GAME}/data" target)
    execute_process(COMMAND cmd /c mklink /J "${link}" "${target}" OUTPUT_QUIET RESULT_VARIABLE r)
    if(r)
        message(FATAL_ERROR "could not link ${link} to ${target}")
    endif()
endif()
