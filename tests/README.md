# Tests

The game's tests: plain C, no framework to install.

```sh
cmake -S tests -B build/tests -G Ninja && cmake --build build/tests
build/tests/wbtests              # all game-logic tests (-j 4 to run 4 at a time)
build/tests/wbtests StrHash      # only tests whose name or file contains "StrHash"
build/tests/wbtests -l           # list them
```

- `wbtests` is the game's code (everything in `src/` except the SDL half of the engine,
  `src/core/sdl.c` and `src/core/sdl_audio.c`) linked against a fake engine
  (`fake_engine.c`, the `include/sdlhelp.h` interface without SDL), plus the `test_*.c` files.
  It needs only a C compiler, CMake and zlib.
- `wbengine` tests the real engine (`sdl.c`, `sdl_audio.c`) with the `engine_*.c` files. It
  needs SDL3, SDL3_image and SDL3_mixer; `tests/build-deps.sh` builds headless ones (dummy
  video and audio, no X11 needed) into `build/deps-test`:
  ```sh
  tests/build-deps.sh
  cmake -S tests -B build/tests -G Ninja -DCMAKE_PREFIX_PATH=$PWD/build/deps-test/prefix
  cmake --build build/tests && build/tests/wbengine
  ```
  Set `WB_TEST_DEPS` to build dependencies outside the source checkout, and pass its
  `prefix` folder through `WB_DEPS` or `CMAKE_PREFIX_PATH`.
- `python3 tests/test_runner.py` checks runner failures (temporary files, forks, output
  redirection and interrupted/failed waits) using an isolated fault injector.
  `python3 tests/test_mutate.py` checks mutation error reporting, selection and cleanup.
  CI runs both harness checks.
- Each test runs in its own forked process, so it starts from the program's initial globals,
  and a crash or a hang (20 s) fails only that test.
- No game data is needed. `BootGame()` (`support.h`) runs the real start-up (`GameMain`) up to
  its main loop with the fake's images and sounds; `RunFrames()` then drives the loop, with
  keys and the mouse set through `g_fake` (`fake_engine.h`). Level, pattern and other archive
  files a test needs, it adds with `FakePacAdd`.

## Writing tests

```c
#include "support.h"

TEST(StrHash_is_the_31_polynomial)
{
    CHECK_EQ_INT(StrHash("abc"), ('a' * 31 + 'b') * 31 + 'c');
}
```

`test.h` has the checks: `CHECK`, `CHECK_MSG`, `CHECK_EQ_INT`, `CHECK_NE_INT`, `CHECK_NEAR`,
`CHECK_STR`, `CHECK_MEM`. The first failing check ends the test.

## Red-green: `tests/mutate.py`

Every test must be able to fail. `tests/mutations/*.txt` lists deliberate breakages of the
game's code, each with the tests that must catch it:

```
== strutil: StrHash multiplier
file: src/core/strutil.c
expect: StrHash_is_the_31_polynomial
- h = h * 31 + s[i];
+ h = h * 37 + s[i];
```

`tests/mutate.py` applies them one at a time (to a copy of the tree, never the working tree),
rebuilds, runs the named expected tests (or the full suite when none are named), and reports
any mutation the tests don't catch:

```sh
tests/mutate.py            # all mutations, in parallel
tests/mutate.py -k strutil # those whose name contains "strutil"
```

When adding a test, add a mutation that breaks what it checks, and see it reported as killed.

The full game and engine suites run before any mutation. Temporary build trees are cleaned
on success and failure. The mutation process still checks every named expected failure.

## Regression coverage

The suite covers keyboard, physical gamepad, touch and legacy joystick input; both orders
of two-player collision dispatch; each obstruction box set; gem respawn without double
movement; hit-mask filenames and dimensions; and ownership through profile migrations.
Regression mutations deliberately restore the faulty behavior to check these assertions.
