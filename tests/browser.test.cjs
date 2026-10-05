// Browser tests for the page (web/page.js) as the Docker image serves it (tests/README.md).
// Run by tests/run.sh: node --test tests/browser.test.cjs, with Playwright's Chromium.
//
// Settings come from the environment:
//   WARBLADE_STUB_URL    the image serving the stand-in game (tests/stub) with the test data
//   WARBLADE_LOCAL_URL   the same without data: players choose their own folder
//   WARBLADE_URL         the image as built, with the real warblade.js/.wasm and the test data
//   WARBLADE_FIXTURES    tests/run.sh's fixtures folder (localdata/, legacy.tar, hostile.tar)
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { chromium } = require('playwright');

const STUB_URL = process.env.WARBLADE_STUB_URL;
const LOCAL_URL = process.env.WARBLADE_LOCAL_URL;
const REAL_URL = process.env.WARBLADE_URL;
const FIXTURES = process.env.WARBLADE_FIXTURES;
const PROFILE = '/save/warblade/profiles/profile000.acc';

let browser;
test.before(async () => { browser = await chromium.launch(); });
test.after(async () => { await browser.close(); });

// A page that records what it sends, its errors and any Content-Security-Policy violation.
async function open(context, url) {
  const page = await context.newPage();
  page.requests = [];
  page.failed = [];
  page.errors = [];
  page.logs = [];
  page.on('request', (r) => page.requests.push(r.method() + ' ' + r.url()));
  page.on('response', (r) => { if (r.status() >= 400) page.failed.push(r.url()); });
  page.on('pageerror', (e) => page.errors.push(String(e)));
  page.on('console', (m) => {
    page.logs.push(m.text());
    // Failed requests are checked one by one (checkClean).
    if (m.type() === 'error' && !m.text().startsWith('Failed to load resource')) page.errors.push(m.text());
  });
  page.on('dialog', (d) => d.accept());
  await page.addInitScript(() => {
    window.__csp = [];
    document.addEventListener('securitypolicyviolation',
      (e) => window.__csp.push(e.violatedDirective + ' ' + e.blockedURI));
  });
  await page.goto(url);
  return page;
}

async function waitStatus(page, re) {
  await page.waitForFunction((src) => new RegExp(src).test(document.getElementById('status').textContent),
    re.source, { timeout: 30000 });
}

// Clicks Play and returns the stand-in game's "STUB ..." line.
async function play(page) {
  const line = page.waitForEvent('console', { predicate: (m) => m.text().startsWith('STUB '), timeout: 20000 });
  await page.click('#play');
  return (await line).text();
}

// The files in an IDBFS database ('/save' or '/data'): {path: text}.
function readIdb(page, db) {
  return page.evaluate((name) => new Promise((resolve, reject) => {
    const req = indexedDB.open(name);
    req.onerror = () => reject(req.error);
    req.onsuccess = () => {
      const idb = req.result;
      if (!idb.objectStoreNames.contains('FILE_DATA')) { idb.close(); resolve({}); return; }
      const out = {};
      const cursor = idb.transaction('FILE_DATA').objectStore('FILE_DATA').openCursor();
      cursor.onsuccess = () => {
        const c = cursor.result;
        if (!c) { idb.close(); resolve(out); return; }
        if (c.value.contents) out[c.key] = new TextDecoder().decode(c.value.contents);
        c.continue();
      };
      cursor.onerror = () => reject(cursor.error);
    };
  }), db);
}

async function waitIdb(page, db, file, text) {
  const deadline = Date.now() + 20000;
  while (Date.now() < deadline) {
    if ((await readIdb(page, db))[file] === text) return;
    await page.waitForTimeout(250);
  }
  assert.fail(`${file} never became ${JSON.stringify(text)} in IndexedDB ${db}`);
}

// No CSP violation, no error, no request failing but those matching `expectedFailures`, and
// nothing sent to the server but reads.
async function checkClean(page, expectedFailures = /^$/) {
  assert.deepEqual(await page.evaluate(() => window.__csp), [], 'CSP violations');
  assert.deepEqual(page.errors, [], 'page errors');
  assert.deepEqual(page.failed.filter((u) => !expectedFailures.test(u)), [], 'failed requests');
  // Nothing but reads, and nothing about saves, ever goes to the server.
  for (const r of page.requests) {
    assert.match(r, /^(GET|HEAD) /, r);
    assert.doesNotMatch(r, /saves|\/api\//, r);
  }
}

// A minimal tar reader for the exported file: {path: text}.
function readTar(buf) {
  const out = {};
  for (let off = 0; off + 512 <= buf.length;) {
    const h = buf.subarray(off, off + 512);
    if (h.every((b) => b === 0)) break;
    const str = (a, b) => h.subarray(a, b).toString().replace(/\0.*$/s, '');
    const size = parseInt(str(124, 136), 8);
    const name = (str(345, 500) ? str(345, 500) + '/' : '') + str(0, 100);
    out[name] = buf.subarray(off + 512, off + 512 + size).toString();
    off += 512 + Math.ceil(size / 512) * 512;
  }
  return out;
}

async function importTar(page, file) {
  await page.setInputFiles('#import-file', file);
  await waitStatus(page, /^(Imported|No Warblade|Could not)/);
  return page.textContent('#status');
}

test('server data: saves persist across reloads, stay in each browser, move by export/import', async () => {
  const alice = await browser.newContext();
  const bob = await browser.newContext();
  try {
    // Alice plays: the game gets the server's data and keeps its files in her browser.
    const a = await open(alice, STUB_URL);
    await waitStatus(a, /^Ready\./);
    assert.equal(await play(a), 'STUB pac=FAKE-PAC runs=1');
    await waitIdb(a, '/save', PROFILE, '1\n');
    await waitIdb(a, '/save', '/save/warblade/WarBlade.inf', 'settings 1\n');
    await checkClean(a);

    // She comes back later: her profile is still there.
    await a.reload();
    await waitStatus(a, /^Ready\./);
    assert.equal(await play(a), 'STUB pac=FAKE-PAC runs=2');
    await waitIdb(a, '/save', PROFILE, '2\n');
    await checkClean(a);

    // Bob, on another browser of the same server, sees none of it.
    const b = await open(bob, STUB_URL);
    await waitStatus(b, /^Ready\./);
    assert.deepEqual(await readIdb(b, '/save'), {});
    assert.equal(await play(b), 'STUB pac=FAKE-PAC runs=1');
    await checkClean(b);

    // Alice exports her saves; Bob's browser takes them (her other device, say).
    await a.reload();
    await waitStatus(a, /^Ready\./);
    const download = a.waitForEvent('download');
    await a.click('#export');
    const file = await (await download).path();
    const tar = readTar(fs.readFileSync(file));
    assert.equal(tar['warblade/profiles/profile000.acc'], '2\n');
    assert.ok('warblade/WarBlade.inf' in tar);

    await b.reload();
    await waitStatus(b, /^Ready\./);
    assert.match(await importTar(b, file), /^Imported 2 files\.$/);
    assert.equal((await readIdb(b, '/save'))[PROFILE], '2\n');
    await b.reload();
    await waitStatus(b, /^Ready\./);
    assert.equal(await play(b), 'STUB pac=FAKE-PAC runs=3');
    await checkClean(a);
    await checkClean(b);
  } finally {
    await alice.close();
    await bob.close();
  }
});

test('import: the old server volume, and nothing outside warblade/', async () => {
  const ctx = await browser.newContext();
  try {
    const page = await open(ctx, STUB_URL);
    await waitStatus(page, /^Ready\./);

    // tar -C <volume> -cf legacy.tar . of the old image's /saves volume (README.md).
    const legacy = fs.readFileSync(path.join(FIXTURES, 'saves/warblade/profiles/profile000.acc'));
    assert.match(await importTar(page, path.join(FIXTURES, 'legacy.tar')), /^Imported 2 files\.$/);
    const saves = await page.evaluate((p) => Array.from(FS.readFile(p)), PROFILE);
    assert.deepEqual(Buffer.from(saves), legacy);

    // Entries that climb out, absolute paths, other folders and links are left out.
    assert.match(await importTar(page, path.join(FIXTURES, 'hostile.tar')), /^Imported 1 files\.$/);
    const keys = Object.keys(await readIdb(page, '/save')).sort();
    assert.deepEqual(keys, ['/save/warblade/WarBlade.inf', PROFILE, '/save/warblade/ok.txt'].sort());
    const everything = await page.evaluate(() => {
      const out = [];
      const walk = (d) => {
        for (const n of FS.readdir(d)) {
          if (n === '.' || n === '..' || (d === '/' && ['dev', 'proc', 'tmp'].includes(n))) continue;
          const p = (d === '/' ? '' : d) + '/' + n;
          if (FS.isDir(FS.stat(p).mode)) walk(p); else out.push(p);
        }
      };
      walk('/');
      return out;
    });
    assert.deepEqual(everything.filter((p) => /evil/.test(p)), []);

    // Not a tar file at all.
    const junk = path.join(FIXTURES, 'junk.tar');
    fs.writeFileSync(junk, 'this is not a tar file'.repeat(40));
    assert.match(await importTar(page, junk), /^Could not read that file: not a tar file/);
    await checkClean(page);
  } finally {
    await ctx.close();
  }
});

test('no server data: the player\'s own folder, kept in the browser', async () => {
  const ctx = await browser.newContext();
  try {
    const page = await open(ctx, LOCAL_URL);
    await waitStatus(page, /^Choose the folder/);
    await page.setInputFiles('#folder', path.join(FIXTURES, 'localdata'));
    await waitStatus(page, /^Your Warblade data is loaded\./);

    await page.reload();
    await waitStatus(page, /^Your Warblade data is loaded\./);   // no need to choose it again
    assert.equal(await play(page), 'STUB pac=LOCAL-PAC runs=1');
    await waitIdb(page, '/save', PROFILE, '1\n');
    await page.reload();
    await waitStatus(page, /^Your Warblade data is loaded\./);
    assert.equal(await play(page), 'STUB pac=LOCAL-PAC runs=2');
    await checkClean(page, /\/data\/$/);      // the server has no data: the page asks for a folder
  } finally {
    await ctx.close();
  }
});

test('a rate-limited data load waits and carries on', async () => {
  const ctx = await browser.newContext();
  try {
    // The server's 429 for the first two tries at a file, as when the per-client budget is spent.
    let refused = 0;
    await ctx.route('**/data/music/title.xm', (route) =>
      refused++ < 2 ? route.fulfill({ status: 429, body: 'slow down' }) : route.continue());
    const page = await open(ctx, STUB_URL);
    await waitStatus(page, /^Ready\./);
    assert.equal(refused, 3);
    assert.equal(await page.evaluate(() => FS.readFile('/data/music/title.xm', { encoding: 'utf8' })), 'music');
    await checkClean(page, /\/data\/music\/title\.xm$/);
  } finally {
    await ctx.close();
  }
});

test('the real game build loads under the Content-Security-Policy', async () => {
  const ctx = await browser.newContext();
  try {
    const page = await open(ctx, REAL_URL);
    await waitStatus(page, /^Ready\./);       // warblade.wasm compiled and ran, data loaded
    assert.equal(await page.evaluate(() => typeof Module._WebSaveSettings), 'function');
    assert.equal(await page.evaluate(() => Array.from(FS.readFile('/data/warblade.pac')).length), 9);
    // The data has links out of it (tests/run.sh): listed, refused, skipped.
    assert.equal(await page.evaluate(() => FS.analyzePath('/data/link-to-passwd').exists), false);
    await checkClean(page, /\/data\/link-/);
  } finally {
    await ctx.close();
  }
});
