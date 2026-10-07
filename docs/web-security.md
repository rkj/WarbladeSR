# Browser deployment security

The old Python `http.server` image exposed one shared save volume through unauthenticated read/write routes. The previous agent's review reproduced arbitrary profile reads, overwrite and deletion, stored SVG script execution, and recoverable in-game passwords. The static nginx image removes the old save routes and serves mounted game data as downloads with restrictive headers. The account API introduced afterward provides private server-owned saves; it is reachable only through the same-origin reverse proxy.

## Current model

- The browser loads Warblade assets and runs the game in WebAssembly. If the operator does not mount the assets, a player can choose their own folder; all assets stay in memory for this visit, and the old asset database is removed.
- Players sign in once to their server-owned game account. Passwords must contain 8–256 characters. The API stores Argon2id password hashes and hashed session tokens. In `WARBLADE_SESSION_MODE=memory`, the page holds a bearer session token only in memory; no login cookie or browser storage is created. Legacy login cookies are expired. Cookie mode remains available for older API clients. Account registration and login have per-address limits and a total account cap.
- The game writes to an in-memory `/save` folder. The page sends changes to the API, which validates the signed-in account, origin, path, size and expected revision before committing a private file to SQLite. Another tab's edits cause a visible conflict instead of silent overwrite. `/save` is never mounted to IndexedDB.
- The game's six native top-20 high-score tables are shared. When a player qualifies, the page submits the changed compressed native table with its starting revision. The API extracts the new rows and merges them in a SQLite transaction, then returns a rebuilt table. Simultaneous players' rows therefore survive. All durable file and score writes happen in the API's database.
- The static image has no write route and no save volume. The API database is a separate private persistent volume; use an online SQLite backup to capture its WAL-consistent state. Keep the legacy save volume intact until its profiles have been assigned to accounts.

The browser creates or loads exactly one native profile per server account, adopting the server username and clearing its legacy profile password. The native login/create-player UI authenticates with the server; local profile selection and a second password are disabled in browser builds; existing multi-profile saves stop startup for explicit reconciliation rather than silently discarding progress. A player with an account can always inspect or alter their own game's in-memory state and submit invented scores; the shared board prevents lost updates, but score authenticity requires server-side gameplay or replay validation. Do not represent its rankings as cheat-proof.

## Reverse proxy

Route `/api/*` to the API container and all other paths to the nginx static container. Keep both upstreams private. Set `WARBLADE_PUBLIC_ORIGIN` to the exact browser origin; the API refuses cross-origin mutation requests. The proxy should overwrite `X-Real-IP`, never preserve a client-provided value. Configure `WARBLADE_TRUSTED_PROXY_IPS` only with exact peer addresses when the proxy address is stable; otherwise the API uses the socket peer for rate limiting. Preserve HTTPS and restrict body size and request rates at the public proxy as appropriate.

The API returns `Cache-Control: private, no-store` and downloaded saves have `application/octet-stream` with `nosniff`. The static image returns `Cache-Control: no-store`, retains a restrictive Content Security Policy and serves game data as octet streams, including SVG files, to prevent running them as same-origin script.

## Validation

`tests/test_api.py` checks account isolation, password hashing, origin/path restrictions, size quotas, revision races, rate limits and two simultaneous high-score submissions from the same board revision. `tests/test_server.py` checks the static image's HTTP boundary and headers. `tests/browser.test.cjs` checks that the browser requires a server account, uploads a game save through the API and never mounts `/save` in IndexedDB.

The read-only game assets load before authentication so the native login screen can render. The public game has no second identity gateway. Private saves and high-score APIs require the in-memory bearer session, and game launch is blocked until the native login succeeds.

Usernames are case-insensitive for both sign-in and registration uniqueness: `rkj`, `RkJ` and `RKJ` identify the same account and saved progress. The server returns the registered spelling; uppercase-only game fonts display its capital glyphs. Passwords remain case-sensitive.
