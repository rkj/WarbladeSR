// warp.c: Hyperspace between levels, the warp ring and flash, the warp malfunction.
#include <stdio.h>
#include "globals.h"
#include "game.h"

// Doneness delay (ms) for the mid-hyperspace phase's "done" flag; the jump-in phase uses the
// shared DONE_DELAY_MS (3000) instead.
enum {
    HYPERSPACE_MID_DONE_DELAY_MS = 1500,
    HAZARD_ENEMY_SCORE            = 5000  // score value of the hazard-type enemies WarpMalfunction spawns
};


// Always returns true; stub/flag kept from the original.
int CenturyLevelFlag()
{
    return 1;
}

// Randomly (once starSpeed is high enough) triggers a warp malfunction: reloads a random
// early level's data, clears most enemies and spawns a fresh wave of hazard-type enemies
// plus a randomized hazard graphic set per enemy slot.
void WarpMalfunction()
{
    int pl;
    int t;
    int i;
    int count;
    int j;

    pl = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        pl = 0;
    g_warpCheckCount++;
    if (g_numMalfunction > 0 && RandRange(0, g_malfunctionTimer) < 4 &&
        g_save.players[g_curPlayer].starSpeed > 120.0) {
        g_warpMalfunctionCount++;
        if (RandRange(0, 100) > 10)
            g_malfunctionTimer = RandRange(-2000, 6000) + 22000;
        else
            g_malfunctionTimer = RandRange(0, 12000) + 6000;
        SoundPlay(g_sfxAlienShoot15, RandRange(10000, 14000), 255, g_panTable[ClampX(400)], 127, g_sndFlags);
        sprintf(g_warpMalfunctionMsg, "W A R P   M A L F U N C T I O N");
        g_alertTextTime = g_time + 2000;
        g_warpMsgBlinkTime = g_time + 100;
        g_flashOverlayActive = 1;
        g_fadeStep = 0;
        g_fadeColorSet = 2;
        g_state = STATE_MALFUNCTION;

        // Reset the current player's run stats and hyperspace state for the malfunction sequence.
        g_save.players[pl].totalEnemies = 0;
        g_save.players[pl].killed = 0;
        g_save.players[pl].escaped = 0;
        g_save.players[pl].done = 0;
        g_save.players[pl].levelFinished = 0;
        g_malfunctionAlarmTime = g_time + 300;
        g_malfunctionBeepCount = 1;
        g_save.players[pl].doneTime = 0;

        g_save.players[g_curPlayer].hyperspaceOutTimer = 0;
        g_save.players[g_curPlayer].hyperspaceMidTimer = 0;
        g_save.players[g_curPlayer].hyperspaceInTimer = 0;
        g_save.players[g_curPlayer].hyperspaceInDuration = 100.0f;
        g_save.players[g_curPlayer].hyperspaceFade = 0;
        g_save.players[g_curPlayer].scrollSpeedY = 0;
        g_save.players[g_curPlayer].starSpeed = 0;
        g_save.players[g_curPlayer].starVelZ = -5.0f;

        // Load a random early level's data as the new "malfunction" backdrop.
        g_levelLoadingFlag = 0;
        g_levelDataLoaded = 0;
        g_curLevelNum = RandRange(0, g_numMalfunction) + 1;
        LoadLevelData();
        g_levelLoadingFlag = 1;

        // Clear all non-hazard enemies, then spawn a fresh wave of hazard-type enemies.
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (g_enemies[pl][i].type != ENEMY_CAPTURED)
                g_enemies[pl][i].active = 0;
        }
        count = RandRange(1, (int)g_save.players[g_curPlayer].shieldHitFlashSpeed);
        for (j = 0; j < MAX_ENEMIES; j++) {
            if (g_enemies[pl][j].active == 0) {
                g_save.players[g_curPlayer].totalEnemies++;
                t = RandRange(0, g_animFrameCount) + 1;
                switch (t) {
                case 1:
                    g_enemies[pl][j].gfxA = g_alienGfxCache[0].gfx1;
                    g_enemies[pl][j].gfxB = g_alienGfxCache[0].gfx2;
                    g_enemies[pl][j].frameSet = g_frames[0];
                    g_enemies[pl][j].hitRect = g_rectsB[0];
                    g_enemies[pl][j].shotFrame = g_alienGfxMem[0];
                    g_enemies[pl][j].shotGfxW = g_hazard0GfxW;
                    g_enemies[pl][j].shotGfxH = g_hazard0GfxH;
                    break;

                case 2:
                    g_enemies[pl][j].gfxA = g_alienGfxCache[1].gfx1;
                    g_enemies[pl][j].gfxB = g_alienGfxCache[1].gfx2;
                    g_enemies[pl][j].frameSet = g_frames[1];
                    g_enemies[pl][j].hitRect = g_rectsB[1];
                    g_enemies[pl][j].shotFrame = g_alienGfxMem[1];
                    g_enemies[pl][j].shotGfxW = g_hazard1GfxW;
                    g_enemies[pl][j].shotGfxH = g_hazard1GfxH;
                    break;

                case 3:
                    g_enemies[pl][j].gfxA = g_alienGfxCache[2].gfx1;
                    g_enemies[pl][j].gfxB = g_alienGfxCache[2].gfx2;
                    g_enemies[pl][j].frameSet = g_frames[2];
                    g_enemies[pl][j].hitRect = g_rectsB[2];
                    g_enemies[pl][j].shotFrame = g_alienGfxMem[2];
                    g_enemies[pl][j].shotGfxW = g_hazard2GfxW;
                    g_enemies[pl][j].shotGfxH = g_hazard2GfxH;
                    break;

                case 4:
                    g_enemies[pl][j].gfxA = g_alienGfxCache[3].gfx1;
                    g_enemies[pl][j].gfxB = g_alienGfxCache[3].gfx2;
                    g_enemies[pl][j].frameSet = g_frames[3];
                    g_enemies[pl][j].hitRect = g_rectsB[3];
                    g_enemies[pl][j].shotFrame = g_alienGfxMem[3];
                    g_enemies[pl][j].shotGfxW = g_hazard3GfxW;
                    g_enemies[pl][j].shotGfxH = g_hazard3GfxH;
                    break;

                case 5:
                    g_enemies[pl][j].gfxA = g_alienGfxCache[4].gfx1;
                    g_enemies[pl][j].gfxB = g_alienGfxCache[4].gfx2;
                    g_enemies[pl][j].frameSet = g_frames[4];
                    g_enemies[pl][j].hitRect = g_rectsB[4];
                    g_enemies[pl][j].shotFrame = g_alienGfxMem[4];
                    g_enemies[pl][j].shotGfxW = g_hazard4GfxW;
                    g_enemies[pl][j].shotGfxH = g_hazard4GfxH;
                    break;

                case 6:
                    g_enemies[pl][j].gfxA = g_alienGfxCache[5].gfx1;
                    g_enemies[pl][j].gfxB = g_alienGfxCache[5].gfx2;
                    g_enemies[pl][j].frameSet = g_frames[5];
                    g_enemies[pl][j].hitRect = g_rectsB[5];
                    g_enemies[pl][j].shotFrame = g_alienGfxMem[5];
                    g_enemies[pl][j].shotGfxW = g_hazard5GfxW;
                    g_enemies[pl][j].shotGfxH = g_hazard5GfxH;
                    break;
                }

                // Common hazard-enemy stats, independent of which graphic set was rolled above.
                g_enemies[pl][j].score = HAZARD_ENEMY_SCORE;
                g_enemies[pl][j].active = 1;
                g_enemies[pl][j].x = RandRange(0, (int)(g_screenW - SPAWN_X_SPAN) >> 1) + SPAWN_X_MARGIN;
                g_enemies[pl][j].y = -110.0f;
                g_enemies[pl][j].speedX = 1.0f;
                g_enemies[pl][j].speedY = 1.0f;
                g_enemies[pl][j].patternTimer = 0;
                g_enemies[pl][j].type = ENEMY_WRAPPER;
                g_enemies[pl][j].dirStepTimer = 3.0f;
                g_enemies[pl][j].turnState = 2;
                g_enemies[pl][j].facing = RandRange(0, 5) + 18;
                g_enemies[pl][j].hp = RandRange(0, g_save.players[g_curPlayer].enemyHpBonusRoll) +
                                      (int)g_diffHpBonusB + 7;
                g_enemies[pl][j].locked = 0;

                g_enemies[pl][j].fireDelay =
                    (g_enemyFireRateMin > RandRange(0, 500) + 400 + g_fireDelayBiasB) ?
                    g_enemyFireRateMin : RandRange(0, 500) + 400 + g_fireDelayBiasB;
                g_enemies[pl][j].fireDelayStep = 0;
                g_enemies[pl][j].attackDelay = (g_fireDelayMin > g_fireDelayBiasA + 100) ?
                    g_fireDelayMin : g_fireDelayBiasA + 100;
                g_enemies[pl][j].attackDelayStep = 10;
                g_enemies[pl][j].attackStaggerTimer = 0;
                g_enemies[pl][j].turnTimer = RandFloat(0.0f, 100.0f) + 50;
                g_enemies[pl][j].unusedAlpha = 64;
                g_enemies[pl][j].srcX = 0;
                g_enemies[pl][j].srcY = 0;

                g_enemies[pl][j].animTimer = (float)RandRange(0, 4) + 5;
                g_enemies[pl][j].animReverse = RandRange(0, 2);
                g_enemies[pl][j].animFrame = 0;
                g_enemies[pl][j].animSpeedDivisor = g_enemies[pl][j].hp / 6;
                g_enemies[pl][j].speedScale = RandFloat(0.8f, 3.0f);
                g_enemies[pl][j].unusedSpawnDelay = RandRange(0, 3) + 2;
                g_enemies[pl][j].animStepTime = RandFloat(0.0f, 8.0f) + 3;
                g_enemies[pl][j].animFrameCount = g_curLevelData.aux[0].v[0] - 1;
                g_enemies[pl][j].animPingPong = g_curLevelData.aux[0].y1;
                g_enemies[pl][j].fixedFireDelay = 0;
                g_enemies[pl][j].flashActive = 0;
                g_enemies[pl][j].flashTimer = 0;
                if (--count < 0)
                    break;
            }
        }
    }
}

// Draws the spinning warp-ring fx sprites (g_fx[0..9]), skipping the draw entirely while a
// blocking dialog window (profile/quit/quit-to-windows/generic) is open. Called once per frame
// during a warp transition.
void DrawWarpRing()
{
    int i;
    float angle;
    if (!g_profileWinOpen && !g_quitGameWinOpen && !g_quitToWindowsWinOpen && !g_dialogWinOpen) {
        for (i = 0; i < MAX_FX; i++) {
            if (g_fx[i].active != 0) {
                angle = g_fxSpinAngle - i * g_fxAngleStep;
                if (angle < 0.0)
                    angle = angle + 360.0;
                if (angle >= 360.0)
                    angle = angle - 360.0;
                QueueStretchRot(g_fx[i].gfx, g_fx[i].x, g_fx[i].y,
                                       g_fx[i].x + g_fx[i].speed,
                                       g_fx[i].y + g_fx[i].scale,
                                       (unsigned char)g_fxRed, (unsigned char)g_fxColorG,
                                       (unsigned char)g_fxBlueColorLevel, (unsigned char)g_fx[i].alpha,
                                       0, angle);
            }
        }
    }
}

// Draws and grows the two warp-flash effects (g_fx[0]: the expanding flash ring, g_fx[1]: its
// fade trail), queuing the whoosh sound once the flash reaches a large enough radius and
// deactivating it once it's grown off-screen. Called once per frame during a warp transition.
void DrawWarpFlash()
{
    int i;
    float w;
    float h;

    for (i = 0; i < 2; i++) {
        if (g_fx[i].active != 0) {
            h = g_fx[i].speed / 4.0;
            if (h < 1.0)
                h = 1.0;
            w = g_fx[i].speed;
            if (w < 1.0)
                w = 1.0;
            if (i == 0) {
                h = w;  // flash 0 is drawn as a circle: height follows width
                if (w > 600.0 && !g_warpWhooshPlayed) {
                    SoundQueueAdd(g_sfxGameOver, 50, 0);
                    g_warpWhooshPlayed = 1;
                }
                if (w > 1000.0)
                    g_fx[i].active = 0;
            }

            QueueStretchRot(g_fx[i].gfx, g_fx[i].x - w, g_fx[i].y - h,
                                   g_fx[i].x + w, g_fx[i].y + h,
                                   g_fx[i].r, g_fx[i].g, g_fx[i].b,
                                   (int)g_fx[i].alpha, 0, 0.0f);
        }
        if (i == 0) {
            g_fx[i].speed *= 1.0f + g_frameDt * 0.04f;
        } else if (g_fx[i].speed < 350.0) {
            g_fx[i].speed *= 1.0f + g_frameDt * 0.074f;
        } else if (g_fx[i].alpha > 0.0) {
            g_fx[i].alpha -= 1.0f * g_frameDt;
        }
    }
}

#define CUR g_save.players[g_curPlayer]

// Picks and queues one of the (up to 5) configured shop-entry stingers at random, skipping any
// that aren't loaded. Identical in all 3 call sites in UpdateHyperspace (each already inside its
// own braces, so this doesn't add a scope of its own).
#define PLAY_RANDOM_SHOP_SOUND()                      \
    int n = 2;                                        \
    if (g_sfxShop1 != 0) n++;                          \
    if (g_sfxShop2 != 0) n++;                          \
    if (g_sfxShop3 != 0) n++;                          \
    if (g_sfxShop4 != 0) n++;                          \
    if (g_sfxShop5 != 0) n++;                          \
    n = RandRange(0, n);                               \
    if (n == 0) SoundQueueAdd(g_sfxShop1, 0, 0);       \
    if (n == 1) SoundQueueAdd(g_sfxShop2, 0, 0);       \
    if (n == 2) SoundQueueAdd(g_sfxShop3, 0, 0);       \
    if (n == 3) SoundQueueAdd(g_sfxShop4, 0, 0);       \
    if (n == 4) SoundQueueAdd(g_sfxShop5, 0, 0);

// Advances the current player's hyperspace jump-in/mid/out sequence and the starfield
// scroll for it. On jump-in completing, finalizes the just-cleared level (kills off
// remaining enemies, advances a pending level warp up to 4 times, and routes to the
// shop, the next level, the end-of-century sequence, or back to the title for demo
// mode) and resets the hyperspace state.
void UpdateHyperspace()
{
    int i;
    int unused = 0;

    if (g_gameMode == MODE_DUAL)
        g_curPlayer = 0;

    if (CUR.hyperspaceInTimer > 0) {
        float d = 1.15f;
        if (d == 0)
            d = 1;
        CUR.starVelZ = CUR.starVelZ / d;
        CUR.starSpeed -= 2.15f;
        CUR.starVelZ -= 0.15f;
        CUR.hyperspaceFade -= 0.01f;
        CUR.hyperspaceInTimer -= 1.0;
        if (CUR.hyperspaceInTimer <= 0) {
            CUR.hyperspaceInTimer = 0;
            g_deathSeqActive = 0;
            CUR.killed = CUR.totalEnemies;
            CUR.escaped = 0;
            if (CUR.done == 0) {
                CUR.done = 1;
                if (CUR.doneTime == 0)
                    CUR.doneTime = g_time + DONE_DELAY_MS;
            }

            for (i = 0; i < MAX_ENEMIES; i++) {
                if (g_enemies[g_curPlayer][i].type != ENEMY_CAPTURED)
                    g_enemies[g_curPlayer][i].active = 0;
            }

            // ---- chain a pending level warp up to 4 levels ----
            if (CUR.levelWarpPending != 0) {
                // NOTE: repeated 4x in the original (a level-warp can chain through up to
                // 4 non-special levels in a row).
                if (IsSpecialLevel(CUR.level) == 0) {
                    g_malfunctionTimer -= RandRange(100, 1500);
                    if (g_malfunctionTimer < 2000)
                        g_malfunctionTimer = RandRange(2300, 4000);
                    CUR.level++;
                    StartLevel();
                }

                if (IsSpecialLevel(CUR.level) == 0) {
                    g_malfunctionTimer -= RandRange(100, 1500);
                    if (g_malfunctionTimer < 2000)
                        g_malfunctionTimer = RandRange(2300, 4000);
                    CUR.level++;
                    StartLevel();
                }

                if (IsSpecialLevel(CUR.level) == 0) {
                    g_malfunctionTimer -= RandRange(100, 1500);
                    if (g_malfunctionTimer < 2000)
                        g_malfunctionTimer = RandRange(2300, 4000);
                    CUR.level++;
                    StartLevel();
                }

                if (IsSpecialLevel(CUR.level) == 0) {
                    g_malfunctionTimer -= RandRange(100, 1500);
                    if (g_malfunctionTimer < 2000)
                        g_malfunctionTimer = RandRange(2300, 4000);
                    CUR.level++;
                    StartLevel();
                }

                g_shopSelItem = 0;
                g_offX = 0;
                g_shopSlideVelX = 0;
                g_offY = -600;
                g_shopSlideVelY = 40;
                g_shopBounceY = -30;
                CUR.levelTransitioning = 1;
                CUR.levelWarpPending = 0;
                g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
                g_transitionLock = 1;
            }

            // ---- route to demo/title, the shop, the end-of-century sequence, or next level ----
            if (g_playerUpdateFn == StateDemo) {
                CUR.levelTransitioning = 0;
                ResetToTitle();
                return;
            } else if (g_gameMode == MODE_DUAL) {
                if (g_save.players[0].money >= SHOP_MIN_MONEY
                        && g_save.players[0].lives > g_shipDefs[g_save.players[0].ship]->minEnergy) {
                    PLAY_RANDOM_SHOP_SOUND()

                    g_shopCurPlayer = 0;
                    CheckProfileBonus();
                    PlayShopMusic();
                    if (g_save.players[g_shopCurPlayer].autofireUnlocked == 0)
                        g_save.players[g_shopCurPlayer].autofire = g_save.players[g_shopCurPlayer].superAuto;
                    g_shopTransition = 500;
                    g_state = STATE_SHOP;
                    g_buttonsOn = 0;
                    HidePointer();
                } else if (g_save.players[1].money >= SHOP_MIN_MONEY
                        && g_save.players[1].lives > g_shipDefs[g_save.players[1].ship]->minEnergy) {
                    PLAY_RANDOM_SHOP_SOUND()

                    g_shopCurPlayer = 1;
                    CheckProfileBonus();
                    PlayShopMusic();
                    if (g_save.players[g_shopCurPlayer].autofireUnlocked == 0)
                        g_save.players[g_shopCurPlayer].autofire = g_save.players[g_shopCurPlayer].superAuto;
                    g_shopTransition = 500;
                    g_state = STATE_SHOP;
                    g_buttonsOn = 0;
                    HidePointer();
                } else {
                    CUR.levelTransitioning = 0;
                    StartNextLevel();
                }
            } else {
                if (CUR.level % 100 == 0 && CUR.level > 5 && g_autoplay == 0) {
                    CenturyLevelFlag();
                    PlayEndMusic();
                    int saved = g_state;
                    g_endPic = 1;
                    EndSequence();
                    g_state = saved;
                    ResetF893();
                }

                if (CUR.money >= SHOP_MIN_MONEY && g_gameMode != MODE_TIME_TRIAL) {
                    PLAY_RANDOM_SHOP_SOUND()

                    CheckProfileBonus();
                    PlayShopMusic();
                    if (CUR.autofireUnlocked == 0)
                        CUR.autofire = CUR.superAuto;
                    g_shopTransition = 500;
                    g_state = STATE_SHOP;
                    g_buttonsOn = 0;
                    HidePointer();
                } else {
                    CUR.levelTransitioning = 0;
                    StartNextLevel();
                }
            }

            CUR.starSpeed = 5;
            CUR.starVelZ = 0;
            CUR.hyperspaceFade = 0;
            CUR.scrollSpeedY = 0;
        }
    }

    // ---- hyperspace mid-phase (between jump-in and jump-out) ----
    if (CUR.hyperspaceMidTimer > 0) {
        float d = 1.15f;
        if (d == 0)
            d = 1;
        CUR.starVelZ = CUR.starVelZ / d;
        CUR.scrollSpeedY += 0.25;
        CUR.hyperspaceMidTimer -= 1.0;
        if (CUR.hyperspaceMidTimer <= 0) {
            CUR.hyperspaceInTimer = CUR.hyperspaceInDuration;
            SoundPlay(g_sfxWarp2, -1, 255, 0.0f, 127, g_sndFlags);
            if (CUR.done == 0) {
                CUR.done = 1;
                if (CUR.doneTime == 0)
                    CUR.doneTime = g_time + HYPERSPACE_MID_DONE_DELAY_MS;
            }
        }
    }

    // ---- hyperspace jump-out phase ----
    if (CUR.hyperspaceOutTimer > 0) {
        g_deathSeqActive = 1;
        float d = 1.15f;
        if (d == 0)
            d = 1;
        CUR.hyperspaceFade += 0.01f;
        CUR.starSpeed += 2.15f;
        CUR.starVelZ += 0.15f;
        CUR.starVelZ = CUR.starVelZ / d;
        CUR.scrollSpeedY += 0.25;
        CUR.hyperspaceOutTimer -= 1.0;
        if (CUR.hyperspaceOutTimer <= 0)
            CUR.hyperspaceMidTimer = CUR.hyperspaceInDuration * 2;
    }

    // ---- scroll and wrap the starfield ----
    for (i = 0; i < (int)g_cfg.numStars; i++) {
        g_starZ[i] = CUR.starVelZ + g_starZ[i];
        g_starX[i] = CUR.starVelX + g_starX[i];
        g_starY[i] = CUR.starSpeed + g_starY[i];
        if (g_starZ[i] > g_starZFar)
            g_starZ[i] -= g_starZFar - g_starZNear;
        if (g_starZ[i] < g_starZNear)
            g_starZ[i] += g_starZFar - g_starZNear;
        if (g_starX[i] < g_starXMin)
            g_starX[i] += g_starXMax - g_starXMin;
        if (g_starX[i] > g_starXMax)
            g_starX[i] -= g_starXMax - g_starXMin;
        if (g_starY[i] < g_starYMin)
            g_starY[i] += g_starYMax - g_starYMin;
        if (g_starY[i] > g_starYMax)
            g_starY[i] -= g_starYMax - g_starYMin;
    }
}

#undef PLAY_RANDOM_SHOP_SOUND
#undef CUR
