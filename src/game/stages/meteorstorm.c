// meteorstorm.c: The meteor-storm bonus stage (race and results).
#include <stdio.h>
#include "globals.h"
#include "game.h"

// Race-result score/money tiers (drunk mode has bigger rewards), keyed by final speed %.
enum {
    DRUNK_MEGA_SCORE = 20000000,
    DRUNK_MEGA_MONEY = 50000,
    DRUNK_EXTREME_SCORE = 10000000,
    DRUNK_EXTREME_MONEY = 10000,
    DRUNK_SUPER_SCORE = 5000000,
    DRUNK_SUPER_MONEY = 5000,
    DRUNK_EXTRA_SCORE = 2000000,
    MEGA_SCORE = 10000000,
    MEGA_MONEY = 25000,
    EXTREME_SCORE = 5000000,
    EXTREME_MONEY = 5000,
    SUPER_SCORE = 2000000,
    SUPER_MONEY = 1000,
    EXTRA_SCORE = 1000000,
};

// Bonus-meteor pickup score/money tiers for MeteorStormCollide below.
enum {
    METEOR_MONEY_50 = 50,
    METEOR_MONEY_100 = 100,
    METEOR_MONEY_250 = 250,
    METEOR_SCORE_1000 = 1000,
    METEOR_SCORE_5000 = 5000,
    METEOR_SCORE_10000 = 10000,
    GEM_SCORE_2500 = 2500,
    GEM_SCORE_5000 = 5000,
    GEM_SCORE_10000 = 10000,
};

// Gate + one-time setup for the bonus meter shown on the results screen: while
// g_time < g_resultsScreenEndTime it (re)arms the meter's starting position/velocity and
// keeps the play-time stat timer running; once it expires it releases the transition lock.
void UpdateMeteorStormIntroGate()
{
    g_raceActive = 0;
    g_introGateScratch = 1;
    if (g_time < g_resultsScreenEndTime) {
        if (!g_bonusMeterStarted) {
            g_meterY = 550.0f;
            g_meterV = 35.0f;
            g_meterDirUp = 1;
            g_bonusMeterStarted = 1;
        }
        g_raceActive = 1;
        g_introGateScratch = 0;
        if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) &&
            g_playerUpdateFn != (void *)StateDemo)
            TimerStart2();
    } else {
        g_transitionLock = 0;
    }
}

// One result-screen bonus-tier text block for DrawMeteorStorm below: a title/subtitle pair
// followed by the meteor-bonus and speed-percentage lines. #undef'd after the function.
#define TIER_TEXT(title, sub)                                                       \
    DrawMenuText(title, POS_CENTERED, cy - 44, 0);                                  \
    DrawMenuText(sub, POS_CENTERED, cy - 16, 3);                                    \
    sprintf(g_logBuf, "METEOR BONUS  %d POINTS", g_save.players[g_curPlayer].meteorBonus); \
    DrawMenuText(g_logBuf, POS_CENTERED, cy + 12, 1);                               \
    sprintf(g_logBuf, "SPEED PERCENTAGE  %d", (int)g_speedPct);                     \
    DrawMenuText(g_logBuf, POS_CENTERED, cy + 44, 1);

// Draws the meteor-storm minigame's intro ("GET READY") and result screens. The result
// screen's bonus tier (mega/extreme/super/extra, with points and credits) depends on the
// player's final speed percentage and whether the boost was held to the end, with separate
// (larger) rewards in "drunk" mode. Also refreshes the on-HUD race meter's show timer.
void DrawMeteorStorm()
{
    int n = 50;
    int cy = (int)g_screenH / 2;

    if (g_gameMode == MODE_DUAL) {
        g_curPlayer = g_vsTurnPlayer;
    }
    g_meterShowUntil = g_time + 3000;

    if (g_raceActive != 0 && g_raceDistanceLeft > 1.0) {
        if (g_cfg.particlesOn == 0) {
            for (int i = 0; i < n; i++) {
                DrawLine(100.0f, (float)(cy - n * 3 / 2 + i * 3), g_screenW - 100.0,
                    (float)(cy - n * 3 / 2 + i * 3), (i * 3 + 50) / 255.0, 0, 0.5f, 1.0f);
            }
        } else if (g_time > g_nextSpark) {
            g_nextSpark = g_time + 8;
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) - 100, 3.0f, 0, 90.0f, 0, 0, 255, 0, 0, 70,
                               150.0f, 0, -1, RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) + 100, 3.0f, 0, 90.0f, 0, 0, 255, 0, 0, 70,
                               150.0f, 0, -1, 0 - RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);
        }
        DrawMenuText("M E T E O R S T O R M", POS_CENTERED, cy - 22, 0);
        DrawMenuText("G E T   R E A D Y", POS_CENTERED, cy + 10, 0);
    }

    if (g_raceActive != 0 && g_raceDistanceLeft == 0.0) {
        if (g_cfg.particlesOn == 0) {
            for (int i = 0; i < n; i++) {
                DrawLine(100.0f, (float)(cy - n * 3 / 2 + i * 3), g_screenW - 100.0,
                    (float)(cy - n * 3 / 2 + i * 3), (i * 3 + 50) / 255.0, 0, 0.5f, 1.0f);
            }
        } else if (g_time > g_nextSpark) {
            g_nextSpark = g_time + 8;
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) - 100, 3.0f, 0, 90.0f, 0, 0, 255, 0, 0, 70,
                               150.0f, 0, -1, RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) + 100, 3.0f, 0, 90.0f, 0, 0, 255, 0, 0, 70,
                               150.0f, 0, -1, 0 - RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);

            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) - 101, 2.0f, 0, 90.0f, 0, 0, 255, 0, 255, 40,
                               80.0f, 0, -1, 0 - RandFloat(0.01f, 0.02f), 1, 0, &g_introGateScratch, 0);
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) + 101, 2.0f, 0, 90.0f, 0, 0, 255, 0, 255, 40,
                               80.0f, 0, -1, RandFloat(0.01f, 0.02f), 1, 0, &g_introGateScratch, 0);
        }

        if (g_drunk) {
            if ((int)g_speedPct == 100 && !g_boostReleased) {
                TIER_TEXT("DRUNK MEGA METEORSTORM BONUS", "20.000.000 POINTS  AND  50.000 CREDITS")
            } else if (g_speedPct >= 99.0) {
                TIER_TEXT("DRUNK EXTREME METEORSTORM BONUS", "10.000.000 POINTS  AND  10.000 CREDITS")
            } else if (g_speedPct >= 90.0) {
                TIER_TEXT("DRUNK SUPER METEORSTORM BONUS", "5.000.000 POINTS  AND  5.000 CREDITS")
            } else if (g_speedPct < 90.0) {
                TIER_TEXT("DRUNK EXTRA METEORSTORM BONUS", "2.000.000 POINTS")
            }
        } else {
            if ((int)g_speedPct == 100 && !g_boostReleased) {
                TIER_TEXT("MEGA METEORSTORM BONUS", "10.000.000 POINTS  AND  25.000 CREDITS")
            } else if (g_speedPct >= 99.0) {
                TIER_TEXT("EXTREME METEORSTORM BONUS", "5.000.000 POINTS  AND  5.000 CREDITS")
            } else if (g_speedPct >= 90.0) {
                TIER_TEXT("SUPER METEORSTORM BONUS", "1.000.000 POINTS  AND  1.000 CREDITS")
            } else {
                TIER_TEXT("EXTRA METEORSTORM BONUS", "1.000.000 POINTS")
            }
        }
    }

    if (g_raceActive != 0 && g_raceDistanceLeft == 1.0) {
        if (g_cfg.particlesOn == 0) {
            for (int i = 0; i < n; i++) {
                DrawLine(100.0f, (float)(cy - n * 3 / 2 + i * 3), g_screenW - 100.0,
                    (float)(cy - n * 3 / 2 + i * 3), (i * 3 + 50) / 255.0, 0, 0.5f, 1.0f);
            }
        } else if (g_time > g_nextSpark) {
            g_nextSpark = g_time + 8;
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) - 100, 3.0f, 0, 90.0f, 0, 0, 255, 0, 0, 70,
                               150.0f, 0, -1, RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) + 100, 3.0f, 0, 90.0f, 0, 0, 255, 0, 0, 70,
                               150.0f, 0, -1, 0 - RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);
        }
        DrawMenuText("METEORSTORM BONUS", POS_CENTERED, cy - 22, 0);
        sprintf(g_logBuf, "%d POINTS", g_save.players[g_curPlayer].meteorBonus);
        DrawMenuText(g_logBuf, POS_CENTERED, cy, 1);
        sprintf(g_logBuf, "SPEED PERCENTAGE  %d", (int)g_speedPct);
        DrawMenuText(g_logBuf, POS_CENTERED, cy + 22, 1);
    }
}

#undef TIER_TEXT

// Per-frame render for the meteor-storm race (STATE_BONUS_RACE): background, slots,
// explosions, sparks, bonus objects, sprites, ship HUD, score popups, particles, flash,
// meteor storm, the warp ring, borders, the speed meter (outside an active race) and HUD.
void RenderMeteorStormRaceFrame()
{
    g_stateFn();
    DrawBackground();
    DrawSlots(g_clipLeft, g_clipRight, g_clipTop, g_clipBottom);
    DrawExplosions();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
    DrawMeteors();
    DrawSprites();
    g_shipHudFn();
    DrawScorePopups();
    UpdateBonusResultsHud();
    DrawParticles();
    DrawFlash();
    DrawMeteorStorm();
    if (g_state == STATE_PAUSED)
        DrawWarpRing();
    g_drawBordersFn();
    if (g_raceActive == 0)
        DrawSpeedMeter();
    g_drawHudFn();
}

// Per-frame render for the meteor-storm bonus stage: background, slots, explosions,
// sparks, ship HUD, bonus HUD, particles, flash, the meteor storm itself, warp ring,
// borders, speed meter and HUD.
void RenderMeteorStormResultsFrame()
{
    g_stateFn();
    DrawBackground();
    DrawSlots(g_clipLeft, g_clipRight, g_clipTop, g_clipBottom);
    DrawExplosions();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
    g_shipHudFn();
    UpdateBonusResultsHud();
    DrawParticles();
    DrawFlash();
    DrawMeteorStorm();
    if (g_state == STATE_PAUSED)
        DrawWarpRing();
    g_drawBordersFn();
    DrawSpeedMeter();
    g_drawHudFn();
}

// (Re)spawns one meteor/bonus-item slot for the meteor bonus level: with g_bonusRareChance% odds
// picks a rare item (a big diamond, or a weighted-random bonus meteor), otherwise a normal rock
// with a graphic/size/hitbox looked up by random index. Sets the sprite source rect and gives it a
// random starting position above the screen with a small downward/lateral drift.
void SpawnMeteor(int idx)
{
    int bonusrnd[6] = { 50, 30, 10, 150, 80, 40 };  // relative spawn weights for the 6 bonus-meteor kinds
    int k;
    int sum;
    int i;
    int r;
    int m;

    k = 0;
    g_bonusMeteors[idx].active = 0;
    if (RandRange(0, 99) < g_bonusRareChance) {
        if (RandRange(0, 99) < 50) {
            // Diamond: 4 color variants, laid out side-by-side in the sheet.
            k = RandRange(0, 3);
            g_bonusMeteors[idx].graphic = g_gfxDiamondBig;
            g_bonusMeteors[idx].hma = g_hmaDiamondBig;
            g_bonusMeteors[idx].hmaW = g_diamondBigGfxW;
            g_bonusMeteors[idx].hmaH = g_diamondBigGfxH;
            g_bonusMeteors[idx].sx = k * 0x50;
            g_bonusMeteors[idx].sy = 0;
            g_bonusMeteors[idx].h = 0x33;
            g_bonusMeteors[idx].w = 0x50;
            g_bonusMeteors[idx].type = 2;
            g_bonusMeteors[idx].animTimer = 5.0f;
            g_bonusMeteors[idx].vol = 0;

        } else {
            g_bonusMeteors[idx].graphic = g_gfxMeteorBonuses;
            g_bonusMeteors[idx].hma = g_hmaMeteorBonuses;
            g_bonusMeteors[idx].hmaW = g_meteorBonusesGfxW;
            g_bonusMeteors[idx].hmaH = g_meteorBonusesGfxH;
            // Weighted pick among the 6 bonus-meteor kinds, same bucket-walk technique as InitGrid.
            sum = 0;
            for (i = 0; i < 6; i++)
                sum = sum + bonusrnd[i];
            r = RandRange(0, sum - 1);
            while (r > bonusrnd[k]) {
                r = r - bonusrnd[k];
                k++;
            }

            // NOTE: dead clamp copied from the 46-variant meteor picker below; k can't exceed 5
            // here since bonusrnd[] only has 6 entries. Kept as-is for the byte match.
            if (k > NUM_METEOR_VARIANTS - 1)
                k = NUM_METEOR_VARIANTS - 1;
            if (k < 0)
                k = 0;
            g_bonusMeteors[idx].sx = k * 64;
            g_bonusMeteors[idx].sy = 0;
            g_bonusMeteors[idx].h = 0x25;
            g_bonusMeteors[idx].w = 0x40;
            g_bonusMeteors[idx].type = METEOR_MONEY;
            g_bonusMeteors[idx].animTimer = 5.0f;
            g_bonusMeteors[idx].vol = 0;
        }

    } else {
        // Normal rock: one of 46 pre-defined meteor variants, looked up by index.
        g_bonusMeteors[idx].graphic = g_bgGraphic;
        g_bonusMeteors[idx].hma = g_hmaMeteors;
        g_bonusMeteors[idx].hmaW = g_meteorsGfxW;
        g_bonusMeteors[idx].hmaH = g_meteorsGfxH;
        m = RandRange(0, NUM_METEOR_VARIANTS - 1);
        g_bonusMeteors[idx].sx = g_meteorSrcX[m];
        g_bonusMeteors[idx].sy = g_bonusSrcY[m];
        g_bonusMeteors[idx].h = g_bonusGfxH[m];
        g_bonusMeteors[idx].w = g_bonusGfxW[m];
        g_bonusMeteors[idx].type = METEOR_HAZARD;
        g_bonusMeteors[idx].vol = g_bonusVolume[idx];
    }
    g_bonusMeteors[idx].x1_38 = g_bonusMeteors[idx].sx;
    g_bonusMeteors[idx].y1_3c = g_bonusMeteors[idx].sy;
    g_bonusMeteors[idx].x2_40 = g_bonusMeteors[idx].x1_38 + g_bonusMeteors[idx].w;
    g_bonusMeteors[idx].y2_44 = g_bonusMeteors[idx].y1_3c + g_bonusMeteors[idx].h;
    g_bonusMeteors[idx].x = RandFloat(-30.0f, g_screenW - 100.0);
    g_bonusMeteors[idx].y = RandFloat(-700.0f, -200.0f);
    g_bonusMeteors[idx].vx = RandFloat(-0.3f, 0.3f);
    g_bonusMeteors[idx].vy = RandFloat(1.0f, 4.0f);
}

// Sets up state for the meteor-storm (lock-on/race) bonus level: race distance and HUD meter, a 3-count
// countdown starting in 4 seconds, initial speed ramp/boost charge, and clears the meteor slots.
void InitMeteorStormLevel()
{
    int i;
    g_raceDistanceLeft = (float)g_save.players[g_curPlayer].raceDistance;
    g_meterValue = g_save.players[g_curPlayer].raceDistance;
    g_raceStartTime = g_time + 4000;
    g_raceCountdownStage = 3;
    g_lockOnLevelInitParam = 0x5460;
    g_save.players[g_curPlayer].savedRaceStarSpeed = g_save.players[g_curPlayer].starSpeed;
    g_baseSpeedRamp = 1.5f;
    g_boostCharge = 5.0f;
    for (i = 0; i < MAX_BONUS_METEORS; i++)
        g_bonusMeteors[i].active = 0;
    g_bonusItemCount = 0;
}

// Bonus-level-end callback for the meteor bonus level; no cleanup needed.
void OnMeteorStormLevelEnd()
{
}

// Awards score for the meteor-storm results and tracks it in the bonus-round tally too.
#define AWARD(val)                                                                            \
    g_save.players[g_curPlayer].bonusRoundScore += (val) * g_scoreMul[g_curPlayer]; \
    ADD_PLAYER_SCORE(g_save.players[g_curPlayer].score, g_curPlayer, val);

// Per-frame update for the race/bonus-meteor level type: spawns and moves falling bonus
// meteors, plays the 3-2-1 countdown voice cues, tracks the "speed %" boost/hurry meter,
// and, once the race distance runs out, tallies the end-of-run score/money bonus (which
// tier depends on drunk-mode and how close to 100% speed the player finished) and
// transitions to the results state.
void MeteorStormUpdate()
{
    int i;
    int j;
    unsigned int dt;
    int bonus;

    if (g_gameMode == MODE_DUAL)
        g_curPlayer = g_vsTurnPlayer;
    dt = g_raceStartTime - g_time;
    if (g_time > g_raceStartTime)
        g_bonusSpawnTarget += g_bonusSpawnRampRate * g_frameDt * (1.0 + g_boostCharge / 30.0 * g_frameDt);
    if (g_raceCountdownStage == 3 && dt < 2000) {
        SoundQueueAdd(g_sfxVoiceThree, 0, 1);
        g_raceCountdownStage = 2;
    }
    if (g_raceCountdownStage == 2 && dt < 1000) {
        SoundQueueAdd(g_sfxVoiceTwo, 0, 2);
        g_raceCountdownStage = 1;
    }
    if (g_raceCountdownStage == 1 && dt < 10) {
        SoundQueueAdd(g_sfxVoiceOne, 0, 3);
        g_raceCountdownStage = 0;
    }

    // Spawn a replacement bonus item if the field is below target.
    if (g_bonusItemCount < (int)g_bonusSpawnTarget && g_raceDistanceLeft > 0.0) {
        for (i = 0; i < MAX_BONUS_METEORS; i++) {
            if (g_bonusMeteors[i].active == 0) {
                SpawnMeteor(i);
                g_bonusMeteors[i].active = 1;
                g_bonusItemCount++;
                break;
            }
        }
    }

    // Move each active bonus meteor and respawn ones that fell off-screen.
    for (i = 0; i < MAX_BONUS_METEORS; i++) {
        if (g_time > g_raceStartTime && g_bonusMeteors[i].active != 0) {
            if (g_bonusMeteors[i].type == METEOR_HAZARD) {
                g_bonusMeteors[i].x += g_bonusMeteors[i].vx * g_frameDt;
                if (g_bonusMeteors[i].y <= -40.0 &&
                    g_bonusMeteors[i].y + (g_bonusMeteors[i].vy + g_baseSpeedRamp + g_boostCharge) >= -40.0)
                    SoundPlay(g_sfxMeteorPass, 15000, g_bonusMeteors[i].vol,
                                      g_panTable[ClampX((int)g_bonusMeteors[i].x)], 127, g_sndFlags);
                g_bonusMeteors[i].y += (g_bonusMeteors[i].vy + g_baseSpeedRamp + g_boostCharge) * g_frameDt;

                if (g_bonusMeteors[i].y > g_screenH) {
                    g_bonusMeteors[i].active = 0;
                    g_bonusItemCount--;
                    if (g_bonusItemCount < (int)g_bonusSpawnTarget) {
                        for (j = 0; j < MAX_BONUS_METEORS; j++) {
                            if (g_bonusMeteors[j].active == 0) {
                                SpawnMeteor(j);
                                g_bonusMeteors[j].active = 1;
                                if (g_bonusMeteors[j].type == 0)
                                    g_bonusItemCount++;
                                break;
                            }
                        }
                    }
                }
            } else {
                g_bonusMeteors[i].x += g_bonusMeteors[i].vx * g_frameDt;
                g_bonusMeteors[i].y += (g_bonusMeteors[i].vy + g_baseSpeedRamp + g_boostCharge) * g_frameDt;
                if (g_bonusMeteors[i].y > g_screenH)
                    g_bonusMeteors[i].active = 0;
                g_bonusMeteors[i].animTimer -= 1.0f * g_frameDt;

                if (g_bonusMeteors[i].animTimer < 0.0) {
                    g_bonusMeteors[i].animTimer = 5.0f;
                    if (g_bonusMeteors[i].type == METEOR_MONEY) {
                        g_bonusMeteors[i].sy += 37;
                        if (g_bonusMeteors[i].sy == 370)
                            g_bonusMeteors[i].sy = 0;
                    } else {
                        g_bonusMeteors[i].sy += 51;
                        if (g_bonusMeteors[i].sy == 561)
                            g_bonusMeteors[i].sy = 0;
                    }
                }
            }
        }
    }

    if (g_time < g_raceStartTime) {
        g_slowFrameCount = 0;
        g_fastFrameCount = 0;
        g_speedPct = 0.0f;
        g_boostReleased = 0;
        g_raceHoldTime = g_time;
    }

    // Ramp base speed and award time-based score while the race is running.
    if (g_time > g_raceStartTime) {
        g_baseSpeedRamp += g_frameDt * (double)0.0012f;
        if (g_chargeMax / 7.0 * 2 > g_boostCharge)
            g_slowFrameCount++;
        if (g_chargeMax / 7.0 * 2 <= g_boostCharge)
            g_fastFrameCount++;
        if (g_fastFrameCount > g_slowFrameCount)
            g_speedPct = (1.0 - (float)g_slowFrameCount / g_fastFrameCount) * 100.0;
        else
            g_speedPct = 0.0f;
        if (g_scoreMul[g_curPlayer] < 1)
            g_scoreMul[g_curPlayer] = 1;

        bonus = (int)(g_boostCharge / g_chargeMax * 50.0) + 1;
        ADD_PLAYER_SCORE(g_save.players[g_curPlayer].score, g_curPlayer, bonus * 10);
        g_save.players[g_curPlayer].bonusRoundScore += (bonus * 10) * g_scoreMul[g_curPlayer];
    }

    if (g_time > g_raceStartTime) {
        g_raceDistanceLeft -= (g_boostCharge / 2.0 + 3.0) * g_frameDt;
        g_meterValue = (int)g_raceDistanceLeft;
    }

    // Race distance has run out: finish the level and score the result.
    if (g_raceDistanceLeft < 0.0) {
        g_drunk = 0;
        if (g_save.players[g_curPlayer].drunkModeTimer > 0)
            g_drunk = 1;
        if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) &&
            g_playerUpdateFn != StateDemo) {
            TimerStop2();
            if (g_gameMode == MODE_SINGLE)
                UpdateFastestMeteorStorm(g_profileIndex, g_timerMin2);
        }
        g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
        g_transitionLock = 1;
        g_resultsScreenEndTime = g_time + 3000;
        g_raceDistanceLeft = 0.0f;
        g_save.players[g_curPlayer].raceDistance += 134;
        g_save.players[g_curPlayer].starSpeed = g_save.players[g_curPlayer].savedRaceStarSpeed;
        if (g_save.players[g_curPlayer].done == 0) {
            g_save.players[g_curPlayer].done = 1;
            if (g_save.players[g_curPlayer].doneTime == 0)
                g_save.players[g_curPlayer].doneTime = g_time + DONE_DELAY_MS;
        }

        g_save.players[g_curPlayer].killed = g_save.players[g_curPlayer].totalEnemies;
        g_save.players[g_curPlayer].escaped = 0;
        ResetObjectsKeep();
        g_save.players[0].energy = 0;
        g_save.players[1].energy = 0;
        g_save.players[2].energy = 0;
        g_save.players[3].energy = 0;

        // Award medals for a full-drunk-mode / high-speed finish.
        if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo &&
            g_drunk && g_speedPct >= 99.0) {
            if ((GetMedals(g_profileIndex) & MEDAL_DRUNK_FINISH) == 0)
                AwardMedal(g_profileIndex, MEDAL_DRUNK_FINISH);
        }
        if (g_speedPct >= 99.0) {
            g_save.players[g_curPlayer].drunkStreak++;
            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo &&
                g_save.players[g_curPlayer].drunkStreak == 5) {
                if ((GetMedals(g_profileIndex) & MEDAL_SPEED_STREAK) == 0)
                    AwardMedal(g_profileIndex, MEDAL_SPEED_STREAK);
            }
        } else {
            g_save.players[g_curPlayer].drunkStreak = 0;
        }

        if (g_drunk && g_speedPct > 90.0 && g_profileIndex != -1 && g_gameMode == MODE_SINGLE &&
            g_playerUpdateFn != StateDemo) {
            g_save.players[g_curPlayer].secretFound29 = 1;
            MarkSecretFound(g_profileIndex, 29);
        }

        // Award end-of-level score/money based on drunk mode and final speed %.
        if (g_drunk) {
            if ((int)g_speedPct == 100 && !g_boostReleased) {
                AWARD(DRUNK_MEGA_SCORE)
                g_save.players[g_curPlayer].money += DRUNK_MEGA_MONEY;
                g_resultsScreenEndTime = g_time + 10000;
            } else if (g_speedPct >= 99.0) {
                AWARD(DRUNK_EXTREME_SCORE)
                g_save.players[g_curPlayer].money += DRUNK_EXTREME_MONEY;
                g_resultsScreenEndTime = g_time + 8000;
            } else if (g_speedPct >= 90.0) {
                AWARD(DRUNK_SUPER_SCORE)
                g_save.players[g_curPlayer].money += DRUNK_SUPER_MONEY;
                g_resultsScreenEndTime = g_time + 8000;
            } else if (g_speedPct < 90.0) {
                AWARD(DRUNK_EXTRA_SCORE)
            }
        } else {

            if ((int)g_speedPct == 100 && !g_boostReleased) {
                AWARD(MEGA_SCORE)
                g_save.players[g_curPlayer].money += MEGA_MONEY;
                g_resultsScreenEndTime = g_time + 10000;
            } else if (g_speedPct >= 99.0) {
                AWARD(EXTREME_SCORE)
                g_save.players[g_curPlayer].money += EXTREME_MONEY;
                g_resultsScreenEndTime = g_time + 8000;
            } else if (g_speedPct >= 90.0) {
                AWARD(SUPER_SCORE)
                g_save.players[g_curPlayer].money += SUPER_MONEY;
            } else {
                AWARD(EXTRA_SCORE)
            }
        }

        RETURN_TO_HUD_VIEW()
        if (g_scoreMul[g_curPlayer] > 1 && g_profileIndex != -1 && g_gameMode == MODE_SINGLE &&
            g_playerUpdateFn != StateDemo) {
            g_save.players[g_curPlayer].secretFound05 = 1;
            MarkSecretFound(g_profileIndex, 5);
        }
        if (g_save.players[g_curPlayer].money > g_save.players[g_curPlayer].moneyMax)
            g_save.players[g_curPlayer].money = g_save.players[g_curPlayer].moneyMax;
        if (g_save.players[g_curPlayer].money > g_moneyMax)
            g_moneyMax = g_save.players[g_curPlayer].money;
        // NOTE: these four assignments repeat the block above verbatim; kept as-is for
        // the byte match.
        RETURN_TO_HUD_VIEW()
        g_state = STATE_METEOR_STORM; // meteor storm results
    }
    g_save.players[g_curPlayer].starSpeed =
        g_save.players[g_curPlayer].savedRaceStarSpeed + (g_baseSpeedRamp + g_boostCharge) * 2;
}

#undef AWARD

// Draws the boost-charge meter (a stack of tick blits) and the "speed %" text next to
// the current player's ship during a race level.
void DrawSpeedMeter()
{
    int p;
    int x;
    int y;
    float max;
    float step;
    float len;
    float i;

    p = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        p = g_vsTurnPlayer;
    x = (int)(g_save.players[p].x + 40.0);
    y = (int)(g_save.players[p].y + 25.0);
    max = 50.0f;
    step = max / 7.0;
    Blit(x, y + 2, 0, g_gfxLogos, 0x30, 0xb0, 0x10, 6);
    if (g_chargeMax == 0.0)
        g_chargeMax = 15.0f;
    len = g_boostCharge / g_chargeMax * max;
    i = 0.0f;

    if (len > 0.0) {
        do {
            if (step == 0.0)
                step = 1.0f;
            Blit(x, y - (int)i, 0, g_gfxLogos, 0x30, 0xae - (int)(i / step) * 2, 0x10, 1);
            i = i + 2.0;
        } while (i < len);
    }

    if ((int)g_speedPct != g_speedPctCache) {
        g_speedPctCache = (int)g_speedPct;
        sprintf(g_speedPctText, "%3d%%", (int)g_speedPct);
    }
    DrawMixedCaseText(g_speedPctText, x - 2, y + 10, 1);
}

// Draws the 30 falling bonus meteors, clipped to the screen rect.
void DrawMeteors()
{
    Rect16 src;
    int i;
    int offY;
    int offX;

    for (i = 0; i < MAX_BONUS_METEORS; i++) {
        if (g_bonusMeteors[i].active != 0) {
            while (1) {
                offY = 0;
                offX = 0;
                if (g_bonusMeteors[i].y > g_clipTop)
                    src.y1 = g_bonusMeteors[i].sy;
                else if (g_clipTop - (int)g_bonusMeteors[i].y >= g_bonusMeteors[i].h)
                    break;
                else {
                    offY = g_clipTop - (int)g_bonusMeteors[i].y;
                    src.y1 = g_bonusMeteors[i].sy + offY;
                }

                if (g_bonusMeteors[i].x > g_clipLeft)
                    src.x1 = g_bonusMeteors[i].sx;
                else if (g_clipLeft - (int)g_bonusMeteors[i].x >= g_bonusMeteors[i].w)
                    break;
                else {
                    offX = g_clipLeft - (int)g_bonusMeteors[i].x;
                    src.x1 = g_bonusMeteors[i].sx + offX;
                }
                src.x2 = g_bonusMeteors[i].w - offX + src.x1;
                if ((int)g_bonusMeteors[i].x + g_bonusMeteors[i].w - offX > g_clipRight) {
                    if (g_bonusMeteors[i].x > g_clipRight)
                        break;
                    else
                        src.x2 = src.x2 -
                                 ((int)g_bonusMeteors[i].x + g_bonusMeteors[i].w - offX - g_clipRight);
                }

                src.y2 = g_bonusMeteors[i].h - offY + src.y1;
                if ((int)g_bonusMeteors[i].y + (g_bonusMeteors[i].h - offY) > g_clipBottom) {
                    if (g_bonusMeteors[i].y > g_clipBottom)
                        break;
                    else
                        src.y2 = src.y2 -
                                 ((int)g_bonusMeteors[i].y + (g_bonusMeteors[i].h - offY) - g_clipBottom);
                }
                QueueBlit(g_bonusMeteors[i].x + offX, g_bonusMeteors[i].y + offY,
                          (Image *)g_bonusMeteors[i].graphic, &src);
                break;
            }
        }
    }
}

#define P g_save.players[g_curPlayer]

#define SHIP g_shipDefs[P.ship]

// Adds score for a meteor pickup and tracks it in the bonus-round tally too.
#define ADD_SCORE(v)                                                            \
    ADD_PLAYER_SCORE(P.score, g_curPlayer, v);           \
    P.bonusRoundScore = P.bonusRoundScore + (v) * g_scoreMul[g_curPlayer];

#define GEM_LOOP(gem, j)                                                                    \
    for (j = 0; j < 5; j++) {                                                               \
        P.gems = P.gems + SHIP->gemStep;                                           \
        if ((P.gems - SHIP->gemBase) / SHIP->gemStep % 100 == 0)                   \
            gem = true;                                                                     \
    }

#define GEM_DROP_HEAD                                                                       \
    SoundQueueAdd(g_sfxGemDrop, 50, 0);                                                  \
    sprintf(g_alertMsg, "G E M   D R O P");                                               \
    if ((P.gems - SHIP->gemBase) / SHIP->gemStep >= 1000) {                        \
        g_superGemDrop = 1;                                                                       \
        P.gems = P.gems - SHIP->gemStep * 1000;                                    \
        sprintf(g_alertMsg, "S U P E R   G E M   D R O P");                               \
        if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) { \
            P.secretFound30 = 1;                                                                    \
            MarkSecretFound(g_profileIndex, 30);                                                  \
        }                                                                                   \
    }                                                                                       \
    if (P.alienLock == 0) {                                                                     \
        if (P.shieldL != 0) {                                                                 \
            DropAlienGfxAge(g_enemies[g_curPlayer][P.shieldLIdx].gfxA);             \
            P.shieldL = 0;                                                                    \
            g_enemies[savedPlayer][P.shieldLIdx].active = 0;                               \
            g_enemies[savedPlayer][P.shieldLIdx].settled = 0;                               \
        }                                                                                   \
        if (P.shieldR != 0) {                                                                 \
            DropAlienGfxAge(g_enemies[g_curPlayer][P.shieldRIdx].gfxA);             \
            P.shieldR = 0;                                                                    \
            g_enemies[savedPlayer][P.shieldRIdx].active = 0;                               \
            g_enemies[savedPlayer][P.shieldRIdx].settled = 0;                               \
        }                                                                                   \
    }                                                                                       \
    if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) { \
        P.secretFound12 = 1;                                                                        \
        MarkSecretFound(g_profileIndex, 12);                                                      \
    }                                                                                       \
    g_msgColor = 3;                                                                           \
    g_flashOverlayActive = 1;                                                                           \
    g_fadeStep = 0;                                                                           \
    g_fadeColorSet = 1;                                                                           \
    g_maxFallingGems = 1.0f;                                                                        \
    InitGemDropLevel();                                                                            \
    PlayGemDropMusic();                                                                            \
    g_gemDropIntroTimer = g_time + 4000;                                                        \
    g_state = STATE_GEM_DROP;

#define GEM_DROP_TAIL                                                                       \
    BankBonusScore();                                                                            \
    g_buttonsOn = 0;                                                                           \
    HidePointer();

// Checks the current player against all active bonus meteors for a pixel-mask collision
// and resolves it by type: type 0 is a hazard that resets the race meter and stuns the
// run (unless autoplay rolls to ignore it); type 1 awards money/score by colour tier
// (its `sx` sprite-sheet column selects the tier); anything else awards score, a floating
// popup, and rolls a chance to drop a gem (and possibly a "super" gem) via GEM_LOOP.
void MeteorStormCollide()
{
    int px;
    int py;
    int px2;
    int py2;
    int bx;
    int by;
    int bx2;
    int by2;
    int i;
    int savedPlayer;

    savedPlayer = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        savedPlayer = 0;
    if (g_gameMode == MODE_DUAL)
        g_curPlayer = g_vsTurnPlayer;
    px = (int)P.x;
    py = (int)P.y;
    px2 = px + 40;
    py2 = py + 27;

    for (i = 0; i < MAX_BONUS_METEORS; i++) {
        if (g_bonusMeteors[i].active != 0) {
            bx = (int)g_bonusMeteors[i].x;
            bx2 = bx + g_bonusMeteors[i].w;
            by = (int)g_bonusMeteors[i].y;
            by2 = by + g_bonusMeteors[i].h;

            if (bx < px2 && bx2 > px && by < py2 && by2 > py) {
                if (MaskCollide(bx, by, bx2, by2, px, py, px2, py2,
                                   (unsigned char *)g_bonusMeteors[i].hma, (unsigned char *)P.hitMask,
                                   g_bonusMeteors[i].hitBox, P.box,
                                   g_bonusMeteors[i].hmaW, P.hitMaskParamA,
                                   g_bonusMeteors[i].hmaH, P.hitMaskParamB, g_cfg.collisionDetail)) {
                    if (g_bonusMeteors[i].type == METEOR_HAZARD) {
                        if (g_autoplay && RandRange(0, 1000) < 992)
                            break;

                        P.drunkStreak = 0;
                        g_flashOverlayActive = 1;
                        g_fadeStep = 0;
                        g_fadeColorSet = 0;
                        g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
                        g_transitionLock = 1;
                        g_raceDistanceLeft = 1.0f;
                        P.starSpeed = P.savedRaceStarSpeed;
                        ResetObjectsKeep();
                        g_save.players[0].energy = 0;
                        g_save.players[1].energy = 0;
                        g_save.players[2].energy = 0;
                        g_save.players[3].energy = 0;

                        SetHurryUpTimer();
                        g_resultsScreenEndTime = g_time + 3000;
                        g_state = STATE_METEOR_STORM;
                        g_meterV = 0.02f;
                        g_meterDirUp = 0;

                        SpawnSlots(px + 16.0, py + 16.0, 400, 255, 0, 0);
                        if (g_cfg.particlesOn != 0)
                            AddParticle(g_gfxFlare3, px + 16, py + 16,
                                               RandFloat(5.0f, 10.0f),
                                               RandFloat(50.8f, 80.6f),
                                               RandFloat(0.0f, 359.0f), 0.0f, 0,
                                               RandRange(150, 255),
                                               RandRange(0, 150),
                                               RandRange(150, 255), 600,
                                               RandFloat(10.0f, 20.0f), 0.0f, -1, 0.0f,
                                               0, 0, 0, 0);
                        SoundPlay(g_sfxThumpBig, 30000, 255, 0.0f, 0x7f, g_sndFlags);
                    } else if (g_bonusMeteors[i].type == METEOR_MONEY) {
                        switch (g_bonusMeteors[i].sx) {
                        case 0:
                            P.money = P.money + METEOR_MONEY_50;
                            break;
                        case 0x40:
                            P.money = P.money + METEOR_MONEY_100;
                            break;
                        case 0x80:
                            P.money = P.money + METEOR_MONEY_250;
                            break;

                        case 0xc0:
                            ADD_SCORE(METEOR_SCORE_1000)
                            break;
                        case 0x100:
                            ADD_SCORE(METEOR_SCORE_5000)
                            break;
                        case 0x140:
                            ADD_SCORE(METEOR_SCORE_10000)
                            break;
                        }

                        RETURN_TO_HUD_VIEW()
                        if (P.money > P.moneyMax)
                            P.money = P.moneyMax;
                        if (P.money > g_moneyMax)
                            g_moneyMax = P.money;
                        g_bonusMeteors[i].active = 0;
                        SoundPlay(g_sfxBing, -1, 255, 0.0f, 0x7f, g_sndFlags);
                    } else {

                        if (g_bonusMeteors[i].sx == 0) {
                            ADD_SCORE(GEM_SCORE_2500)
                            AddScorePopup(bx + 20, by + 20, GEM_SCORE_2500, 0);
                            bool gem1 = false;
                            int j1;
                            GEM_LOOP(gem1, j1)
                            if (gem1) {
                                GEM_DROP_HEAD
                                g_meterY = 550.0f;
                                g_meterV = 35.0f;
                                g_meterDirUp = 1;
                                g_meterShowUntil = g_time - 100;
                                g_bonusMeterStarted = 0;
                                GEM_DROP_TAIL
                            }
                        }

                        if (g_bonusMeteors[i].sx == 0x50) {
                            ADD_SCORE(GEM_SCORE_5000)
                            AddScorePopup(bx + 20, by + 20, GEM_SCORE_5000, 0);
                            bool gem2 = false;
                            int j2;
                            GEM_LOOP(gem2, j2)
                            if (gem2) {
                                GEM_DROP_HEAD
                                GEM_DROP_TAIL
                            }
                        }

                        if (g_bonusMeteors[i].sx == 0xa0) {
                            ADD_SCORE(GEM_SCORE_10000)
                            AddScorePopup(bx + 20, by + 20, GEM_SCORE_10000, 0);
                            bool gem3 = false;
                            int j3;
                            GEM_LOOP(gem3, j3)
                            if (gem3) {
                                GEM_DROP_HEAD
                                GEM_DROP_TAIL
                            }
                        }

                        g_bonusMeteors[i].active = 0;
                        SoundQueueAdd(g_sfxBonus, 50, 0);
                        SoundPlay(g_sfxBell1, RandRange(22000, 32000), 255, g_pan, 0x7f, g_sndFlags);
                    }
                }
            }
        }
    }
}

#undef P
#undef SHIP
#undef ADD_SCORE
#undef GEM_LOOP
#undef GEM_DROP_HEAD
#undef GEM_DROP_TAIL
