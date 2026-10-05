// The browser build's page script (web/index.html; README.md, "Browser").
//
// The game sees two folders in its virtual file system:
//   /data   the Warblade data. From the server when it has some (the Docker image serves it
//           read-only under data/, with JSON directory listings), otherwise the player's own
//           Warblade folder, copied into this browser's IndexedDB once.
//   /save   the game's user folder (SysUserFolder): profiles, saves, settings, high scores.
//           Always kept in this browser's IndexedDB and never sent anywhere; Export saves and
//           Import saves move it between browsers as a tar file.
'use strict';
const $ = (id) => document.getElementById(id);
const DATA = '/data';   // the game looks for data/ in its own folder, / here
const SAVE = '/save';   // the game's user folder in the browser (SysUserFolder)

const MAX_DATA_FILES = 5000;               // a Warblade data folder has a few hundred
const MAX_DATA_DEPTH = 8;
const MAX_IMPORT_BYTES = 64 * 1024 * 1024; // a Warblade save folder is well under 1 MB

// Whether the server has the data (Docker image); otherwise the player chooses a folder.
let server = false;
// Whether IndexedDB works here; without it nothing outlives the page.
let persistent = true;

function setStatus(text) { $('status').textContent = text; }
function show(id, on) { $(id).hidden = !on; }

function sync(populate) {
  return new Promise((resolve, reject) =>
    FS.syncfs(populate, (err) => (err ? reject(err) : resolve())));
}

// Writes the IndexedDB-backed folders out, one FS.syncfs at a time; a request made while one
// runs makes it run again afterwards, so the latest files always get written.
let persisting = null, persistAgain = false;
function persist() {
  if (persisting) { persistAgain = true; return persisting; }
  persisting = (async () => {
    try {
      do { persistAgain = false; await sync(false); } while (persistAgain);
    } catch (e) {
      console.warn('Could not save to this browser', e);
    } finally {
      persisting = null;
    }
  })();
  return persisting;
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

function showSaveButtons(on) { show('export', on); show('import', on); }

function showReady() {
  let text = server ? 'Ready.' : 'Your Warblade data is loaded.';
  if (!persistent) text += ' This browser won\'t keep saves after you close the page.';
  setStatus(text);
  show('pick', false); show('play', true); show('forget', !server);
  showSaveButtons(true);
  $('play').focus();
}

function showPicker(message) {
  setStatus(message || 'Choose the folder you installed Warblade 1.34 in.');
  show('pick', true); show('play', false); show('forget', false);
  showSaveButtons(true);
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
    if (++done % 20 === 0) setStatus('Copying ' + done + ' / ' + wanted.length + ' files…');
  }
  setStatus('Saving to this browser…');
  await persist();
  showReady();
}

// ---- server data ----

const urlPath = (p) => p.split('/').map(encodeURIComponent).join('/');
const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

// fetch(), waiting and trying again while the server says to slow down (429: its per-client
// rate limit, which a game load can reach when many players share an address).
async function fetchPolitely(url, options) {
  for (let attempt = 0; ; attempt++) {
    const res = await fetch(url, options);
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
        setStatus('Loading game data ' + done + ' / ' + files.length + '…');
    }
  };
  await Promise.all(Array.from({ length: 6 }, worker));
}

// ---- saves export and import (tar files) ----
// The tar holds the game's warblade/ folder as it is under SAVE (and as it was under the old
// Docker image's /saves volume), so either can be imported.

const utf8 = new TextEncoder(), fromUtf8 = new TextDecoder();

function listSaves(dir = SAVE, out = []) {
  for (const name of FS.readdir(dir)) {
    if (name === '.' || name === '..') continue;
    const p = dir + '/' + name, st = FS.stat(p);
    if (FS.isDir(st.mode)) listSaves(p, out);
    else out.push({ path: p.slice(SAVE.length + 1), mtime: st.mtime.getTime() });
  }
  return out;
}

// A ustar header for a file, or null if the path doesn't fit.
function tarHeader(path, size, mtime) {
  let name = path, prefix = '';
  if (utf8.encode(name).length > 100) {
    name = null;
    for (let i = path.indexOf('/'); i !== -1; i = path.indexOf('/', i + 1)) {
      if (utf8.encode(path.slice(0, i)).length <= 155 && utf8.encode(path.slice(i + 1)).length <= 100) {
        prefix = path.slice(0, i); name = path.slice(i + 1);
        break;
      }
    }
    if (name === null) return null;
  }
  const h = new Uint8Array(512);
  const put = (off, len, text) => h.set(utf8.encode(text).subarray(0, len), off);
  const octal = (off, len, n) => put(off, len - 1, n.toString(8).padStart(len - 1, '0'));
  put(0, 100, name);
  octal(100, 8, 0o644);
  octal(108, 8, 0);
  octal(116, 8, 0);
  octal(124, 12, size);
  octal(136, 12, Math.max(0, Math.floor(mtime / 1000)));
  h.fill(32, 148, 156);           // the checksum counts its own field as spaces
  h[156] = 48;                    // '0': a regular file
  put(257, 6, 'ustar');
  put(263, 2, '00');
  put(345, 155, prefix);
  let sum = 0;
  for (const b of h) sum += b;
  put(148, 8, sum.toString(8).padStart(6, '0') + '\0 ');
  return h;
}

function exportSaves() {
  const parts = [];
  let count = 0;
  for (const f of listSaves()) {
    const data = FS.readFile(SAVE + '/' + f.path);
    const header = tarHeader(f.path, data.length, f.mtime);
    if (!header) { console.warn('Path too long for the export, skipped:', f.path); continue; }
    parts.push(header, data, new Uint8Array((512 - (data.length % 512)) % 512));
    count++;
  }
  if (!count) { setStatus('There are no saves in this browser yet.'); return; }
  parts.push(new Uint8Array(1024));
  const a = document.createElement('a');
  a.href = URL.createObjectURL(new Blob(parts, { type: 'application/x-tar' }));
  a.download = 'warblade-saves-' + new Date().toISOString().slice(0, 10) + '.tar';
  document.body.appendChild(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(a.href), 60000);
  setStatus('Exported ' + count + ' files.');
}

// The entries of a tar file: [{name, type, data}]. Understands ustar, GNU long names and pax
// paths; throws on anything that isn't a well-formed tar.
function readTar(bytes) {
  const str = (a, b) => {
    const s = bytes.subarray(a, b), z = s.indexOf(0);
    return fromUtf8.decode(z < 0 ? s : s.subarray(0, z));
  };
  const out = [];
  let off = 0, longName = null, paxName = null;
  while (off + 512 <= bytes.length) {
    const h = bytes.subarray(off, off + 512);
    if (h.every((b) => b === 0)) break;
    let sum = 0;
    for (let i = 0; i < 512; i++) sum += i >= 148 && i < 156 ? 32 : h[i];
    if (parseInt(str(off + 148, off + 156).trim(), 8) !== sum) throw new Error('not a tar file');
    const size = parseInt(str(off + 124, off + 136).trim() || '0', 8);
    if (!Number.isSafeInteger(size) || size < 0) throw new Error('not a tar file');
    const start = off + 512, end = start + size;
    if (end > bytes.length) throw new Error('the tar file is cut short');
    const type = h[156] === 0 ? '0' : String.fromCharCode(h[156]);
    let name = str(off, off + 100);
    if (str(off + 257, off + 262) === 'ustar') {
      const prefix = str(off + 345, off + 500);
      if (prefix) name = prefix + '/' + name;
    }
    const data = bytes.subarray(start, end);
    off = start + Math.ceil(size / 512) * 512;
    if (type === 'L') { longName = str(start, end); continue; }
    if (type === 'x') { paxName = paxPath(data); continue; }
    if (type === 'g') continue;
    if (paxName !== null) name = paxName; else if (longName !== null) name = longName;
    longName = paxName = null;
    out.push({ name, type, data });
  }
  return out;
}

// The path= record of a pax extended header, or null.
function paxPath(data) {
  let pos = 0, path = null;
  while (pos < data.length) {
    const sp = data.indexOf(32, pos);
    if (sp < 0) break;
    const len = parseInt(fromUtf8.decode(data.subarray(pos, sp)), 10);
    if (!(len > 0) || pos + len > data.length) break;
    const rec = fromUtf8.decode(data.subarray(sp + 1, pos + len - 1));
    if (rec.startsWith('path=')) path = rec.slice(5);
    pos += len;
  }
  return path;
}

// Where a tar entry goes under SAVE: inside its warblade/ folder, or null for anything else.
function savePath(name) {
  const parts = name.replace(/\\/g, '/').split('/').filter((p) => p !== '' && p !== '.');
  if (!parts.length || parts.some((p) => p === '..' || p.includes('\0'))) return null;
  if (parts.length < 2 || parts[0].toLowerCase() !== 'warblade') return null;
  parts[0] = 'warblade';
  return parts.join('/');
}

async function importSaves(file) {
  if (file.size > MAX_IMPORT_BYTES) { setStatus('That file is too big to be Warblade saves.'); return; }
  let entries;
  try {
    entries = readTar(new Uint8Array(await file.arrayBuffer()));
  } catch (e) {
    setStatus('Could not read that file: ' + e.message + '. Choose a .tar made by Export saves.');
    return;
  }
  const files = entries.filter((e) => e.type === '0' || e.type === '7')
    .map((e) => ({ path: savePath(e.name), data: e.data }))
    .filter((f) => f.path);
  if (!files.length) {
    setStatus('No Warblade saves in that file (they are in a warblade/ folder).');
    return;
  }
  if (!confirm('Import ' + files.length + ' files? Profiles, saves and settings with the same ' +
               'names in this browser are replaced.')) return;
  let done = 0, skipped = 0;
  for (const f of files) {
    try {
      const path = SAVE + '/' + f.path;
      mkdirs(path.slice(0, path.lastIndexOf('/')));
      FS.writeFile(path, f.data.slice());
      done++;
    } catch (e) {
      console.warn('Could not import', f.path, e);
      skipped++;
    }
  }
  await persist();
  setStatus('Imported ' + done + ' files' + (skipped ? ' (' + skipped + ' skipped)' : '') + '.');
}

// ---- the game ----

function start() {
  show('panel', false);
  FS.chdir('/');
  $('canvas').focus();
  // The game writes its files to /save; keep IndexedDB up to date. When the tab is hidden or
  // closed, have the game write its settings first (it otherwise does so only on quit).
  const save = () => {
    try { Module._WebSaveSettings(); } catch (e) { console.warn('WebSaveSettings', e); }
    persist();
  };
  setInterval(save, 5000);
  document.addEventListener('visibilitychange', () => { if (document.visibilityState === 'hidden') save(); });
  window.addEventListener('pagehide', save);
  Module.callMain([]);
}

var Module = {
  canvas: $('canvas'),
  print: (t) => console.log(t),
  printErr: (t) => console.warn(t),
  onExit: () => {
    persist();
    show('panel', true);
    setStatus('Warblade has closed. Reload the page to play again.');
    show('pick', false); show('play', false); show('forget', false);
    showSaveButtons(false);
  },
  onAbort: (what) => {
    show('panel', true);
    setStatus('Warblade stopped with an error: ' + what);
  },
  onRuntimeInitialized: async () => {
    const IDBFS = FS.filesystems.IDBFS;
    mkdirs(SAVE); FS.mount(IDBFS, {}, SAVE);
    mkdirs(DATA);
    const files = await listServerData().catch((err) => {
      setStatus('Could not load the game data: ' + err.message);
      throw err;
    });
    if (files) {
      server = true;
      $('intro').textContent = 'Warblade 1.34 by Edgar M. Vigdal, running in your browser, ' +
        'with the game data from this server.';
    } else {
      FS.mount(IDBFS, {}, DATA);
    }
    try { await sync(true); } catch (e) { persistent = false; console.warn('IndexedDB unavailable', e); }
    // The saves live only here: ask the browser not to clear them when it runs short of space.
    if (persistent && navigator.storage && navigator.storage.persist) navigator.storage.persist().catch(() => {});
    if (server) {
      try {
        await download(files);
      } catch (err) {
        setStatus('Could not load the game data: ' + err.message);
        return;
      }
      showReady();
    } else if (exists(DATA + '/warblade.pac')) {
      showReady();
    } else {
      showPicker();
    }
  },
};

$('canvas').addEventListener('contextmenu', (e) => e.preventDefault());
$('folder').addEventListener('change', (e) => {
  setStatus('Reading folder…');
  loadFolder(e.target.files).catch((err) => showPicker('Could not read that folder: ' + err));
});
$('play').addEventListener('click', start);
$('forget').addEventListener('click', async () => {
  removeTree(DATA);
  await persist();
  showPicker();
});
$('export').addEventListener('click', () => {
  try { exportSaves(); } catch (e) { setStatus('Could not export the saves: ' + e); }
});
$('import-file').addEventListener('change', (e) => {
  const file = e.target.files[0];
  e.target.value = '';
  if (file) importSaves(file).catch((err) => setStatus('Could not import the saves: ' + err));
});
