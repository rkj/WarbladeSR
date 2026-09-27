// hud.c: The in-game HUD: the HUD strip and per-mode layouts, FPS, boss bar, rank alert, score
// popups.
#include <stdio.h>
#include "globals.h"
#include "game.h"

// HUD strip layout: bar length (in pixels) shared by the speed/ammo/time/fire-rate bars, and the
// x threshold below which HUD elements are drawn left-aligned instead of right-aligned.
#define HUD_BAR_LEN 45.0
enum { HUD_SIDE_SPLIT_X = 400 };


// Recomputes g_fps once per second from the frame counter and draws the "N FPS" overlay.
void DrawFps()
{
    unsigned int elapsed;
    char buff[256];

    g_frameCount++;
    elapsed = SysMillis() - g_lastFpsTime;
    if (elapsed > 1000) {  // 1000 ms since the last FPS sample
        if (elapsed == 0)
            elapsed = 1;  // avoid divide-by-zero
        g_fps = g_frameCount * 1000 / elapsed;
        g_lastFpsTime = SysMillis();
        g_frameCount = 0;
    }
    if (g_fps == 0)
        return;
    if (g_fps != g_prevFps)
        g_prevFps = g_fps;
    if (g_fps > 150)
        g_fps = 150;  // clamp display to a sane max
    sprintf(buff, "%d FPS", g_fps);
    DrawMenuText(buff, (g_screenW >> 1) - 25, 30, 0);
}

// Draws `count` copies of a 16x15 logo-strip tile (column `col`) in a row, 19px apart.
void DrawRow(int x, int y, int col, int count)
{
    int i;
    for (i = 0; i < count; i++)
        Blit(x + i * 19, y, 0, g_gfxLogos, col << 4, 160, 16, 15);
}

// Queues the "new rank available" alert message for 5 seconds.
void NewRank()
{
    sprintf(g_alertMsg, "******  NEW RANK IS NOW AVAILABLE  ******");
    g_msgColor = 8;
    g_msgTimer = g_time + 5000;
}

// Draws the in-game HUD strip for player `p` at screen position (x, y): money counter,
// EXTRA-letter icons, spare-life icons, score-multiplier/freeze icons, armour marks, the
// speed/ammo/time/fire-rate bars, rocket count, rank badge, marathon/secret bird icons, level
// number, and (during a meteor storm) the race meter. `x` also selects left/right alignment
// for player-specific HUD elements.
void Hud(int p, int x, int meterX)
{
    int n;
    int i;
    int t;
    int y0;
    int y1;
    int y2;
    int y3;
    int y4;
    int y5;
    int y6;
    int saved0;
    int saved1;
    int one;
    int cnt;
    float ratio;
    int d0;
    int d1;
    int d2;
    int d3;
    int bx;
    int by;
    float scale;

    y0 = 0x16;
    y1 = 0x44;
    y2 = 0x55;
    y3 = 0xc3;
    y4 = 0xf0;
    y5 = 0x244;
    y6 = 0x1fe;

    saved0 = g_clipLeft;
    saved1 = g_clipRight;
    g_clipLeft = 0;
    g_clipRight = g_screenW;

    // ---- money ----
    if (g_gameMode != MODE_TIME_TRIAL) {
        if (!g_moneyBlinkTimer--) {
            g_moneyBlinkTimer = 16;
            if (p == 0) {
                sprintf(g_moneyBuf0, "$%d", g_save.players[p].money);
                g_moneyW0 = StrLenPlat(g_moneyBuf0) << 2;
            }

            if (p == 1) {
                sprintf(g_moneyBuf1, "$%d", g_save.players[p].money);
                g_moneyW1 = StrLenPlat(g_moneyBuf1) << 2;
            }

            if (p == 2) {
                sprintf(g_moneyBuf2, "$%d", g_save.players[p].money);
                g_moneyW2 = StrLenPlat(g_moneyBuf2) << 2;
            }

            if (p == 3) {
                sprintf(g_moneyBuf3, "$%d", g_save.players[p].money);
                g_moneyW3 = StrLenPlat(g_moneyBuf3) << 2;
            }
        }
        if (p == 0) {
            if (x < HUD_SIDE_SPLIT_X)
                DrawTinyText2(g_moneyBuf0, 32 - g_moneyW0, 2, 3);
            else
                DrawTinyText2(g_moneyBuf0, g_screenW - 32 - g_moneyW0, 2, 4);
        }

        if (p == 1) {
            if (x < HUD_SIDE_SPLIT_X)
                DrawTinyText2(g_moneyBuf1, 32 - g_moneyW1, 2, 3);
            else
                DrawTinyText2(g_moneyBuf1, g_screenW - 32 - g_moneyW1, 2, 4);
        }

        if (p == 2) {
            if (x < HUD_SIDE_SPLIT_X)
                DrawTinyText2(g_moneyBuf2, 32 - g_moneyW2, 2, 3);
            else
                DrawTinyText2(g_moneyBuf2, g_screenW - 32 - g_moneyW2, 2, 4);
        }

        if (p == 3) {
            if (x < HUD_SIDE_SPLIT_X)
                DrawTinyText2(g_moneyBuf3, 32 - g_moneyW3, 2, 3);
            else
                DrawTinyText2(g_moneyBuf3, g_screenW - 32 - g_moneyW3, 2, 4);
        }
    }
    // ---- EXTRA letters ----
    if (g_save.players[p].extraLetterE)
        Blit2(x - 10, y2, 0, g_gfxBonus, g_bonusIconFrame1 * 20, 60, 20, 20);
    if (g_save.players[p].extraLetterX)
        Blit2(x - 10, y2 + 21, 0, g_gfxBonus, g_bonusIconFrame2 * 20, 80, 20, 20);
    if (g_save.players[p].extraLetterT)
        Blit2(x - 10, y2 + 42, 0, g_gfxBonus, g_bonusIconFrame3 * 20, 100, 20, 20);
    if (g_save.players[p].extraLetterR)
        Blit2(x - 10, y2 + 63, 0, g_gfxBonus, g_bonusIconFrame4 * 20, 120, 20, 20);
    if (g_save.players[p].extraLetterA)
        Blit2(x - 10, y2 + 84, 0, g_gfxBonus, g_bonusIconFrame5 * 20, 140, 20, 20);

    // ---- lives ----
    one = 1;
    if (g_curPlayer != p && g_gameMode != MODE_DUAL)
        one = 0;
    cnt = (g_save.players[p].lives - g_shipDefs[g_save.players[p].ship]->minEnergy) /
        g_shipDefs[g_save.players[p].ship]->cost;
    if (cnt > one) {
        for (i = 0; i < cnt - one; i++) {
            if (i < 4)
                Blit2(x - 8, i * 10 + y0, 0, g_gfxLogos, 0, 0, 16, 10);
        }
    }

    // ---- freeze/multiplier timers ----
    if (g_save.players[p].freezeTimer) {
        t = g_time;
        if (t > g_save.players[p].freezeTimer) {
            g_save.players[p].freezeTimer = 0;
            g_viewTransitionFlag = 10;
            g_stateFn = SetViewHud;
            EmptyViewChangeHook();
            g_drawBordersFn = DrawBorders;
        } else {
            ratio = (g_save.players[p].freezeTimer - t) / 10000.0;
            if (x < HUD_SIDE_SPLIT_X)
                Blit2(x - 21, y6, 0, g_gfxLogos, 0, 64, (int)(ratio * 43.0), 15);
            else
                Blit2(x - 21, y6, 0, g_gfxLogos, 0, 64, (int)(ratio * 43.0), 15);
        }
    }

    if (g_save.players[p].scoreMult2Timer) {
        t = g_time;
        if (t > g_save.players[p].scoreMult2Timer) {
            g_save.players[p].scoreMult2Timer = 0;
            g_scoreMul[p] = 1;
            g_viewTransitionFlag = 2;
            g_stateFn = SetViewHud;
            EmptyViewChangeHook();
            g_drawBordersFn = DrawBorders;
        }
    }

    if (g_save.players[p].scoreMult5Timer) {
        t = g_time;
        if (t > g_save.players[p].scoreMult5Timer) {
            g_save.players[p].scoreMult5Timer = 0;
            g_scoreMul[p] = 1;
            g_viewTransitionFlag = 2;
            g_stateFn = SetViewHud;
            EmptyViewChangeHook();
            g_drawBordersFn = DrawBorders;
        }
    }

    if (g_scoreMul[p] == 2)
        Blit2(x - 8, y1, 0, g_gfxLogos, 16, 0, 16, 10);
    if (g_scoreMul[p] == 5)
        Blit2(x - 8, y1, 0, g_gfxLogos, 32, 0, 16, 10);
    if (g_scoreMul[p] != 2 && g_scoreMul[p] != 5)
        g_scoreMul[p] = 1;

    // ---- marks: secret-collected "mark" icon row (bit flags, one small icon each) ----
    if ((short)g_save.players[p].marks & MARK_1)
        Blit2(x - 8, y3, 0, g_gfxLogos, 48, 0, 16, 4);
    if ((short)g_save.players[p].marks & MARK_2)
        Blit2(x - 8, y3 + 6, 0, g_gfxLogos, 48, 4, 16, 4);
    if ((short)g_save.players[p].marks & MARK_3)
        Blit2(x - 8, y3 + 12, 0, g_gfxLogos, 64, 0, 16, 4);
    if ((short)g_save.players[p].marks & MARK_4)
        Blit2(x - 8, y3 + 18, 0, g_gfxLogos, 64, 4, 16, 4);

    if ((short)g_save.players[p].marks & MARK_5)
        Blit2(x - 8, y3 + 24, 0, g_gfxLogos, 80, 0, 16, 4);
    if ((short)g_save.players[p].marks & MARK_6)
        Blit2(x - 8, y3 + 30, 0, g_gfxLogos, 80, 4, 16, 4);

    // ---- armour ----
    if (g_save.players[p].armour > g_shipDefs[g_save.players[p].ship]->baseArmour) {
        n = (g_save.players[p].armour - g_shipDefs[g_save.players[p].ship]->baseArmour) /
            g_shipDefs[g_save.players[p].ship]->armourStep;
        if (n > 2)
            n = 2;
        for (i = 0; i < n; i++)
            Blit2(x - 7, i * 12 + y4, 0, g_gfxLogos, 96, 0, 16, 11);
    }

    // ---- bar computation (speed/ammo/time/fire-rate, recomputed every 14 frames) ----
    if (!g_hudSpeedBarTick--) {
        g_hudSpeedBarTick = 14;
        if (p == 0) {
            d0 = (int)(g_speedStep * g_maxSpeedMul + g_speedBase);
            if (d0 == 0)
                d0 = 1;
            g_speedBarP0 = (int)(g_save.players[p].speed / d0 * HUD_BAR_LEN);
            d0 = MAX_BULLETS;
            if (d0 == 0)
                d0 = 1;
            g_ammoBarP0 = (int)((float)g_save.players[p].bullets / d0 * HUD_BAR_LEN) + 1 > 45 ? 45 :
                (int)((float)g_save.players[p].bullets / d0 * HUD_BAR_LEN) + 1;
            d0 = g_timeMax;
            if (d0 == 0)
                d0 = 1;
            g_timeBarP0 = (int)((float)g_save.players[p].buffDuration / d0 * HUD_BAR_LEN);
            g_fireRateBarP0 = 0;
            if (g_save.players[p].bulletSpeedMult > 0.0) {
                d0 = (int)(g_bulletSpeedMax - 1.0);
                if (d0 == 0)
                    d0 = 1;
                g_fireRateBarP0 = (int)((float)(g_save.players[p].bulletSpeedMult - 1.0) / d0 * HUD_BAR_LEN);
            }
        }

        if (p == 1) {
            d1 = (int)(g_speedStep * g_maxSpeedMul + g_speedBase);
            if (d1 == 0)
                d1 = 1;
            g_speedBarP1 = (int)(g_save.players[p].speed / d1 * HUD_BAR_LEN);
            d1 = MAX_BULLETS;
            if (d1 == 0)
                d1 = 1;
            g_ammoBarP1 = (int)((float)g_save.players[p].bullets / d1 * HUD_BAR_LEN) + 1 > 45 ? 45 :
                (int)((float)g_save.players[p].bullets / d1 * HUD_BAR_LEN) + 1;
            d1 = g_timeMax;
            if (d1 == 0)
                d1 = 1;
            g_timeBarP1 = (int)((float)g_save.players[p].buffDuration / d1 * HUD_BAR_LEN);
            g_fireRateBarP1 = 0;
            if (g_save.players[p].bulletSpeedMult > 0.0) {
                d1 = (int)(g_bulletSpeedMax - 1.0);
                if (d1 == 0)
                    d1 = 1;
                g_fireRateBarP1 = (int)((float)(g_save.players[p].bulletSpeedMult - 1.0) / d1 * HUD_BAR_LEN);
            }
        }

        if (p == 2) {
            d2 = (int)(g_speedStep * g_maxSpeedMul + g_speedBase);
            if (d2 == 0)
                d2 = 1;
            g_speedBarP2 = (int)(g_save.players[p].speed / d2 * HUD_BAR_LEN);
            d2 = MAX_BULLETS;
            if (d2 == 0)
                d2 = 1;
            g_ammoBarP2 = (int)((float)g_save.players[p].bullets / d2 * HUD_BAR_LEN) + 1 > 45 ? 45 :
                (int)((float)g_save.players[p].bullets / d2 * HUD_BAR_LEN) + 1;
            d2 = g_timeMax;
            if (d2 == 0)
                d2 = 1;
            g_timeBarP2 = (int)((float)g_save.players[p].buffDuration / d2 * HUD_BAR_LEN);
            g_fireRateBarP2 = 0;
            if (g_save.players[p].bulletSpeedMult > 0.0) {
                d2 = (int)(g_bulletSpeedMax - 1.0);
                if (d2 == 0)
                    d2 = 1;
                g_fireRateBarP2 = (int)((float)(g_save.players[p].bulletSpeedMult - 1.0) / d2 * HUD_BAR_LEN);
            }
        }

        if (p == 3) {
            d3 = (int)(g_speedStep * g_maxSpeedMul + g_speedBase);
            if (d3 == 0)
                d3 = 1;
            g_speedBarP3 = (int)(g_save.players[p].speed / d3 * HUD_BAR_LEN);
            d3 = MAX_BULLETS;
            if (d3 == 0)
                d3 = 1;
            g_ammoBarP3 = (int)((float)g_save.players[p].bullets / d3 * HUD_BAR_LEN) + 1 > 45 ? 45 :
                (int)((float)g_save.players[p].bullets / d3 * HUD_BAR_LEN) + 1;
            d3 = g_timeMax;
            if (d3 == 0)
                d3 = 1;
            g_timeBarP3 = (int)((float)g_save.players[p].buffDuration / d3 * HUD_BAR_LEN);
            g_fireRateBarP3 = 0;
            if (g_save.players[p].bulletSpeedMult > 0.0) {
                d3 = (int)(g_bulletSpeedMax - 1.0);
                if (d3 == 0)
                    d3 = 1;
                g_fireRateBarP3 = (int)((float)(g_save.players[p].bulletSpeedMult - 1.0) / d3 * HUD_BAR_LEN);
            }
        }

        g_viewTransitionFlag = 2;
        g_stateFn = SetViewHud;
        g_drawBordersFn = DrawBorders;
    }

    // ---- bar drawing ----
    if (p == 0) {
        Blit2(x - 25, y4 + 35, 0, g_gfxLogos, 45, 22, g_speedBarP0 + 6, 5);
        Blit2(x - 25, y4 + 41, 0, g_gfxLogos, 45, 28, g_ammoBarP0 + 6, 5);
        Blit2(x - 25, y4 + 47, 0, g_gfxLogos, 45, 34, g_timeBarP0 + 6, 5);
        if (g_fireRateBarP0)
            Blit2(x - 27, y4 + 53, 0, g_gfxLogos, 43, 40, g_fireRateBarP0 + 8, 5);
    }

    if (p == 1) {
        Blit2(x - 25, y4 + 35, 0, g_gfxLogos, 45, 22, g_speedBarP1 + 6, 5);
        Blit2(x - 25, y4 + 41, 0, g_gfxLogos, 45, 28, g_ammoBarP1 + 6, 5);
        Blit2(x - 25, y4 + 47, 0, g_gfxLogos, 45, 34, g_timeBarP1 + 6, 5);
        if (g_fireRateBarP1)
            Blit2(x - 27, y4 + 53, 0, g_gfxLogos, 43, 40, g_fireRateBarP1 + 8, 5);
    }

    if (p == 2) {
        Blit2(x - 25, y4 + 35, 0, g_gfxLogos, 45, 22, g_speedBarP2 + 6, 5);
        Blit2(x - 25, y4 + 41, 0, g_gfxLogos, 45, 28, g_ammoBarP2 + 6, 5);
        Blit2(x - 25, y4 + 47, 0, g_gfxLogos, 45, 34, g_timeBarP2 + 6, 5);
        if (g_fireRateBarP2)
            Blit2(x - 27, y4 + 53, 0, g_gfxLogos, 43, 40, g_fireRateBarP2 + 8, 5);
    }

    if (p == 3) {
        Blit2(x - 25, y4 + 35, 0, g_gfxLogos, 45, 22, g_speedBarP3 + 6, 5);
        Blit2(x - 25, y4 + 41, 0, g_gfxLogos, 45, 28, g_ammoBarP3 + 6, 5);
        Blit2(x - 25, y4 + 47, 0, g_gfxLogos, 45, 34, g_timeBarP3 + 6, 5);
        if (g_fireRateBarP3)
            Blit2(x - 27, y4 + 53, 0, g_gfxLogos, 43, 40, g_fireRateBarP3 + 8, 5);
    }

    // ---- rockets ----
    if (g_save.players[p].rockets > 0) {
        if (x < HUD_SIDE_SPLIT_X) {
            DrawTinyText2("ROCKETS", x - 28, y4 + 70, 3);
            sprintf(g_scoreBuf, "%02d", g_save.players[p].rockets);
            DrawTinyText2(g_scoreBuf, x - 8, y4 + 78, 3);
        } else {
            DrawTinyText2("ROCKETS", x - 24, y4 + 70, 4);
            sprintf(g_scoreBuf, "%02d", g_save.players[p].rockets);
            DrawTinyText2(g_scoreBuf, x - 8, y4 + 78, 4);
        }
    }

    // ---- rank badge and pips ----
    bx = x - 21;
    by = y4 + 90;
    i = g_save.players[p].rank;
    Blit2(bx - 4, by, 0, g_gfxRankIcons, 0, g_weaponRow[i], 64, 10);

    // Draws pip `n` (0, 1 or 2) of a rank's tier row, 16px apart, in column `col` of g_gfxLogos.
#define RANK_PIP(col, n) Blit2(bx - 3 + 16 * (n), by + 13, 0, g_gfxLogos, col, 160, 16, 15);

    if (i == RANK_ADMIRAL_1_1) {
        RANK_PIP(0, 0)
    }
    if (i == RANK_ADMIRAL_1_2) {
        RANK_PIP(0, 0)
        RANK_PIP(0, 1)
    }
    if (i == RANK_ADMIRAL_1_3) {
        RANK_PIP(0, 0)
        RANK_PIP(0, 1)
        RANK_PIP(0, 2)
    }

    if (i == RANK_ADMIRAL_2_1) {
        RANK_PIP(16, 0)
    }
    if (i == RANK_ADMIRAL_2_2) {
        RANK_PIP(16, 0)
        RANK_PIP(16, 1)
    }
    if (i == RANK_ADMIRAL_2_3) {
        RANK_PIP(16, 0)
        RANK_PIP(16, 1)
        RANK_PIP(16, 2)
    }

    if (i == RANK_ADMIRAL_3_1) {
        RANK_PIP(32, 0)
    }
    if (i == RANK_ADMIRAL_3_2) {
        RANK_PIP(32, 0)
        RANK_PIP(32, 1)
    }
    if (i == RANK_ADMIRAL_3_3) {
        RANK_PIP(32, 0)
        RANK_PIP(32, 1)
        RANK_PIP(32, 2)
    }

    if (i == RANK_GRANDMASTER_1) {
        RANK_PIP(32, 0)
    }
    if (i == RANK_GRANDMASTER_2) {
        RANK_PIP(32, 0)
        RANK_PIP(32, 1)
    }
    if (i == RANK_GRANDMASTER_3) {
        RANK_PIP(32, 0)
        RANK_PIP(32, 1)
        RANK_PIP(32, 2)
    }
#undef RANK_PIP

    // ---- marathon / secret bird ----
    if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo && p == 0) {
        ShowMarathonScore(g_profileIndex);
        if (g_shownStat >= MARATHON_MILESTONE_SCORE && g_save.players[p].secretBirdHits > 0) {
            bx = x - 20;
            by = y4 + 125;
            if (g_save.players[p].secretBirdHits < 10) {
                for (i = 0; i < g_save.players[p].secretBirdHits; i++)
                    Blit2(i % 2 * 20 + bx, (i >> 1) * 22 + by, 0, g_gfxLogos, 80, 254, 19, 20);
            } else {
                for (i = 0; i < g_save.players[p].secretBirdHits; i++)
                    Blit2(i % 2 * 20 + bx, (i >> 1) * 22 + by, 0, g_gfxLogos, 112, 254, 19, 20);
            }
        }
    }

    // ---- counters and level ----
    if (g_save.players[p].highScoreMilestone) {
        sprintf(g_scoreBuf, "%03d", g_save.players[p].secretCount);
        DrawTinyText2(g_scoreBuf, x - 12, y5 - 26, 1);
    }
    if (g_save.players[p].gemCounterUnlocked) {
        sprintf(g_scoreBuf, "%03d",
            (g_save.players[p].gems - g_shipDefs[g_save.players[p].ship]->gemBase) /
                g_shipDefs[g_save.players[p].ship]->gemStep);
        DrawTinyText2(g_scoreBuf, x - 12, y5 - 11, 2);
    }

    if (x < HUD_SIDE_SPLIT_X)
        DrawTinyText2("LEVEL", x - 20, y5, 3);
    else
        DrawTinyText2("LEVEL", x - 20, y5, 4);
    sprintf(g_scoreBuf, "%04d", g_save.players[p].displayLevel);
    if (x < HUD_SIDE_SPLIT_X)
        DrawTinyText2(g_scoreBuf, x - 16, y5 + 9, 3);
    else
        DrawTinyText2(g_scoreBuf, x - 16, y5 + 9, 4);

    // ---- meteor meter ----
    if (g_time < g_meterShowUntil) {
        Blit2(meterX - 32, 0 - (int)g_meterY, 0, g_gfxMeteorMeter, 0, 0, 64, 600);
        scale = g_save.players[p].raceDistance / 455.0;
        if (scale == 0.0)
            scale = 1.0;
        if (x < HUD_SIDE_SPLIT_X)
            Blit2(meterX - 32, (int)(g_meterValue / scale) + 67 - (int)g_meterY, 0, g_gfxMeteorMeter,
                0, 619, 48, 18);
        else
            Blit2(meterX - 32, (int)(g_meterValue / scale) + 67 - (int)g_meterY, 0, g_gfxMeteorMeter,
                0, 600, 48, 18);
        if (g_meterV > 0.035f) {
            if (g_meterDirUp) {
                g_meterY = g_meterY - g_meterV;
                g_meterV = g_meterV * 0.935f;
            } else {
                g_meterY = g_meterY + g_meterV;
                g_meterV = g_meterV * 1.1f;
            }
        }
    }
    g_clipLeft = saved0;
    g_clipRight = saved1;
}

// Draws the 1-player time-trial HUD: player 1's score, the running hiscore, the countdown
// clock (clamped to 0 once it goes outside 0..10 minutes), and the HUD strip.
void DrawHudTimed()
{
    int len;
    float fmin;
    int mins;
    float secs;
    int ms;

    if (g_save.players[0].score > g_bestScore)
        g_bestScore = g_save.players[0].score;
    len = Int64ToStrGrouped(g_save.players[0].score, g_logBuf);
    DrawMenuText(g_logBuf, (g_screenW >> 1) - len * 6 + 12, 12, 1);
    DrawMenuText("PL1", (g_screenW >> 1) - len * 6 - 36, 12, 2);
    len = Int64ToStrGrouped(g_bestScore, g_logBuf);
    DrawMenuText(g_logBuf, (g_screenW >> 1) - len * 6 + 36, 0, 5);
    DrawMenuText("HISCORE", (g_screenW >> 1) - len * 6 - 60, 0, 2);
    fmin = (g_timeTrialDeadline - g_time) / 60000.0;
    mins = (g_timeTrialDeadline - g_time) / 60000;
    secs = (fmin - mins) * 60.0;
    ms = (g_timeTrialDeadline - g_time) - mins * 60 * 100;
    if ((int)fmin > 10 || (int)fmin < 0)
    {
        fmin = 0;
        secs = 0;
    }
    sprintf(g_scoreBuf, "%d:%02d", (int)fmin, (int)secs);
    DrawScoreDigits(g_scoreBuf, 0x148, 0x1c, 0, 200.0f);
    Hud(0, 0x20, g_screenW - 0x20);
}

// Draws the 1-player HUD: player 1's score, the running hiscore, and the HUD strip.
void DrawHud1P()
{
    int len;

    if (g_save.players[0].score > g_bestScore)
        g_bestScore = g_save.players[0].score;
    len = Int64ToStrGrouped(g_save.players[0].score, g_logBuf);
    DrawMenuText(g_logBuf, (g_screenW >> 1) - len * 6 + 12, 14, 1);
    DrawMenuText("PL1", (g_screenW >> 1) - len * 6 - 36, 14, 2);
    len = Int64ToStrGrouped(g_bestScore, g_logBuf);
    DrawMenuText(g_logBuf, (g_screenW >> 1) - len * 6 + 36, 2, 5);
    DrawMenuText("HISCORE", (g_screenW >> 1) - len * 6 - 60, 2, 2);
    Hud(0, 0x20, g_screenW - 0x20);
}

// Draws the 2-player (versus turn-based) HUD: both players' scores, the running hiscore,
// the "PL1"/"PL2" labels blinking on the active player, and the HUD strip for whichever
// player is active (or both, once the meteor meter has scrolled past y=500).
void DrawHud2P()
{
    int len;

    len = 0;
    if (g_save.players[0].score > g_bestScore)
        g_bestScore = g_save.players[0].score;
    if (g_save.players[1].score > g_bestScore)
        g_bestScore = g_save.players[1].score;

    len = Int64ToStrGrouped(g_save.players[0].score, g_logBuf);
    DrawMenuText(g_logBuf, 0x7a, 2, 1);
    if (g_curPlayer == 0 && g_activePlayerBlink != 0)
        DrawMenuText("PL1", 0x50, 2, 0);
    else
        DrawMenuText("PL1", 0x50, 2, 2);

    len = Int64ToStrGrouped(g_bestScore, g_logBuf);
    DrawMenuText(g_logBuf, (g_screenW >> 1) - len * 6 + 36, 2, 5);
    DrawMenuText("HISCORE", (g_screenW >> 1) - len * 6 - 60, 2, 2);

    len = Int64ToStrGrouped(g_save.players[1].score, g_logBuf);
    DrawMenuText(g_logBuf, g_screenW - 80 - len * 12, 2, 1);
    if (g_curPlayer == 1 && g_activePlayerBlink != 0)
        DrawMenuText("PL2", g_screenW - 80 - len * 12 - 42, 2, 0);
    else
        DrawMenuText("PL2", g_screenW - 80 - len * 12 - 42, 2, 2);

    if (g_time > g_activePlayerBlinkTime)
    {
        g_activePlayerBlinkTime = g_time + 100;
        g_activePlayerBlink = g_activePlayerBlink == 0;
    }

    if (g_state != STATE_BONUS_RACE && g_meterY > 500.0)
    {
        Hud(0, 0x20, g_screenW - 0x20);
        Hud(1, g_screenW - 0x20, 0x20);
    }
    else
    {
        if (g_curPlayer == 0)
            Hud(0, 0x20, g_screenW - 0x20);
        if (g_curPlayer == 1)
            Hud(1, g_screenW - 0x20, 0x20);
    }
}

// Draws the 2-player co-op HUD: both players' scores, the running hiscore, and the HUD
// strip for whichever player has the current turn (g_vsTurnPlayer), or both once the
// meteor meter has scrolled past y=500.
void DrawHud2PCoop()
{
    int len;

    if (g_save.players[0].score > g_bestScore)
        g_bestScore = g_save.players[0].score;
    if (g_save.players[1].score > g_bestScore)
        g_bestScore = g_save.players[1].score;

    len = Int64ToStrGrouped(g_save.players[0].score, g_logBuf);
    DrawMenuText(g_logBuf, 0x7a, 2, 1);
    DrawMenuText("PL1", 0x50, 2, 2);

    len = Int64ToStrGrouped(g_bestScore, g_logBuf);
    DrawMenuText(g_logBuf, (g_screenW >> 1) - len * 6 + 36, 2, 5);
    DrawMenuText("HISCORE", (g_screenW >> 1) - len * 6 - 60, 2, 2);

    len = Int64ToStrGrouped(g_save.players[1].score, g_logBuf);
    DrawMenuText(g_logBuf, g_screenW - 80 - len * 12, 2, 4);
    DrawMenuText("PL2", g_screenW - 80 - len * 12 - 42, 2, 2);

    if (g_state != STATE_BONUS_RACE && g_state != STATE_METEOR_STORM && g_meterY > 500.0)
    {
        Hud(0, 0x20, g_screenW - 0x20);
        Hud(1, g_screenW - 0x20, 0x20);
    }
    else
    {
        if (g_vsTurnPlayer == 0)
            Hud(0, 0x20, g_screenW - 0x20);
        if (g_vsTurnPlayer == 1)
            Hud(1, g_screenW - 0x20, 0x20);
    }
}

// Queues a floating score number at (x, y); `big` uses the larger, longer-lived popup
// style (for milestone bonuses) instead of the small in-line digit strip.
void AddScorePopup(int x, int y, __int64 value, bool big)
{
    int i;
    for (i = 0; i < MAX_POPUPS; i++) {
        if (g_popups[i].active == 0) {
            g_popups[i].active = 1;
            g_popups[i].life = 91.0f;
            g_popups[i].value = value;
            Int64ToStrGrouped(value, g_popups[i].text);
            g_popups[i].glyphGfx = g_numbersGfx;
            g_popups[i].x = (float)x;
            g_popups[i].y = (float)y;
            g_popups[i].vy = -0.3f;
            g_popups[i].big = big;
            if (big)
                g_popups[i].life = 221.0f;
            g_popups[i].blinkCounter = 1;
            g_popups[i].blinkPhase = 0;
            break;
        }
    }
}

// Ages and moves every active score popup, retiring it once its life or Y position runs
// out, and advances the "big" popups' blink animation.
void UpdateScorePopups()
{
    int i;
    for (i = 0; i < MAX_POPUPS; i++) {
        if (g_popups[i].active == 1) {
            g_popups[i].life -= 1.0;
            if (g_popups[i].life < 0.0)
                g_popups[i].active = 0;
            g_popups[i].y += g_popups[i].vy;
            if (g_popups[i].y < 0.0)
                g_popups[i].active = 0;
            if (g_popups[i].big && g_popups[i].life < 78.0) {
                g_popups[i].blinkCounter++;
                g_popups[i].blinkPhase += 4;
                if (g_popups[i].blinkPhase > g_popups[i].blinkCounter)
                    g_popups[i].blinkPhase = 0;
            }
        }
    }
}

// Draws every active score popup: small popups as a colored digit strip whose color
// changes past the 5000/10000/30000 point bands, "big" popups centered and blinking.
void DrawScorePopups()
{
    int i;
    int pos;
    int frame;
    int first;
    int color;
    char *p;
    float xoff;
    float yoff;
    float width;

    for (i = 0; i < MAX_POPUPS; i++) {
        if (g_popups[i].active == 1 && !g_popups[i].big) {
            xoff = 0;
            color = 1;
            if (g_popups[i].value > 4999)
                color = 2;
            if (g_popups[i].value > 9999)
                color = 3;
            if (g_popups[i].value > 29999)
                color = 0;

            first = 0;
            for (p = g_popups[i].text; *p; p++) {
                if (*p == '.') {
                    pos = 10;
                } else if (*p >= '0' && *p <= '9') {
                    pos = *p - '0';
                }
                frame = 7 - (int)(g_popups[i].life / 13.0);
                if (frame < 0)
                    frame = 0;
                if (frame > 6)
                    frame = 6;
                Blit((int)(g_popups[i].x + xoff), (int)g_popups[i].y,
                                  0, g_numbersGfx,
                                  first * 80 + pos * 8 + color * 168, frame * 9, 8, 9);
                if (*p == '.')
                    xoff += 4.0;
                else if (first == 0)
                    xoff += 7.0;
                else
                    xoff += 6.0;
                if (first == 0)
                    first = 1;
            }
        }

        if (g_popups[i].active == 1 && g_popups[i].big == 1) {
            width = 0;
            for (p = g_popups[i].text; *p; p++) {
                if (*p == '.') {
                    pos = 10;
                } else if (*p >= '0' && *p <= '9') {
                    pos = *p - '0';
                }
                width += g_digitW[pos] + 4;
            }

            xoff = 0 - width / 2.0;
            yoff = 0;
            for (p = g_popups[i].text; *p; p++) {
                if (*p == '.') {
                    pos = 10;
                } else if (*p >= '0' && *p <= '9') {
                    pos = *p - '0';
                }
                if (g_popups[i].blinkPhase < 3) {
                    Blit((int)(g_popups[i].x + xoff), (int)(g_popups[i].y + yoff),
                                      0, g_tinyFont,
                                      g_digitX[pos], 45, g_digitW[pos], 33);
                }
                xoff += g_digitW[pos] + 4;
            }
        }
    }
}

// Draws the boss health bar (when a boss is active): a fixed-size frame with a fill
// whose color shifts as health drops and which flickers faster the lower the boss's
// health ratio gets.
void DrawBossBar()
{
    int x = 0x55;
    int y = 0x1e;
    int sy = 0;
    int chance;
    float ratio;
    int hp;
    int i;
    float div;
    int j;

    if (g_gameMode == MODE_DUAL)
        g_curPlayer = 0;

    if (g_bossIdx != -1) {
        hp = (int)g_enemies[g_curPlayer][g_bossIdx].hp;
        for (i = 0; i < 15; i++)
            Blit(x + i * 8, y, 0, g_gfxLogos, 0x30, 0x39, 8, 0xd);

        if (g_enemies[g_curPlayer][g_bossIdx].maxHp == 0.0)
            g_enemies[g_curPlayer][g_bossIdx].maxHp = 1;
        div = g_enemies[g_curPlayer][g_bossIdx].maxHp;
        if (div == 0.0)
            div = 1;
        ratio = g_enemies[g_curPlayer][g_bossIdx].hp / div;

        sy = 0x50;
        chance = 40000;
        if (ratio < 0.66)
            sy = 0x40;
        if (ratio < 0.33)
            sy = 0x30;
        if (ratio < 0.2)
            chance = 10;
        if (ratio < 0.1)
            chance = 2;

        if (RandRange(0, chance)) {
            for (j = 0; j < (int)(ratio * 80.0) / 2; j++)
                Blit(x + j * 2 + 0x25, y + 2, 0, g_gfxLogos, sy, 0x30, 2, 9);
        }
        DrawTinyText("BOSS", x + 3, y + 3, 1);
    }
}
