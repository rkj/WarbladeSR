# Web deployment: security review, data flow, migration, deployment

This covers the Docker image that serves the browser version (`Dockerfile`, `docker/`,
`web/`). It replaces the image published as `ghcr.io/rkj/warbladesr:latest` at
`sha256:f6c16864f5c38bce1e0ec94c1f4521dda4cd39c0bc8d6fe1d0b4db607d43f76e`, which served the
game with `/app/server.py` (Python `http.server`). That image's layers could not be pulled
for this review; it was reviewed from the source the workflow builds it from (`main` at
`318adc1`: `docker/server.py`, `web/index.html`, `Dockerfile`).

The new design: **a read-only static site.** The server delivers the page, the game and (if
mounted) the Warblade data, and accepts nothing. Profiles, saves, settings and high scores
stay in each player's browser.

## 1. Findings in the previous image

Each finding was reproduced against `docker/server.py` from `main`, run locally with a save
folder holding one profile in the game's format.

| # | Severity | Finding |
| - | -------- | ------- |
| 1 | Critical | **Anyone who can reach the server can list, read, overwrite and delete every save.** `GET /api/saves` lists the save volume, `GET/PUT/DELETE /saves/<path>` read, replace or remove any file in it. The server has no authentication or per-player separation: Authelia was the only gate. Reproduced: listed `warblade/profiles/profile000.acc`, read it, overwrote it with `pwned` (204), deleted it (204). |
| 2 | High | **Stored cross-site scripting.** `PUT` accepts any name, and `GET` serves the file with a type guessed from its extension. `PUT /saves/x.svg` with an SVG `<script>` is served back as `image/svg+xml` from the game's origin (`.svg` is also the game's own suspended-game extension, `profileNNN.svg`). Any authenticated user could plant script that runs as any other user who opens the link. |
| 3 | High | **Every player received every profile, password included.** The page downloaded the whole save volume into each visitor's browser. Profile passwords are recoverable from those files (section 2). Reproduced: read `hunter2` from a downloaded `.acc` with one `zlib` decompress. |
| 4 | High | **No limits on resources.** `ThreadingHTTPServer` starts an unbounded thread per connection and sets no timeouts: 200 half-sent requests were all still held open after 20 s. Each `PUT` reads up to 16 MiB into memory, and nothing caps the number or total size of files written to the volume. |
| 5 | Medium | **Shared, writable game state.** All players shared one save folder ("like one PC"). Anyone could plant crafted save or high-score files that every other player's game then parses (C code compiled to WebAssembly). That corrupts or crashes other players' games and rewrites the shared high scores. It also includes `WarBlade.inf`, which two concurrent players overwrite. |
| 6 | Medium | **No security headers.** There was no `Content-Security-Policy`, `X-Content-Type-Options`, framing protection or `Referrer-Policy`, and the page used inline script and style. |
| 7 | Low | **Build hygiene.** A compiled `docker/__pycache__/server.cpython-311.pyc` was committed. The WebAssembly build cloned its dependencies by tag only, and a tag can be moved. |

Path handling in the old server (`safe_join`: `realpath` plus `commonpath`) held up. It
was not a finding, but the code is gone anyway.

**Were `PUT` and `DELETE` needed?** No. The game never talks to the server: it reads and
writes its user folder `/save` in the browser's virtual file system (`SysUserFolder`,
`src/core/sdl.c`). `PUT`/`DELETE` existed only so the page could mirror that folder onto the
server, which gave one shared save folder across browsers. The non-Docker browser build
already kept `/save` in IndexedDB. The new page does that in every mode, and the game runs
the same either way. Moving saves between devices is now an explicit export and import,
with no server storage.

## 2. Profile passwords

**These are not login credentials.** They lock a profile only against other people using
the same browser, and anyone holding the profile file can read them.

* **Storage:** `struct Account` keeps `char password[16]` at offset `0x25`
  (`include/types.h`, `struct Account`). `PackAccount` (`src/profile/account.c`) compresses
  the struct with plain `zlib compress()`, with no key and no hashing. `SaveAccount` writes it
  to `warblade/profiles/profileNNN.acc`, and backups go to `warblade/backup/profileNNN.acc`.
* **Check:** the login and change-password dialogs (`src/ui/menu.c`, "login submit:
  password check") decompress the profile and compare the typed text with the stored text,
  inside the game. The server never saw a password, and no server ever checked one. In the
  old image, a request to `/saves/...` needed no password at all.
* **Recovery:** decompress the `.acc` file and read the 15 bytes at `0x25`. The test
  fixtures (`tests/run.sh`) build a profile this way. Anyone with access to the following can
  read the passwords:
  * the browser (IndexedDB database `/save`, readable from the browser's developer tools or
    profile folder);
  * an exported `warblade-saves-*.tar`;
  * the old `/saves` volume and its backups;
  * under the old image, any visitor of the site (finding 3).
* **On the page:** the panel now says a profile password only keeps other people using
  this browser out of a profile, that it is stored readable, and that players shouldn't
  reuse a real password.

If the game ever needs real accounts (for example server-side saves), they must be separate
from these in-game passwords. See section 6.

## 3. Data flow now

```
                      GET/HEAD only                                     
 Browser  ───────────────────────────────▶  nginx (uid 101, read-only)  
   │       index.html page.js page.css      /app/web       (in the image, owned by root)
   │       warblade.js warblade.wasm        
   │       /data/  (JSON listings, files)   /data          (bind mount, :ro; optional)
   │                                         
   │       anything else: 405 (method) / 404 (path) / 413 / 414 / 429
   │
   ├─ IndexedDB "/save"  profiles, saves, settings, high scores (the game's /save/warblade)
   ├─ IndexedDB "/data"  only when the server has no data and the player chose a folder
   ├─ Export saves  ──▶ warblade-saves-<date>.tar, built in the page, saved by the browser
   └─ Import saves  ◀── a tar the player picks; only regular files under warblade/ are taken
```

* **To the server:** only `GET`/`HEAD` requests. It logs the client address, request line,
  status, size, user agent and time to stdout. Nothing a player does in the game reaches it.
* **Served:** the page and game from the image, and the data folder through `/data/`. That
  includes JSON directory listings (`autoindex`, which leaves out dot files), so the page can
  find the files. Data files always go out as `application/octet-stream`, and links in the
  data folder are not followed (`disable_symlinks`). The image has no route to `/saves`, even
  when the old volume is still mounted.
* **In the page** (`web/page.js`): it mounts IndexedDB at `/save` before the game starts and
  writes it out every 5 s, when the tab is hidden, on `pagehide` and when the game exits. It
  also asks for persistent storage (`navigator.storage.persist()`). Import rejects entries
  that climb out (`..`), absolute paths outside `warblade/`, links, devices and anything over
  64 MiB.

## 4. Migrating existing saves

The new image **does not read, serve, change or delete** the old `/saves` volume. The data
stays as it is until you remove it yourself.

1. **Back it up first**, while the old container still runs:

   ```sh
   docker run --rm -v warblade-saves:/saves:ro -v "$PWD":/backup alpine \
       tar -C /saves -czf /backup/warblade-saves-$(date +%F).tar.gz .
   ```

   Treat the backup as sensitive: it holds every player's recoverable profile password.

2. **Deploy the new image** (section 5). Remove the `/saves` mount from the container, but
   **keep the volume** (don't run `docker volume rm`). It is what a rollback uses.

3. **Hand the saves to the players.** Make a tar the page can import:

   ```sh
   docker run --rm -v warblade-saves:/saves:ro alpine tar -C /saves -cf - . > warblade-saves.tar
   ```

   Each player opens the game, chooses **Import saves** and picks the file. Imported files
   replace files of the same name in that browser only. The old volume was one shared "PC",
   so this tar has every profile and the shared settings and high scores. Give it only to
   the people who already had access (Authelia-authenticated players).

   For one player only, include just their profile number `NNN`:

   ```sh
   docker run --rm -v warblade-saves:/saves:ro alpine sh -c \
       'cd /saves && tar -cf - warblade/profiles/profileNNN.* warblade/backup/profileNNN.acc warblade/warblade_132.his 2>/dev/null' \
       > player-NNN.tar
   ```

   The shared high-score table `warblade_132.his` holds every player's names and scores.
   Leave it out if that matters.

4. **What changes for players:**
   * Server-mode players of the old image had their saves only on the server: the old page
     kept `/save` in memory. They start with no profiles until they import.
   * Players who used the page without server data (their own folder) keep everything: it
     is the same IndexedDB database `/save` as before.
   * Saves no longer follow a player between browsers or devices on their own. **Export
     saves** in one browser and **Import saves** in another to move them.

5. **When the migration is done** and you no longer need to roll back, delete the volume
   and its backups (`docker volume rm warblade-saves`). They hold readable passwords.

## 5. Deploying and rolling back

CI (`.github/workflows/docker.yml`) runs `tests/run.sh` on every push and pull request. It
publishes `ghcr.io/rkj/warbladesr` (`latest`, `main`, `sha-<commit>`) only from `main` and
only when the tests pass. Deploy a specific `sha-<commit>` tag or digest rather than
`latest`, so a rollback is exact.

**Record what runs now** before changing anything. Today that is
`ghcr.io/rkj/warbladesr@sha256:f6c16864f5c38bce1e0ec94c1f4521dda4cd39c0bc8d6fe1d0b4db607d43f76e`.

**Deploy:** keep Authelia and the reverse proxy route exactly as they are. Only the container
changes. A compose service for the homelab:

```yaml
services:
  warblade:
    image: ghcr.io/rkj/warbladesr:sha-<commit>
    read_only: true
    tmpfs:
      - /tmp:size=16m
    cap_drop: [ALL]
    security_opt: [no-new-privileges:true]
    pids_limit: 64
    mem_limit: 128m
    environment:
      # The reverse proxy's address(es), so the rate limits see real clients. Never a range
      # that clients can connect from directly.
      WARBLADE_TRUSTED_PROXIES: "172.18.0.0/16"
    volumes:
      - /path/to/Warblade/data:/data:ro   # optional: without it players use their own folder
      # no /saves: the image doesn't use it (keep the volume itself for rollback)
    restart: unless-stopped
```

* **Ports:** the container listens on `8080` (`PORT`). Publish it only to the reverse proxy,
  for example on a Docker network with no `ports:`.
* **HSTS:** TLS ends at the reverse proxy, so set `Strict-Transport-Security` there. Keep
  forwarding `X-Forwarded-For`.
* **Rate limits** are counted per client address. Without `WARBLADE_TRUSTED_PROXIES`, every
  client behind the proxy shares one budget: 20 requests/s with a burst of 400, 32
  simultaneous requests.

**Check it after deploying:**

```sh
curl -sI  https://<host>/ | grep -i content-security-policy              # headers present
curl -s -o /dev/null -w '%{http_code}\n' https://<host>/api/saves        # 404
curl -s -o /dev/null -w '%{http_code}\n' -X PUT -d x https://<host>/saves/a  # 405
```

Then open the game, play, reload, and check the profile is still there. Through Authelia,
these requests need its session cookie.

**Roll back:** point the service back at
`ghcr.io/rkj/warbladesr@sha256:f6c16864f5c38bce1e0ec94c1f4521dda4cd39c0bc8d6fe1d0b4db607d43f76e`,
restore the `- warblade-saves:/saves` volume line and remove `read_only`/`tmpfs`
(`docker compose up -d`). The old image finds the volume exactly as it was before the
switch. Two things to know:

* Saves made in browsers after the switch stay in those browsers. The old server mode
  doesn't read them, and players who want them back would need them copied into the volume
  by hand.
* Rolling back restores findings 1–6. Only roll back while Authelia still protects the
  route.

## 6. Remaining risks

* **Game data on a public site.** If Authelia is removed and `/data` is mounted, anyone can
  download the Warblade 1.34 data, which is copyrighted. That is a licensing decision rather
  than a technical one. Without the `/data` mount, the image serves only the page and the
  game, and players use their own copy.
* **Bandwidth.** One game load is about 3.7 MB of page and game plus the data folder. The
  per-address limits stop a single client's flood, but not a distributed one or one through
  an untrusted proxy. Put a CDN or the edge proxy's own limits in front for real public
  traffic.
* **Proxy configuration.** A wrong `WARBLADE_TRUSTED_PROXIES` either makes every client share
  one budget, so honest players get 429, or lets clients pick their own address through
  `X-Forwarded-For` (if a range they can connect from is trusted). The page waits and
  retries on 429 for about 40 s, so a shared budget slows game loads before it breaks them.
* **Saves live only in the browser.** Clearing site data, a private window or storage
  eviction loses them. The page asks for persistent storage and offers Export saves, but
  there is no server backup. Cross-device server saves are a separate design: player
  accounts that aren't the in-game passwords, per-player storage, size and rate quotas, and
  validation of uploaded files. That would need approval before anyone builds it.
* **Imported files are trusted by the game.** A tar from someone else can contain crafted
  save files. The game parses them in the WebAssembly sandbox of that player's own tab, so
  at worst that player's game misbehaves. It reaches no other player or the server.
* **Profile passwords** are recoverable wherever the files are (section 2).
* **Pinned by tag, not digest.** The base images (`emscripten/emsdk:4.0.23`,
  `nginxinc/nginx-unprivileged:1.30-alpine`) and the GitHub Actions are pinned only by tag.
  Digest pinning with Dependabot or Renovate would close that. `build-web.sh` now checks each
  library's commit.
* **Newer releases exist** for zlib (1.3.2) and SDL_image (3.4.8). Their inputs here are the
  operator's data and the player's own saves. Update them separately, with a play-through
  using real data.
* **Logs** hold client addresses and user agents. Set retention on the container logs.

## 7. Tests

`tests/run.sh` (details in `tests/README.md`) starts the image read-only, with no
capabilities, and checks the following.

* **Saves** (`test_server.py`), with an old-style save volume mounted at `/saves`:
  * an anonymous client can't list, read, create, overwrite or delete saves, through any of
    13 methods on 9 paths or through traversal;
  * the volume's files are byte-for-byte unchanged afterwards;
  * nginx's user can't write the site, the data, the config or `/saves`.
* **Delivery:** content types, revalidation, security headers on every response including
  errors, no inline code, and data files served only as plain bytes.
* **Paths:** traversal (encoded, double-encoded, backslashes, `..` past the root), dot
  files, links out of the data folder, NUL bytes, invalid escapes, and malformed or
  ambiguous requests all get a 4xx and never leak content.
* **Limits:** 413 for bodies, 400/414 for oversized headers and URIs, slow headers dropped
  after 10 s, idle keep-alive closed, 429 once the request burst or the concurrent-download
  limit is exceeded.
* **Browser** (`browser.test.cjs`, Chromium):
  * the page runs under the CSP with no violations;
  * it sends nothing but `GET`/`HEAD`;
  * a profile persists across reloads;
  * a second browser sees none of the first one's saves;
  * export and import move saves between browsers;
  * the old volume's tar imports and hostile tar entries are dropped;
  * a load that gets 429 waits and carries on;
  * the player's own-folder mode persists its data;
  * the real `warblade.wasm` compiles and starts under the CSP.

  These tests use a stand-in game (`tests/stub`) built with the same Emscripten options,
  because the repository has no Warblade data. A real play-through needs a Warblade 1.34
  install.
