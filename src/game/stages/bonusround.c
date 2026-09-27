// bonusround.c: Bonus waves: the results HUD (and the timed HUD messages it draws), banking the
// bonus score, special-level test.
#include <stdio.h>
#include "globals.h"
#include "game.h"

// Base "not perfect" bonus-round score, awarded when the results screen times out before
// every alien was caught.
enum { NOT_PERFECT_BONUS = 10000 };

// Score below which a PERFECT award instead re-prices from the combo table (below), and the
// per-combo-level unit it's scaled by.
enum { COMBO_THRESHOLD = 10000000, COMBO_UNIT_SCORE = 1000 };

// Score awarded per alien in the results tally.
enum { TALLY_ALIEN_SCORE = 500 };

// Combo bonus levels 0..9, each a multiplier for COMBO_UNIT_SCORE.
static const __int64 g_comboBonusTable[10] = { 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000 };

// True for bonus-round/race/warp level types (2, 3, 4), unless in time-trial mode. `unused` is
// never read.
int IsSpecialLevel(int unused)
{
    if (g_gameMode == MODE_TIME_TRIAL)
        return 0;
    if (g_curLevelData.type == LEVEL_BOSS || g_curLevelData.type == LEVEL_RACE
            || g_curLevelData.type == LEVEL_BONUS_WAVE)
        return 1;
    return 0;
}

// Called when leaving a bonus stage: banks the bonus-round score into the player's
// bonus high score and restores the hyperspace/scroll-speed state saved before the
// bonus stage. In 2-player vs mode this updates both players but restores from
// g_vsTurnPlayer's saved state into the shared slot 0.
void BankBonusScore()
{
    if (g_gameMode == MODE_DUAL) {
        if (g_save.players[0].bonusRoundScore > g_save.players[0].bonusHighScore)
            g_save.players[0].bonusHighScore = g_save.players[0].bonusRoundScore;
        g_save.players[1].energy = 0;
        if (g_save.players[1].bonusRoundScore > g_save.players[1].bonusHighScore)
            g_save.players[1].bonusHighScore = g_save.players[1].bonusRoundScore;

        // Restore the scroll/hyperspace state into the shared slot 0 from whichever
        // player's turn was saved.
        RESTORE_HYPERSPACE(g_save.players[0], g_save.players[g_vsTurnPlayer])
    } else {
        if (g_save.players[g_curPlayer].bonusRoundScore > g_save.players[g_curPlayer].bonusHighScore)
            g_save.players[g_curPlayer].bonusHighScore = g_save.players[g_curPlayer].bonusRoundScore;

        RESTORE_HYPERSPACE(g_save.players[g_curPlayer], g_save.players[g_curPlayer])
    }
}

// Draws whichever queued HUD text is currently timed in (banner, tiny option message, warp-alert
// blink, and colored alert message), then drives the bonus-level results screen: shows the tally
// of aliens caught, awards/animates the PERFECT bonus (score, streak, fireworks), and ticks the
// kill tally toward its target with its accompanying sounds. Called every frame while any of these
// timers or the results screen are active.
void UpdateBonusResultsHud()
{
    int y = 0x82;   // base y for the results-screen flare particles / "RESULTS" heading (130)
    int len = 0;
    char buf[2];
    int col;
    int y2;
    int p;
    int i;
    int played;

    buf[0] = 0;
    buf[1] = 0;
    // ---- timed messages ----
    if (g_bannerMsgTime != 0 && g_state != STATE_PAUSED) {
        if (g_time > g_bannerMsgTime)
            g_bannerMsgTime = 0;
        else
            DrawMenuText(g_bannerMsg, POS_CENTERED, 50, 5);
    }

    if (g_msgTime != 0 && g_state != STATE_PAUSED) {
        if (g_time > g_msgTime)
            g_msgTime = 0;
        else if (g_gameMode != MODE_DUAL && g_gameMode != MODE_TWO_PLAYER)
            DrawTinyText(g_optionMsg, 67, 1, 4);
        else
            DrawTinyText(g_optionMsg, POS_CENTERED, 15, 4);
    }

    if (g_alertTextTime != 0 && g_state != STATE_PAUSED) {
        if (g_time > g_alertTextTime)
            g_alertTextTime = 0;
        else {
            if (g_time > g_warpMsgBlinkTime) {
                g_warpMsgBlinkTime = g_time + 70;
                g_warpMsgBlinkOn = !g_warpMsgBlinkOn;
            }
            if (g_warpMsgBlinkOn)
                DrawMenuText(g_warpMalfunctionMsg, POS_CENTERED, 300, 0);
            else
                DrawMenuText(g_warpMalfunctionMsg, POS_CENTERED, 300, 2);
        }
    }

    if (g_msgTimer != 0 && g_state != STATE_PAUSED) {
        if (g_time > g_msgTimer)
            g_msgTimer = 0;
        else {
            col = g_msgColor;
            if (col == 8)
                col = RandRange(0, 7);
            DrawMenuText(g_alertMsg, POS_CENTERED, 400, col);
        }
    }

    if (g_levelBannerTime != 0) {
        if (g_time > g_levelBannerTime)
            g_levelBannerTime = 0;
        else {
            y2 = 250;
            DrawMenuText(g_getReadyText, POS_CENTERED, y2, 2);
            DrawMenuText(g_levelBannerText, POS_CENTERED, y2 + 25, 5);
            DrawMenuText(g_levelName, POS_CENTERED, y2 + 50, 0);
        }
    }

    // ---- timeout fallback ----
    if (g_bonusResultsTime != 0) {
        if (g_time > g_bonusResultsTime && g_state != STATE_PAUSED) {
            // Results screen has timed out: if the player didn't clear every alien and hasn't
            // already been awarded the PERFECT bonus this round, fall back to the base "not
            // perfect" bonus-round score and reset the combo.
            if (g_gameMode != MODE_DUAL) {
                if (g_save.players[g_curPlayer].bonusKilled < g_save.players[g_curPlayer].totalEnemies &&
                    g_perfectAwardedP0 == 0) {
                    g_save.players[g_curPlayer].bonusRoundPoints = NOT_PERFECT_BONUS;
                    g_save.players[g_curPlayer].perfectStreak = 0;
                    g_comboLevel = 0;
                    g_comboStep = 0;
                }
            } else {
                if (g_save.players[0].bonusKilled < g_save.players[0].totalEnemies
                    && g_perfectAwardedP0 == 0) {
                    g_save.players[0].bonusRoundPoints = NOT_PERFECT_BONUS;
                    g_save.players[0].perfectStreak = 0;
                    g_comboLevel = 0;
                    g_comboStep = 0;
                }

                if (g_save.players[1].bonusKilled < g_save.players[1].totalEnemies
                    && g_perfectAwardedP1 == 0) {
                    g_save.players[1].bonusRoundPoints = NOT_PERFECT_BONUS;
                    g_save.players[1].perfectStreak = 0;
                    g_comboLevel = 0;
                    g_comboStep = 0;
                }
            }
            g_bonusResultsTime = 0;
            g_save.players[g_curPlayer].trackKillsFlag = 0;
            g_save.players[g_curPlayer].bonusResultsInitDone = 0;

        } else {
            // ---- corner flares ----
            if (g_state != STATE_PAUSED) {
                // Roughly a 1-in-(frames per second) chance per frame, i.e. about once a second
                // on average: spawn one flare particle at each corner of the results box.
                if (RandRange(0, (int)(1 * (1 / g_frameDt))) == 0) {
                    AddParticle(g_gfxFlare5, (int)g_bonusFxX, y, 60.0f, -0.2f, RandFloat(0, 359.0f),
                                       RandFloat(0, 30.0f) - 15.0, RandRange(0, 359),
                                       RandRange(0, 100), RandRange(0, 100), 255, 255,
                                       RandFloat(1, 3.0f) * 20.0, RandFloat(0, 3.0f), -1, 0.1f, 0, 0, 0, 1);
                    AddParticle(g_gfxFlare5, g_screenW - (int)g_bonusFxX, y, 60.0f, -0.2f,
                                       RandFloat(0, 359.0f),
                                       RandFloat(0, 30.0f) - 15.0, RandRange(0, 359),
                                       RandRange(0, 100), RandRange(0, 100), 255, 255,
                                       RandFloat(1, 3.0f) * 20.0, RandFloat(0, 3.0f), -1, 0.1f, 0, 0, 0, 1);
                    AddParticle(g_gfxFlare5, g_screenW - (int)g_bonusFxX, y + 42, 60.0f, -0.2f,
                                       RandFloat(0, 359.0f),
                                       RandFloat(0, 30.0f) - 15.0, RandRange(0, 359),
                                       RandRange(0, 100), RandRange(0, 100), 255, 255,
                                       RandFloat(1, 3.0f) * 20.0, RandFloat(0, 3.0f), -1, 0.1f, 0, 0, 0, 1);
                    AddParticle(g_gfxFlare5, (int)g_bonusFxX, y + 42, 60.0f, -0.2f, RandFloat(0, 359.0f),
                                       RandFloat(0, 30.0f) - 15.0, RandRange(0, 359),
                                       RandRange(0, 100), RandRange(0, 100), 255, 255,
                                       RandFloat(1, 3.0f) * 20.0, RandFloat(0, 3.0f), -1, 0.1f, 0, 0, 0, 1);
                }

                g_bonusFxX = g_bonusFxX + g_bonusFxVel;
                if ((int)g_bonusFxX > (int)(g_screenW - 100)) {
                    g_bonusFxX = g_screenW - 100;
                    g_bonusFxVel = 0 - g_bonusFxVel;
                }
                if ((int)g_bonusFxX < 100) {
                    g_bonusFxX = 100;
                    g_bonusFxVel = 0 - g_bonusFxVel;
                }
            }

            // ---- results text ----
            DrawMenuText("B O N U S   L E V E L   R E S U L T S", POS_CENTERED, y + 20, 2);
            if (g_gameMode == MODE_DUAL) {
                if (g_save.players[0].bonusKilled > g_save.players[0].totalEnemies)
                    g_save.players[0].bonusKilled = g_save.players[0].totalEnemies;
                if (g_save.players[1].bonusKilled > g_save.players[1].totalEnemies)
                    g_save.players[1].bonusKilled = g_save.players[1].totalEnemies;
                g_textCursorY = g_textCursorY + 40;
                sprintf(g_logBuf, " PLAYER ONE GOT %d OF %d ALIENS", g_save.players[0].bonusTally,
                        g_save.players[0].totalEnemies);
                DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 1);
                g_textCursorY = g_textCursorY + 15;
                sprintf(g_logBuf, " %d X 500 = %d POINTS", g_save.players[0].bonusTally,
                        g_save.players[0].bonusTally * 500);
                DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 1);

                g_textCursorY = g_textCursorY + 80;
                sprintf(g_logBuf, " PLAYER TWO GOT %d OF %d ALIENS", g_save.players[1].bonusTally,
                        g_save.players[1].totalEnemies);
                DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 4);
                g_textCursorY = g_textCursorY + 15;
                sprintf(g_logBuf, " %d X 500 = %d POINTS", g_save.players[1].bonusTally,
                        g_save.players[1].bonusTally * 500);
                DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 4);
            } else {
                if (g_save.players[g_curPlayer].bonusKilled > g_save.players[g_curPlayer].totalEnemies)
                    g_save.players[g_curPlayer].bonusKilled = g_save.players[g_curPlayer].totalEnemies;
                g_textCursorY = g_textCursorY + 40;
                sprintf(g_logBuf, " YOU GOT %d OF %d ALIENS", g_save.players[g_curPlayer].bonusTally,
                        g_save.players[g_curPlayer].totalEnemies);
                DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 1);
                g_textCursorY = g_textCursorY + 20;
                sprintf(g_logBuf, " %d X 500 = %d POINTS", g_save.players[g_curPlayer].bonusTally,
                        g_save.players[g_curPlayer].bonusTally * 500);
                DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 1);
                g_textCursorY = g_textCursorY + 30;
            }

            // ---- perfect check ----
            p = -1;
            g_perfectAny = 0;
            if (g_gameMode != MODE_DUAL) {
                if (g_save.players[g_curPlayer].bonusTally >= g_save.players[g_curPlayer].totalEnemies &&
                    g_time > g_perfectCheckDelay && g_state != STATE_PAUSED) {
                    g_perfectAny = 1;
                    g_perfectAwardedP0 = 1;
                } else
                    g_perfectAwardedP0 = 0;

                p = g_curPlayer;
            } else {
                if (g_save.players[0].bonusTally >= g_save.players[0].totalEnemies &&
                    g_time > g_perfectCheckDelay && g_state != STATE_PAUSED) {
                    p = 0;
                    g_perfectAny = 1;
                    g_perfectAwardedP0 = 1;
                } else
                    g_perfectAwardedP0 = 0;

                if (g_save.players[1].bonusTally >= g_save.players[0].totalEnemies &&
                    g_time > g_perfectCheckDelay && g_state != STATE_PAUSED) {
                    p = 1;
                    g_perfectAny = 1;
                    g_perfectAwardedP1 = 1;
                } else
                    g_perfectAwardedP1 = 0;
            }

            // ---- PERFECT award ----
            if (g_perfectAny != 0) {
                if (g_gameMode == MODE_DUAL) {
                    if (p == 0)
                        y = 100;
                    else
                        y = 220;
                } else
                    y = 130;
                // Draw the "PERFECT" banner (plus the score digits already merged into g_perfect
                // below) one character at a time so each glyph can cycle through its own color.
                for (i = 0; i < 32; i++) {
                    buf[0] = g_perfect[i];
                    DrawMenuText(buf,
                                 (int)((g_screenW >> 1) - g_save.players[p].perfectTextSpacing * 16.0 +
                                       i * g_save.players[p].perfectTextSpacing),
                                 y + 160, g_perfectColorTable[g_colorPhase3 + i]);
                }

                if (g_save.players[p].perfectDone == 0) {
                    g_save.players[p].perfectDone = 1;
                    ADD_PLAYER_SCORE(g_save.players[p].score, p, g_save.players[p].bonusRoundPoints);

                    g_save.players[p].bonusRoundCount += 1.0;
                    // Right-align the point total into the blank digit slots of the "PERFECT"
                    // banner text (10 chars wide, starting at offset 14).
                    len = Int64ToStrGrouped(g_save.players[p].bonusRoundPoints, g_bonusNumBuf);
                    CopyBytesAt(g_perfect, g_perfectBlank, 14, 10);
                    CopyBytesAt(g_perfect, g_bonusNumBuf, 10 - len + 14, len);
                    if (g_save.players[p].bonusRoundPoints < COMBO_THRESHOLD) {
                        // Below the combo threshold: advance the combo level (capped at 9) and
                        // reprice this round's bonus from the combo table instead.
                        if (g_comboStep > 0) {
                            g_comboLevel = g_comboLevel + g_comboStep;
                            if (g_comboLevel > 9)
                                g_comboLevel = 9;
                            g_save.players[p].bonusRoundPoints =
                                g_comboBonusTable[g_comboLevel] * COMBO_UNIT_SCORE;
                        }
                        g_comboStep = 0;
                    }

                    g_save.players[p].perfectStreak++;
                    if (g_save.players[p].perfectStreak > 1 && g_profileIndex != -1 &&
                        g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                        g_save.players[p].secretFound22 = 1;
                        MarkSecretFound(g_profileIndex, 22);
                    }

                    g_perfectCount++;
                    g_bonusResultsTime = g_time + 4000;
                    SoundPlay(g_sfxAlright, -1, 255, 0, 223, g_sndFlags);
                    SoundPlay(g_sfxOrkHit, -1, 255, 0, 223, g_sndFlags);
                }

                if (RandRange(0, 100) < 2 && g_state != STATE_PAUSED) {
                    SpawnFirework();
                    SoundPlay(g_sfxExplo3, -1, RandRange(30, 100), g_panTable[ClampX(400)], 127, g_sndFlags);
                }
            }

            // ---- tally: single vs dual ----
            played = 0;
            if (g_state != STATE_PAUSED) {
                if (g_gameMode != MODE_DUAL) {
                    g_save.players[g_curPlayer].bonusTallyDelay =
                        g_save.players[g_curPlayer].bonusTallyDelay - 1 * g_frameDt;
                    if (g_save.players[g_curPlayer].bonusTallyDelay < 0.0) {
                        g_save.players[g_curPlayer].bonusTallyDelay = 1;
                        g_save.players[g_curPlayer].perfectTextSpacing =
                            g_save.players[g_curPlayer].perfectTextSpacing +
                            g_save.players[g_curPlayer].perfectTextSpacingVel;
                        if (g_save.players[g_curPlayer].perfectTextSpacing < 7.0)
                            g_save.players[g_curPlayer].perfectTextSpacingVel =
                                -g_save.players[g_curPlayer].perfectTextSpacingVel;
                        if (g_save.players[g_curPlayer].perfectTextSpacing > 20.0)
                            g_save.players[g_curPlayer].perfectTextSpacingVel =
                                -g_save.players[g_curPlayer].perfectTextSpacingVel;
                    }

                    // Post-decrement: fires (and resets the 3-frame divider) on the frame the
                    // counter reaches 0, i.e. once every 3 frames while the tally is still behind.
                    if (!g_save.players[g_curPlayer].bonusTallyTick--) {
                        g_save.players[g_curPlayer].bonusTallyTick = 3;
                        if (g_save.players[g_curPlayer].bonusTally
                            < g_save.players[g_curPlayer].bonusKilled) {
                            ADD_PLAYER_SCORE(g_save.players[g_curPlayer].score, g_curPlayer,
                                TALLY_ALIEN_SCORE);

                            g_save.players[g_curPlayer].bonusTally++;
                            if (g_save.players[g_curPlayer].bonusTally
                                >= g_save.players[g_curPlayer].bonusKilled)
                                g_save.players[g_curPlayer].bonusTally =
                                    g_save.players[g_curPlayer].bonusKilled;
                            g_perfectCheckDelay = g_time + 1000;
                            g_bonusResultsTime = g_time + 4000;
                            SoundPlay(g_sfxBell3, -1, 155, 0, 127, g_sndFlags);
                        }
                    }

                } else {
                    g_save.players[0].bonusTallyDelay = g_save.players[0].bonusTallyDelay - 1 * g_frameDt;
                    if (g_save.players[0].bonusTallyDelay < 0.0) {
                        g_save.players[0].bonusTallyDelay = 1;
                        g_save.players[0].perfectTextSpacing =
                            g_save.players[0].perfectTextSpacing + g_save.players[0].perfectTextSpacingVel;
                        if (g_save.players[0].perfectTextSpacing < 7.0)
                            g_save.players[0].perfectTextSpacingVel =
                                -g_save.players[0].perfectTextSpacingVel;
                        if (g_save.players[0].perfectTextSpacing > 20.0)
                            g_save.players[0].perfectTextSpacingVel =
                                -g_save.players[0].perfectTextSpacingVel;
                    }

                    if (!g_save.players[0].bonusTallyTick--) {
                        g_save.players[0].bonusTallyTick = 3;
                        if (g_save.players[0].bonusTally < g_save.players[0].bonusKilled) {
                            ADD_PLAYER_SCORE(g_save.players[0].score, 0, TALLY_ALIEN_SCORE);

                            g_save.players[0].bonusTally++;
                            if (g_save.players[0].bonusTally >= g_save.players[0].bonusKilled)
                                g_save.players[0].bonusTally = g_save.players[0].bonusKilled;
                            g_perfectCheckDelay = g_time + 1000;
                            g_bonusResultsTime = g_time + 4000;
                            played = 1;
                        }
                    }

                    g_save.players[1].bonusTallyDelay = g_save.players[1].bonusTallyDelay - 1 * g_frameDt;
                    if (g_save.players[1].bonusTallyDelay < 0.0) {
                        g_save.players[1].bonusTallyDelay = 1;
                        g_save.players[1].perfectTextSpacing =
                            g_save.players[1].perfectTextSpacing + g_save.players[1].perfectTextSpacingVel;
                        if (g_save.players[1].perfectTextSpacing < 7.0)
                            g_save.players[1].perfectTextSpacingVel =
                                -g_save.players[1].perfectTextSpacingVel;
                        if (g_save.players[1].perfectTextSpacing > 20.0)
                            g_save.players[1].perfectTextSpacingVel =
                                -g_save.players[1].perfectTextSpacingVel;
                    }

                    if (!g_save.players[1].bonusTallyTick--) {
                        g_save.players[1].bonusTallyTick = 3;
                        if (g_save.players[1].bonusTally < g_save.players[1].bonusKilled) {
                            ADD_PLAYER_SCORE(g_save.players[1].score, 1, TALLY_ALIEN_SCORE);

                            g_save.players[1].bonusTally++;
                            if (g_save.players[1].bonusTally >= g_save.players[1].bonusKilled)
                                g_save.players[1].bonusTally = g_save.players[1].bonusKilled;
                            g_perfectCheckDelay = g_time + 1000;
                            g_bonusResultsTime = g_time + 4000;
                            played = 1;
                        }
                    }

                    if (played != 0)
                        SoundPlayNoFade(g_sfxBell3, -1, 155, 0, 127, g_sndFlags);
                }
            }
        }
    }
}
