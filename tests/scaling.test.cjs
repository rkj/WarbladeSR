// Actual SDL output, rather than a stand-in canvas: density must improve the
// picture without moving the native login or breaking trusted mobile taps.
'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { chromium } = require('playwright');
const path = require('node:path');

for (const density of [1, 3]) test(`mobile scaling at DPR ${density} preserves display density and input after rotation`,
  { skip: !process.env.WARBLADE_GAME_URL, timeout: 180000 }, async () => {
    const browser = await chromium.launch({ args: ['--no-sandbox'] });
    try {
      const context = await browser.newContext({ viewport: { width: 375, height: 812 },
        deviceScaleFactor: density, isMobile: true, hasTouch: true });
      const page = await context.newPage();
      const errors = [], accountRequests = [];
      page.on('pageerror', error => errors.push(error.message));
      await page.route('**/api/**', route => {
        if (new URL(route.request().url()).pathname === '/api/performance')
          return route.fulfill({ status: 204 });
        accountRequests.push(route.request().url());
        return route.abort();
      });
      await page.goto(process.env.WARBLADE_GAME_URL);
      await page.waitForFunction(() => gameStarted && Module._WebLoginReady() === 1,
        null, { timeout: 120000 });
      await page.locator('#loading').waitFor({ state: 'hidden' });
      await page.evaluate(() => {
        const sync = Module.syncLoginInputs;
        Module.syncLoginInputs = state => { window.lastNativeLogin = state; return sync(state); };
      });
      await page.waitForTimeout(1500); // allow the native window's entrance to finish

      const checkDisplay = async () => {
        await page.waitForFunction(density => {
          const canvas = document.getElementById('canvas');
          const rect = canvas.getBoundingClientRect();
          return Math.abs(canvas.width - rect.width * density) <= 2 &&
            Math.abs(canvas.height - rect.height * density) <= 2;
        }, density, { timeout: 10000 });
        const canvas = await page.locator('#canvas').boundingBox();
        assert.ok(Math.abs(canvas.width / canvas.height - 4 / 3) < 0.01);
        assert.equal(await page.locator('#canvas').evaluate(canvas =>
          getComputedStyle(canvas).imageRendering), 'auto');
        const state = await page.evaluate(() => lastNativeLogin);
        assert.equal(state.screenW, 800, 'game and input keep original logical resolution');
        assert.equal(state.screenH, 600);
        const password = await page.locator('#login-password').boundingBox();
        assert.ok(Math.abs(password.x - (canvas.x + state.x * canvas.width / 800)) < 2);
        assert.ok(Math.abs(password.y - (canvas.y + (state.y + 25) * canvas.height / 600)) < 2);
        const guest = await page.locator('#login-guest').boundingBox();
        assert.ok(Math.abs(guest.x - (canvas.x + (state.x - 120) * canvas.width / 800)) < 2);
        assert.ok(Math.abs(guest.y - (canvas.y + (state.y + 192) * canvas.height / 600)) < 2);
        return password;
      };
      let password = await checkDisplay();
      await page.touchscreen.tap(password.x + password.width / 2, password.y + password.height / 2);
      assert.equal(await page.evaluate(() => document.activeElement.id), 'login-password');
      await page.keyboard.type('MiXeD#123');
      await page.waitForFunction(() => lastNativeLogin.password === 'MiXeD#123');
      await page.setViewportSize({ width: 812, height: 375 });
      password = await checkDisplay();
      await page.touchscreen.tap(password.x + password.width / 2, password.y + password.height / 2);
      assert.equal(await page.evaluate(() => document.activeElement.id), 'login-password');
      await page.keyboard.press('Backspace');
      await page.waitForFunction(() => lastNativeLogin.password === 'MiXeD#12');
      await page.locator('#performance').waitFor({ state: 'visible', timeout: 15000 });
      assert.match(await page.locator('#performance').textContent(), /^\d+ FPS$/);
      if (process.env.WARBLADE_SCALING_EVIDENCE)
        await page.screenshot({ path: path.join(process.env.WARBLADE_SCALING_EVIDENCE,
          `mobile-login-dpr${density}.png`) });
      const guest = await page.locator('#login-guest').boundingBox();
      await page.touchscreen.tap(guest.x + guest.width / 2, guest.y + guest.height / 2);
      await page.waitForFunction(() => Module._WebIsGuest() === 1, null, { timeout: 15000 });
      assert.equal(await page.evaluate(() => Module._WebCanPlay()), 1);
      assert.deepEqual(accountRequests, [], 'scaling/input checks need no account or saved progress');
      assert.deepEqual(errors, []);
    } finally { await browser.close(); }
  });
