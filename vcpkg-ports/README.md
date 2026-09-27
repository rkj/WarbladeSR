# vcpkg overlay ports

`vcpkg.json` points vcpkg here (`vcpkg-configuration.overlay-ports`); a port in this folder
replaces the registry's port of the same name.

- `sdl3-mixer`: vcpkg's `sdl3-mixer` 3.2.4 plus a `drmp3` feature. The upstream port always
  builds with `-DSDLMIXER_MP3_DRMP3=OFF`, so MP3 needed `mpg123`, which is LGPL 2.1 and can't be
  linked statically without shipping relinkable objects. dr_mp3 (public domain / MIT-0) is part
  of SDL_mixer. The changes: the `drmp3` feature in `vcpkg.json` (and `port-version` 1), its
  `SDLMIXER_MP3_DRMP3` line in `vcpkg_check_features`, `SDLMIXER_MP3` set from either decoder
  feature instead of from `mpg123` alone, and no `-DSDLMIXER_MP3_DRMP3=OFF`. When updating the
  builtin-baseline, copy the new upstream port here and redo them.
- `sdl3`: vcpkg's `sdl3` 3.4.16 (`port-version` 2) built without what the game, SDL3_image and
  SDL3_mixer don't use: the GPU API and its renderer, camera, haptic, power, sensor, dialog and
  tray subsystems, OpenGL ES, and HIDAPI (dedicated drivers for PlayStation/Switch/etc.
  controllers; without it they are generic DirectInput joysticks, as with the original's winmm,
  and Xbox pads use XInput). The D3D9/11/12, OpenGL and Vulkan renderers stay (the settings
  page offers them). The options are marked "Warblade" in `portfile.cmake`; redo them when
  updating the builtin-baseline.
