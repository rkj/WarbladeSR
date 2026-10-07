// The browser build's page script (web/index.html; README.md, "Browser").
//
// The game sees two folders in its virtual file system:
//   /data   the Warblade data. From the server when it has some (the Docker image serves it
//           read-only under data/, with JSON directory listings), otherwise the player's own
//           Warblade folder, copied into memory for this visit only.
//   /save   the game's in-memory user folder. Its durable copy belongs to the authenticated
//           server account, not to browser storage. The server controls file ownership.
'use strict';
const $ = (id) => document.getElementById(id);
const DATA = '/data';   // the game looks for data/ in its own folder, / here
const SAVE = '/save';   // the game's user folder in the browser (SysUserFolder)
const SCORE_FILE = SAVE + '/warblade/warblade_132.his';

const MAX_DATA_FILES = 5000;               // a Warblade data folder has a few hundred
const MAX_DATA_DEPTH = 8;

// Whether the server has the data (Docker image); otherwise the player chooses a folder.
let server = false;
// All game data and credentials remain in memory for this visit.
let runtimeReady = false;
let accountReady = false;
let saveConflict = false;
let saveBusy = null;
let saveAgain = false;
const saved = new Map();
let scoreBase = null;
let scoreVersion = null;
let scoreReloadPending = false;
let lastScorePoll = 0;

function setStatus(text, completed, total) {
  $('status').textContent = text;
  if (total) { $('progress').max = total; $('progress').value = completed; }
  else $('progress').removeAttribute('value');
}
function show(id, on) { $(id).hidden = !on; }

// The session token and virtual filesystem exist only in this page's memory.
let sessionToken = '';
let accountName = '';
async function accountFetch(url, options = {}) {
  const headers = new Headers(options.headers || {});
  if (sessionToken) headers.set('Authorization', 'Bearer ' + sessionToken);
  return fetch(url, { ...options, headers, credentials: 'omit', cache: 'no-store' });
}

// Remove the old optional asset cache; new code never creates browser databases.
async function forgetLegacyAssets() {
  try {
    await new Promise((resolve) => {
      const request = indexedDB.deleteDatabase('/data');
      request.onsuccess = request.onerror = request.onblocked = resolve;
    });
  } catch (_) { /* Storage may be disabled; this build does not need it. */ }
}

function exists(path) {
  try { FS.stat(path); return true; } catch (e) { return false; }
}

function mkdirs(path) {
  let cur = '';
  for (const part of path.split('/').filter(Boolean)) {
    cur += '/' + part;
    if (!exists(cur)) FS.mkdir(cur);
  }
}

function removeTree(path) {
  for (const name of FS.readdir(path)) {
    if (name === '.' || name === '..') continue;
    const p = path + '/' + name;
    if (FS.isDir(FS.stat(p).mode)) { removeTree(p); FS.rmdir(p); } else FS.unlink(p);
  }
}

function showReady() {
  show('pick', false);
  setStatus('Starting…');
  start();
}

function showPicker(message) {
  setStatus(message || 'Choose your Warblade folder.');
  show('pick', true);
}

function startupError(message) {
  show('loading', true);
  setStatus(message);
  show('progress', false);
  show('retry', true);
}

// Copies the chosen folder's data/ (the files next to warblade.pac) into DATA.
async function loadFolder(files) {
  const list = Array.from(files);
  const pac = list.find((f) => f.name.toLowerCase() === 'warblade.pac');
  if (!pac) {
    showPicker('No warblade.pac in that folder. Choose your Warblade 1.34 folder or its data folder.');
    return;
  }
  const rel = pac.webkitRelativePath || pac.name;
  const prefix = rel.slice(0, rel.length - pac.name.length);
  const wanted = list.filter((f) => (f.webkitRelativePath || f.name).startsWith(prefix));
  if (exists(DATA)) removeTree(DATA);
  let done = 0;
  for (const f of wanted) {
    const sub = (f.webkitRelativePath || f.name).slice(prefix.length);
    const dest = DATA + '/' + sub;
    mkdirs(dest.slice(0, dest.lastIndexOf('/')));
    FS.writeFile(dest, new Uint8Array(await f.arrayBuffer()));
    if (++done % 20 === 0) setStatus('Loading…', done, wanted.length);
  }
  showReady();
}

// ---- server data ----

const urlPath = (p) => p.split('/').map(encodeURIComponent).join('/');
const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

// fetch(), waiting and trying again while the server says to slow down (429: its per-client
// rate limit, which a game load can reach when many players share an address).
async function fetchPolitely(url, options) {
  for (let attempt = 0; ; attempt++) {
    const res = await accountFetch(url, options);
    if (res.status !== 429 || attempt === 8) return res;
    await sleep(500 * 2 ** Math.min(attempt, 4));
  }
}
const validName = (n) => n !== '' && n !== '.' && n !== '..' && !/[\/\\\0]/.test(n);

// The server's data files as [{path, size}], walking its JSON directory listings, or null
// when it has none (no data mounted, or a plain static server without listings).
async function listServerData() {
  const files = [];
  const dirs = [{ path: '', depth: 0 }];
  while (dirs.length) {
    const dir = dirs.shift();
    let entries = null;
    try {
      const res = await fetchPolitely('data/' + urlPath(dir.path), { cache: 'no-cache' });
      const type = res.headers.get('Content-Type') || '';
      if (res.ok && type.includes('json')) entries = await res.json();
    } catch (e) {
      entries = null;
    }
    if (!Array.isArray(entries)) {
      if (dir.path === '') return null;
      console.warn('Could not list data/' + dir.path + ', skipped');   // e.g. a link the server won't follow
      continue;
    }
    for (const e of entries) {
      if (!e || typeof e.name !== 'string' || !validName(e.name)) continue;
      if (e.type === 'directory' && dir.depth < MAX_DATA_DEPTH)
        dirs.push({ path: dir.path + e.name + '/', depth: dir.depth + 1 });
      else if (e.type === 'file')
        files.push({ path: dir.path + e.name, size: e.size });
      if (files.length > MAX_DATA_FILES) throw new Error('too many files in data/');
    }
  }
  return files.some((f) => f.path.toLowerCase() === 'warblade.pac') ? files : null;
}

// Downloads `files` (paths relative to data/) into DATA, a few at a time.
async function download(files) {
  let next = 0, done = 0;
  const worker = async () => {
    while (next < files.length) {
      const f = files[next++];
      const res = await fetchPolitely('data/' + urlPath(f.path));
      if (res.status === 403 || res.status === 404) {
        console.warn('data/' + f.path + ': HTTP ' + res.status + ', skipped');   // e.g. a link
      } else if (!res.ok) {
        throw new Error(f.path + ': HTTP ' + res.status);
      } else {
        const path = DATA + '/' + f.path;
        mkdirs(path.slice(0, path.lastIndexOf('/')));
        FS.writeFile(path, new Uint8Array(await res.arrayBuffer()));
      }
      if (++done % 10 === 0 || done === files.length)
        setStatus('Loading…', done, files.length);
    }
  };
  await Promise.all(Array.from({ length: 6 }, worker));
}

// ---- authenticated, server-owned saves ----

function saveNotice(message) {
  $('save-alert').textContent = message;
  $('save-alert').hidden = !message;
}

function saveFiles(dir = SAVE, out = []) {
  for (const name of FS.readdir(dir)) {
    if (name === '.' || name === '..') continue;
    const path = dir + '/' + name;
    if (FS.isDir(FS.stat(path).mode)) saveFiles(path, out);
    else if (path !== SCORE_FILE) out.push(path.slice(SAVE.length + 1));
  }
  return out;
}

function equalBytes(a, b) {
  return a.length === b.length && a.every((byte, i) => byte === b[i]);
}

async function loadSaves() {
  const response = await accountFetch('/api/saves', { cache: 'no-store' });
  if (!response.ok) throw new Error('save list: HTTP ' + response.status);
  const manifest = await response.json();
  saved.clear();
  for (const file of manifest.files) {
    if (file.path === 'warblade/warblade_132.his') continue;
    if (typeof file.path !== 'string' || !file.path.startsWith('warblade/') ||
        file.path.split('/').some((part) => !validName(part)))
      throw new Error('invalid save path from server');
    const item = await accountFetch('/api/saves/' + urlPath(file.path), { cache: 'no-store' });
    if (!item.ok) throw new Error('save download: HTTP ' + item.status);
    const bytes = new Uint8Array(await item.arrayBuffer());
    const path = SAVE + '/' + file.path;
    mkdirs(path.slice(0, path.lastIndexOf('/')));
    FS.writeFile(path, bytes);
    saved.set(file.path, { bytes, version: file.version });
  }
  accountReady = true;
  saveConflict = false;
  saveNotice('');
}

async function loadScores() {
  const response = await accountFetch('/api/hiscores', { cache: 'no-store' });
  if (!response.ok) throw new Error('high scores: HTTP ' + response.status);
  scoreVersion = response.headers.get('ETag')?.replaceAll('"', '');
  if (!/^\d+$/.test(scoreVersion || '')) throw new Error('invalid high-score revision');
  scoreBase = new Uint8Array(await response.arrayBuffer());
  mkdirs(SCORE_FILE.slice(0, SCORE_FILE.lastIndexOf('/')));
  FS.writeFile(SCORE_FILE, scoreBase);
}

function reloadScores() {
  if (!scoreReloadPending) return;
  try { scoreReloadPending = Module._WebReloadHiscores() !== 1; }
  catch (err) { console.warn('High-score refresh', err); }
}

async function refreshScores() {
  if (!scoreBase || !equalBytes(FS.readFile(SCORE_FILE), scoreBase)) return;
  if (Date.now() - lastScorePoll < 15000) { reloadScores(); return; }
  lastScorePoll = Date.now();
  const response = await accountFetch('/api/hiscores', { cache: 'no-store' });
  if (!response.ok) throw new Error('High-score refresh failed: HTTP ' + response.status);
  const version = response.headers.get('ETag')?.replaceAll('"', '');
  if (version !== scoreVersion) {
    scoreVersion = version;
    scoreBase = new Uint8Array(await response.arrayBuffer());
    FS.writeFile(SCORE_FILE, scoreBase);
    scoreReloadPending = true;
  }
  reloadScores();
}

// The server compares the candidate with the exact board revision this tab loaded and
// transactionally merges only new entries. A stale tab therefore cannot erase another score.
async function flushScores() {
  if (!scoreBase || !exists(SCORE_FILE)) return;
  const candidate = FS.readFile(SCORE_FILE);
  if (equalBytes(candidate, scoreBase)) return;
  const response = await accountFetch('/api/hiscores', {
    method: 'POST', credentials: 'same-origin',
    headers: { 'Content-Type': 'application/octet-stream', 'If-Match': scoreVersion },
    body: candidate,
  });
  if (!response.ok) throw new Error('High-score sync failed (HTTP ' + response.status + '). Reload before playing again.');
  scoreVersion = response.headers.get('ETag')?.replaceAll('"', '');
  scoreBase = new Uint8Array(await response.arrayBuffer());
  FS.writeFile(SCORE_FILE, scoreBase);
  scoreReloadPending = true;
  reloadScores();
}

// The browser reports changed game files, but only the authenticated server writes durable
// data. Every update carries its last version, so another tab/device cannot be overwritten.
async function flushSaves(keepalive = false) {
  if (!accountReady || saveConflict) return;
  const paths = new Set(saveFiles());
  for (const path of paths) {
    const bytes = FS.readFile(SAVE + '/' + path);
    const previous = saved.get(path);
    if (previous && equalBytes(bytes, previous.bytes)) continue;
    const response = await accountFetch('/api/saves/' + urlPath(path), {
      method: 'PUT', credentials: 'same-origin', keepalive: keepalive && bytes.length <= 60000,
      headers: { 'Content-Type': 'application/octet-stream', 'If-Match': previous ? String(previous.version) : '*' },
      body: bytes,
    });
    if (response.status === 409 || response.status === 412) {
      saveConflict = true;
      throw new Error('Your saves changed in another tab or device. Reload to load the newer copy; this tab will not overwrite it.');
    }
    if (!response.ok) throw new Error('save failed: HTTP ' + response.status);
    const result = await response.json();
    saved.set(path, { bytes: bytes.slice(), version: result.version });
  }
  for (const [path, previous] of saved) {
    if (paths.has(path)) continue;
    const response = await accountFetch('/api/saves/' + urlPath(path), {
      method: 'DELETE', credentials: 'same-origin', keepalive,
      headers: { 'If-Match': String(previous.version) },
    });
    if (response.status === 409 || response.status === 412) {
      saveConflict = true;
      throw new Error('Your saves changed in another tab or device. Reload to load the newer copy; this tab will not overwrite it.');
    }
    if (!response.ok) throw new Error('save deletion failed: HTTP ' + response.status);
    saved.delete(path);
  }
  saveNotice('');
}

function persistSaves(keepalive = false) {
  if (saveBusy) { saveAgain = true; return saveBusy; }
  saveBusy = (async () => {
    try {
      do { saveAgain = false; await flushScores(); await flushSaves(keepalive); await refreshScores(); } while (saveAgain);
    } catch (err) {
      saveNotice(String(err.message || err));
    } finally { saveBusy = null; }
  })();
  return saveBusy;
}

async function loadGameData() {
  const files = await listServerData();
  if (files) {
    server = true;
    await download(files);
    showReady();
  } else {
    mkdirs(DATA);
    showPicker('Choose your Warblade data folder for this visit. It will stay in memory only.');
  }
}

let authBusy = false;

function accountFailure(status, create) {
  if (status === 400) return 'Check your username and password requirements.';
  if (status === 401) return 'Incorrect username or password. Try again.';
  if (status === 409) return 'Username already taken. Choose another or sign in.';
  if (status === 429) return 'Too many attempts. Wait 15 minutes and try again.';
  if (status === 403) return create ? 'Account creation is currently unavailable.' : 'Sign-in is currently unavailable.';
  return 'Account service unavailable. Please try again shortly.';
}

// Called by the web game's native login window while its Asyncify loop yields.
// The standalone executable continues to authenticate its local profiles itself.
async function authenticateGame(username, password, create) {
  if (authBusy || accountReady) return false;
  Module.authError = '';
  username = username.trim();
  if (!/^[A-Za-z0-9][A-Za-z0-9_.-]{2,31}$/.test(username)) {
    Module.authError = 'Username: 3-32 letters/numbers, dots, underscores or hyphens; start with a letter or number.';
    return false;
  }
  if ([...password].length < 12 || [...password].length > 256) {
    Module.authError = 'Password must have 12-256 characters.';
    return false;
  }
  authBusy = true;
  try {
    const response = await accountFetch('/api/' + (create ? 'register' : 'login'), {
      method: 'POST', headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ username, password }),
    });
    if (!response.ok || response.redirected) {
      Module.authError = accountFailure(response.status, create);
      return false;
    }
    const result = await response.json();
    if (typeof result.token !== 'string' || !/^[A-Za-z0-9_-]{43}$/.test(result.token) ||
        typeof result.username !== 'string') {
      Module.authError = 'Invalid response from the account service. Try again.';
      return false;
    }
    sessionToken = result.token;
    accountName = result.username;
    // Discard unauthenticated temporary settings before loading private saves.
    removeTree(SAVE);
    saved.clear();
    await loadSaves();
    await loadScores();
    Module.accountName = accountName;
    return true;
  } catch (_) {
    Module.authError = 'Cannot load your account. Check your connection and try again.';
    sessionToken = '';
    accountReady = false;
    scoreBase = null;
    scoreVersion = null;
    return false;
  } finally { authBusy = false; }
}

async function signOutGame() {
  await persistSaves();
  if (saveConflict || !$('save-alert').hidden) {
    Module.authError = 'Your progress could not be saved. Try again before signing out.';
    return false;
  }
  try {
    const response = await accountFetch('/api/logout', { method: 'POST' });
    if (!response.ok) {
      Module.authError = 'Could not sign out. Check your connection and try again.';
      return false;
    }
    sessionToken = '';
    accountReady = false;
    location.reload();
    return true;
  } catch (_) {
    Module.authError = 'Could not sign out. Check your connection and try again.';
    return false;
  }
}

// ---- touch controls: the stick and buttons set bits of the game's pad 0 ----

const PAD = { left: 1, right: 2, up: 4, down: 8, fire: 16, rocket: 32, pause: 64 };
let padBits = 0, stickBits = 0;
const buttonBits = new Map();   // pointerId -> bit held on a button

function sendPad() {
  let bits = stickBits;
  for (const b of buttonBits.values()) bits |= b;
  if (bits !== padBits) {
    padBits = bits;
    Module._SysSetVirtualPad(bits);
  }
}

function setupTouch() {
  const stick = $('stick'), knob = $('knob');
  let stickPointer = null;
  const moveStick = (e) => {
    const r = stick.getBoundingClientRect();
    const radius = r.width / 2;
    let dx = e.clientX - (r.left + radius), dy = e.clientY - (r.top + radius);
    const len = Math.hypot(dx, dy), max = radius - 30;
    if (len > max) { dx *= max / len; dy *= max / len; }
    knob.style.transform = `translate(${dx}px, ${dy}px)`;
    const dead = radius * 0.3;
    stickBits = (dx < -dead ? PAD.left : 0) | (dx > dead ? PAD.right : 0) |
                (dy < -dead ? PAD.up : 0) | (dy > dead ? PAD.down : 0);
    sendPad();
  };
  const releaseStick = () => {
    stickPointer = null;
    knob.style.transform = '';
    stickBits = 0;
    sendPad();
  };
  stick.addEventListener('pointerdown', (e) => {
    e.preventDefault();
    stickPointer = e.pointerId;
    stick.setPointerCapture(e.pointerId);
    moveStick(e);
  });
  stick.addEventListener('pointermove', (e) => { if (e.pointerId === stickPointer) moveStick(e); });
  stick.addEventListener('pointerup', releaseStick);
  stick.addEventListener('pointercancel', releaseStick);

  for (const [id, bit] of [['fire', PAD.fire], ['rocket', PAD.rocket], ['pause', PAD.pause]]) {
    const el = $(id);
    const release = (e) => { buttonBits.delete(e.pointerId); el.classList.remove('down'); sendPad(); };
    el.addEventListener('pointerdown', (e) => {
      e.preventDefault();
      el.setPointerCapture(e.pointerId);
      buttonBits.set(e.pointerId, bit);
      el.classList.add('down');
      sendPad();
    });
    el.addEventListener('pointerup', release);
    el.addEventListener('pointercancel', release);
  }
}

// Shown on touch screens: right away where the main pointer is a finger, else on the first touch.
function enableTouch() {
  if (!$('touch').hidden) return;
  setupTouch();
  show('touch', true);
}

// ---- the game ----

let gameStarted = false;
function start() {
  if (gameStarted) return;
  gameStarted = true;
  FS.chdir('/');
  $('canvas').focus();
  // The game writes to its in-memory /save folder. Flush to the server regularly and when
  // the tab is hidden; the browser never owns a durable save or high-score copy.
  const save = () => {
    try { Module._WebSaveSettings(); } catch (e) { console.warn('WebSaveSettings', e); }
    persistSaves();
  };
  setInterval(save, 5000);
  document.addEventListener('visibilitychange', () => { if (document.visibilityState === 'hidden') { save(); persistSaves(true); } });
  window.addEventListener('pagehide', () => { try { Module._WebSaveSettings(); } catch (_) {} persistSaves(true); });
  Module.callMain([]);
  // Hand off once native initialization finishes, independently of account
  // window focus or authentication activity.
  const revealGame = () => {
    if (Module._WebGameReady && Module._WebGameReady()) {
      show('loading', false);
      if (matchMedia('(pointer: coarse)').matches) enableTouch();
      window.addEventListener('touchstart', enableTouch, { passive: true });
      $('canvas').focus();
    } else requestAnimationFrame(revealGame);
  };
  requestAnimationFrame(revealGame);
}

var Module = {
  canvas: $('canvas'),
  authenticateGame, signOutGame,
  accountName: '', authError: '',
  onGameScoreWritten: () => queueMicrotask(() => persistSaves()),
  print: (t) => console.log(t),
  printErr: (t) => console.warn(t),
  onExit: () => {
    persistSaves();
    startupError('Warblade has closed.');
    show('pick', false);
  },
  onAbort: (what) => {
    startupError('Warblade stopped: ' + what);
  },
  onRuntimeInitialized: async () => {
    runtimeReady = true;
    mkdirs(SAVE);
    await forgetLegacyAssets();
    try { await loadGameData(); }
    catch (err) { startupError('Could not load Warblade. ' + err.message); }

  },
};

// The native game historically maps text to uppercase. Account passwords need
// actual browser text, including case, punctuation and paste.
function insertLoginText(text) {
  if (!runtimeReady || !Module._WebLoginReady || !Module._WebLoginReady()) return false;
  return !!Module.ccall('WebInsertLoginText', 'number', ['string'], [text]);
}
document.addEventListener('keydown', event => {
  if (!event.ctrlKey && !event.metaKey && !event.altKey && event.key.length === 1
      && insertLoginText(event.key)) event.preventDefault();
});
document.addEventListener('paste', event => {
  if (insertLoginText(event.clipboardData.getData('text').replace(/[\r\n]/g, '')))
    event.preventDefault();
});

$('canvas').addEventListener('contextmenu', (e) => e.preventDefault());
$('folder').addEventListener('change', (e) => {
  setStatus('Reading folder…');
  loadFolder(e.target.files).catch((err) => showPicker('Could not read that folder: ' + err));
});
$('retry').addEventListener('click', () => location.reload());
$('loading-art').addEventListener('error', () => show('loading-art', false));
