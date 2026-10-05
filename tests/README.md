# Tests for the Docker image

`./tests/run.sh` builds the image (`Dockerfile`) and a stand-in game (`stub/`). It then
starts the image in four containers, read-only and with no capabilities, as it should be
deployed:

| Container | Serves | Used by |
| --------- | ------ | ------- |
| `real`   | the built page and game, test data, and an old-style save volume at `/saves` | HTTP tests, CSP check |
| `stub`   | the built page with the stand-in game, test data | browser tests |
| `local`  | the built page with the stand-in game, no data | browser tests (own-folder mode) |
| `limits` | like `real`, with a 32 MiB file | rate and connection limit tests |

Then it runs:

* `test_server.py` (Python standard library, `unittest`): no save API, no writes, path
  handling, headers, and request size, timeout and rate limits.
* `browser.test.cjs` (Node.js `node:test` with Playwright's Chromium): play, reload and
  keep the profile; separate browsers keep separate saves; export and import; the old
  volume's tar; the CSP holds.

The stand-in game (`stub/stub.c`) is built with the same Emscripten options as the real one.
It reads `data/warblade.pac` and counts its runs in a profile file under
`/save/warblade`, so the page's real file system and IndexedDB code are tested without the
Warblade data, which isn't in the repository.

```sh
cd tests && npm ci && npx playwright install chromium   # once
./tests/run.sh
```

* `IMAGE=<image>` tests an existing image instead of building one.
* `STUB_DIR=<dir>` uses an already-built stand-in game.
* `KEEP=1` leaves the containers running.

To see what the tests guard against, run the save tests against any server, for example the
old `docker/server.py` from git history:
`WARBLADE_URL=http://host:port python3 -m unittest tests.test_server.Saves`.

See `docs/web-security.md` for the design and the threat model behind these tests.
