// levelstart.c: Starting levels: StartLevel, the next level, get-ready intros, background tint,
// hot-seat switch, object resets.
#include <stdio.h>
#include "globals.h"
#include "game.h"

// Local to this file: enemy score caps applied when spawning (regular group / header / boss).
enum {
    ENEMY_SCORE_CAP_GROUP  = 50000,
    ENEMY_SCORE_CAP_HEADER = 200000,
    ENEMY_SCORE_CAP_BOSS   = 20000000,
};
// Local to this file: level thresholds above which wave enemies aim at the player / fire faster.
enum {
    WAVE_AIM_LEVEL         = 500,
    WAVE_FAST_BULLET_LEVEL = 750,
};
// Local to this file: a sub-weapon fireDelay of exactly this value is x10'd instead of clamped.
enum { FIXED_FIRE_DELAY = 3500 };


// NOTE: dead code — the early `return` makes the zero-fill below unreachable. Kept as-is for the byte
// match; players are actually reset field-by-field via InitPlayer instead.
void ClearPlayers()
{
    return;
    char *p = (char *)g_save.players;
    int n = sizeof(g_save.players);
    for (int i = 0; i < n; i++)
        p[i] = 0;
}

// Advances to the next level for the current player: resets per-level flags, nudges the alien-malfunction
// timer, bumps the level counter, and schedules the "get ready" intro state to start shortly.
void StartNextLevel()
{
    if (g_gameMode != MODE_TIME_TRIAL && g_levelStartLatch)
        return;
    int p = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        p = 0;
    g_getReadyFlagA = 0;
    g_introDone = 0;
    g_save.players[p].started = 0;
    g_save.players[p].done = 0;
    g_save.players[p].doneTime = 0;
    g_malfunctionTimer = g_malfunctionTimer - RandRange(100, 1500);
    if (g_malfunctionTimer < 2000)
        g_malfunctionTimer = RandRange(2300, 4000);
    g_save.players[p].level++;
    g_pendingLevelsPlayed++;
    g_save.players[p].levelFinished = 0;
    if (g_gameMode == MODE_TIME_TRIAL)
        g_timerA = g_time + 1000;
    else
        g_timerA = g_time + 2000;
    g_state = STATE_RESPAWN;
    g_levelStartLatch = 1;
}

// Hot-seat player swap: clears the outgoing player's enemies/level objects/items, flips
// g_curPlayer, then either starts that player's hyperspace-out sequence (if they already
// finished their level) or loads their in-progress level directly. Called after a player
// dies or completes a level in split-turn modes. No-op while state STATE_HISCORE_TABLE is active.
void SwitchPlayer()
{
    if (g_state != STATE_HISCORE_TABLE) {
        SoundStopAll();
        if (g_state == STATE_MALFUNCTION) {  // snapshot enemies before the swap
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (g_enemies[g_curPlayer][i].type != ENEMY_CAPTURED) {
                    g_enemies[g_curPlayer][i].forcedDir = -1;
                    g_enemies[g_curPlayer][i].active = 0;
                    g_enemies[g_curPlayer][i].altFireActive = 0;
                    g_enemies[g_curPlayer][i].pairedEnemyIdx = -1;
                    g_enemies[g_curPlayer][i].hitFlashTimer = 0;
                }
            }
            g_save.players[g_curPlayer].levelFinished = 1;
        }

        if (g_curPlayer == 0)
            g_curPlayer = 1;
        else
            g_curPlayer = 0;
        for (int j = 0; j < MAX_LEVEL_OBJS; j++)
            g_levelObj[j].active = 0;
        for (int k = 0; k < MAX_ITEMS; k++) {
            if (g_items[k].alive != 0 && g_items[k].active != 0)
                g_items[k].alive = 0;
        }

        if (g_save.players[g_curPlayer].levelFinished != 0) {
            // Player already finished their level: warp out and hand off to the death/
            // transition sequence (STATE_MALFUNCTION_DEATH) instead of loading a new level here.
            SoundPlay(g_sfxWarp, -1, 0xff, 0.0f, 0xbf, g_sndFlags);
            g_save.players[g_curPlayer].hyperspaceOutTimer = g_save.players[g_curPlayer].hyperspaceInDuration;
            if (g_gameMode == MODE_DUAL) {  // drop both players' shields together
                if (!g_save.players[0].alienLock) {
                    g_save.players[0].shieldL = 0;
                    g_save.players[0].shieldR = 0;
                }
                g_save.players[0].freezeTimer = 0;

                if (!g_save.players[1].alienLock) {
                    g_save.players[1].shieldL = 0;
                    g_save.players[1].shieldR = 0;
                }
                g_save.players[1].freezeTimer = 0;
            } else {
                if (!g_save.players[g_curPlayer].alienLock) {
                    g_save.players[g_curPlayer].shieldL = 0;
                    g_save.players[g_curPlayer].shieldR = 0;
                }
                g_save.players[g_curPlayer].freezeTimer = 0;
            }
            g_deathSeqActive = 1;
            g_save.players[g_curPlayer].levelFinished = 0;
            g_state = STATE_MALFUNCTION_DEATH;

        } else {
            // Player still mid-level: resume it directly at the "get ready" state.
            g_curLevelNum = g_save.players[g_curPlayer].level;
            LoadLevelData();
            SetHurryUpTimer();
            g_getReadyFlagA = 0;
            g_introDone = 0;
            g_save.players[g_curPlayer].done = 0;
            g_save.players[g_curPlayer].doneTime = 0;
            g_timerA = g_time + 1500;  // ms
            g_timerB = g_time + 5000;  // ms
            g_state = STATE_GET_READY;
        }
    }
}

// Clamps g_bgTint to [0,2] and applies the matching pale color tint (red/green/blue) to the
// background graphic.
int ApplyBgTint()
{
    if (g_bgTint < 0)
        g_bgTint = 0;
    if (g_bgTint > 2)
        g_bgTint = 2;
    if (g_bgTint == 0)
        ImgSetBlitColor(g_bgGraphic, 1.0f, 0.8f, 0.8f, 1.0f);
    if (g_bgTint == 1)
        ImgSetBlitColor(g_bgGraphic, 0.8f, 1.0f, 0.8f, 1.0f);
    if (g_bgTint == 2)
        ImgSetBlitColor(g_bgGraphic, 0.8f, 0.8f, 1.0f, 1.0f);
    return 1;
}

// Picks a random background tint, biased toward tints matching the current player's active
// secret bonuses (blue money, gem counter, money-sucker multiplier), and marks secret 11
// found the first time one of those bonuses is seen (single-player, non-demo).
void PickBgTint()
{
    if (g_save.players[g_curPlayer].blueMoneyActive) {
        if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
            g_save.players[g_curPlayer].secretFound11 = 1;
            MarkSecretFound(g_profileIndex, 11);
        }

        g_bgTint = RandRange(1, 2);
        ApplyBgTint();
    } else if (g_save.players[g_curPlayer].gemCounterCollected) {
        if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
            g_save.players[g_curPlayer].secretFound11 = 1;
            MarkSecretFound(g_profileIndex, 11);
        }

        g_bgTint = RandRange(0, 2);
        if (g_bgTint == 1)
            g_bgTint = 2;
        ApplyBgTint();
    } else if (g_save.players[g_curPlayer].msMultiplierActive) {
        if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
            g_save.players[g_curPlayer].secretFound11 = 1;
            MarkSecretFound(g_profileIndex, 11);
        }

        g_bgTint = RandRange(0, 2);
        ApplyBgTint();
    } else {
        g_bgTint = RandRange(0, 3);
        ApplyBgTint();
    }
}

// Sets this level's death-explosion graphic/lifetime and enemy death-flash color (repeated
// once per 4-level band of the level-theme table below).
#define SET_DEATH_FX(gfx, life, r, g, b) \
    deathExplosionGfx = (gfx);           \
    deathExplosionLife = (life);         \
    g_levelEnemyColorR = (r);            \
    g_levelEnemyColorG = (g);            \
    g_levelEnemyColorB = (b);

// Points a freshly-spawned enemy at hazard-graphics channel `n`'s gfx/hma/frame-set, and
// clamps its score (from `scoreSrc`) to `cap`. Used once per hazard type (1-6) in both the
// per-group spawn loop (scoreSrc = g_curLevelData.w[n], cap = ENEMY_SCORE_CAP_GROUP, hitRectArr
// = g_rectsA) and the header spawn loop (h[n], ENEMY_SCORE_CAP_HEADER, g_rectsB).
#define INIT_HAZARD_ENEMY(n, scoreSrc, cap, hitRectArr)        \
    g_enemies[g_curPlayer][k].gfxA = g_alienGfxCache[n].gfx1;  \
    g_enemies[g_curPlayer][k].gfxB = g_alienGfxCache[n].gfx2;  \
    g_enemies[g_curPlayer][k].shotFrame = g_alienGfxMem[n];    \
    g_enemies[g_curPlayer][k].shotGfxW = g_hazard##n##GfxW;    \
    g_enemies[g_curPlayer][k].shotGfxH = g_hazard##n##GfxH;    \
    g_enemies[g_curPlayer][k].score = (scoreSrc);              \
    if (g_enemies[g_curPlayer][k].score < 0)                   \
        g_enemies[g_curPlayer][k].score = 0;                   \
    if (g_enemies[g_curPlayer][k].score > (cap))                \
        g_enemies[g_curPlayer][k].score = (cap);                \
    g_enemies[g_curPlayer][k].frameSet = g_frames[n];          \
    g_enemies[g_curPlayer][k].hitRect = hitRectArr[n];

// Called once per level to begin it: records the previous level's stats, resets per-level
// state (HUD, timers, boss guns, difficulty ramp on marathon milestones), loads the level
// data file, then spawns the level's enemies (group entries, plus boss-gun/header entries
// for boss levels) into `g_enemies[g_curPlayer]`.
void StartLevel()
{
    int i;
    int j;
    int k;
    int spawnStagger;
    int spawnedCount;
    int noFreeSlot;
    int slotSearchDone;
    float maxEnemyHp;
    int idx;
    float autofireFactorP0;
    float autofireFactorP1;
    float autofireFactorP2;
    float autofireFactorP3;
    int bgLevelMod;
    int deathExplosionGfx;
    int deathExplosionLife;

    int themeLevelMod;
    int themeLevelMod10;
    int bossGunIdx;

    // ---- record the previous level's stats ----
    idx = g_save.players[g_curPlayer].level > MAX_LEVEL_RECS - 1 ? MAX_LEVEL_RECS - 1 : g_save.players[g_curPlayer].level;
    g_levelRecs[idx].score = g_save.players[g_curPlayer].score;
    g_levelRecs[idx].livesGainedByte = (g_livesGainedCount - 20) / 10;
    g_levelRecs[idx].shots = g_save.players[g_curPlayer].shots;
    g_levelRecs[idx].deathsByte = (g_deathsCount - 50) / 2;
    g_levelRecs[idx].armourAddedByte = (g_armourAddedCount - 10) / 5;
    g_levelRecs[idx].money = g_save.players[g_curPlayer].money;
    g_levelRecs[idx].verify0 = g_save.players[g_curPlayer].weaponAmmoPacked;
    g_levelRecs[idx].rank = g_save.players[g_curPlayer].rank;
    g_levelRecs[idx].verify4 = g_save.players[g_curPlayer].gemPickups;
    g_levelRecs[idx].verify3 = g_save.players[g_curPlayer].collisionsTaken;
    g_levelRecs[idx].verify5 = g_save.players[g_curPlayer].bombPickups;
    g_levelRecs[idx].verify1 = g_save.players[g_curPlayer].pickupCount;
    g_levelRecs[idx].verify2 = g_save.players[g_curPlayer].shopVisits;
    g_levelRecs[idx].verify6 = g_save.players[g_curPlayer].rocketsFired;
    g_levelRecs[idx].frameBucket = g_enemyFrameCounter / 70;

    // ---- reset per-level flags ----
    g_armourAddedCount = 10;
    if (IsGameOver())
        ShowHiscoreTable();
    g_rankFanfarePlayed = 0;
    g_levelStartLatch = 0;
    g_isBossLevel = 0;
    g_isWaveLevel = 0;
    g_comboStep = 1;
    g_marksBonusGiven = 0;
    g_playerStallTime = g_time;
    g_livesGainedCount = 20;

    g_viewTransitionFlag = 2;
    g_stateFn = SetViewHud;
    EmptyViewChangeHook();
    EmptyPostTransitionHook();
    g_save.players[0].freezeTimer = 0;
    g_save.players[1].freezeTimer = 0;
    g_save.players[2].freezeTimer = 0;
    g_save.players[3].freezeTimer = 0;
    g_bossGunActiveA = 0;
    g_bossGunActiveB = 0;
    g_bossGunActiveC = 0;
    g_deathsCount = 50;

    // recompute super-auto autofire interval from each player's game-speed setting
    if (g_save.players[0].superAuto) {
        autofireFactorP0 = 1.0 - (g_save.players[0].gameSpeedSetting - 50.0) / 400.0 * 1.75;
        g_save.players[0].autofireInterval =
            (int)(autofireFactorP0 * 30.0) < 5 ? 5 : (int)(autofireFactorP0 * 30.0);
    }
    if (g_save.players[1].superAuto) {
        autofireFactorP1 = 1.0 - (g_save.players[1].gameSpeedSetting - 50.0) / 400.0 * 1.75;
        g_save.players[1].autofireInterval =
            (int)(autofireFactorP1 * 30.0) < 5 ? 5 : (int)(autofireFactorP1 * 30.0);
    }
    if (g_save.players[2].superAuto) {
        autofireFactorP2 = 1.0 - (g_save.players[2].gameSpeedSetting - 50.0) / 400.0 * 1.75;
        g_save.players[2].autofireInterval =
            (int)(autofireFactorP2 * 30.0) < 5 ? 5 : (int)(autofireFactorP2 * 30.0);
    }
    if (g_save.players[3].superAuto) {
        autofireFactorP3 = 1.0 - (g_save.players[3].gameSpeedSetting - 50.0) / 400.0 * 1.75;
        g_save.players[3].autofireInterval =
            (int)(autofireFactorP3 * 30.0) < 5 ? 5 : (int)(autofireFactorP3 * 30.0);
    }

    // ---- profile stat updates ----
    if (g_profileIndex != -1 && g_playerUpdateFn != StateDemo) {
        if (g_gameMode == MODE_DUAL) {
            if (g_save.players[0].level > 25)
                UpdateHitPctAbove25(g_profileIndex,
                                     (int)((double)g_save.players[0].hits / g_save.players[0].shots * 100.0));
            if (g_save.players[1].level > 25)
                UpdateHitPctAbove25(g_profileIndex,
                                     (int)((double)g_save.players[1].hits / g_save.players[1].shots * 100.0));
        } else if (g_save.players[g_curPlayer].level > 25) {
            UpdateHitPctAbove25(g_profileIndex,
                                 (int)((double)g_save.players[g_curPlayer].hits /
                                       g_save.players[g_curPlayer].shots * 100.0));
        }
    }
    if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
        ShowSecretsInOneGame(g_profileIndex);
        if (g_itemSteered) {
            g_save.players[g_curPlayer].secretFound04 = 1;
            MarkSecretFound(g_profileIndex, 4);
        }
        UpdateHighestLevel(g_profileIndex, g_save.players[g_curPlayer].level);
        AddLevelsPlayed(g_profileIndex, g_pendingLevelsPlayed);

        g_pendingLevelsPlayed = 0;
        UpdateHighestMoney(g_profileIndex, g_moneyMax);
    }

    // ---- bulk-level cooldown ----
    g_levelStarted = 1;
    if (g_save.players[0].bulkLevelsCooldown > 0)
        g_save.players[0].bulkLevelsCooldown--;
    if (g_save.players[1].bulkLevelsCooldown > 0)
        g_save.players[1].bulkLevelsCooldown--;
    if (g_save.players[2].bulkLevelsCooldown > 0)
        g_save.players[2].bulkLevelsCooldown--;
    if (g_save.players[3].bulkLevelsCooldown > 0)
        g_save.players[3].bulkLevelsCooldown--;

    // ---- current player / display level ----
    if (g_gameMode == MODE_DUAL) {
        g_curPlayer = 0;
        if (g_save.players[0].lives > g_shipDefs[g_save.players[0].ship]->minEnergy)
            g_save.players[0].displayLevel = g_save.players[g_curPlayer].level;
        if (g_save.players[1].lives > g_shipDefs[g_save.players[1].ship]->minEnergy)
            g_save.players[1].displayLevel = g_save.players[g_curPlayer].level;
    } else {
        if (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL)
            g_curPlayer = 0;
        g_save.players[g_curPlayer].displayLevel = g_save.players[g_curPlayer].level;
    }
    // ---- per-mode resets and marathon ramp ----
    if (g_gameMode == MODE_TIME_TRIAL) {
        idx = g_save.players[g_curPlayer].level > MAX_LEVEL_RECS - 1 ? MAX_LEVEL_RECS - 1 : g_save.players[g_curPlayer].level;
        g_save.players[g_curPlayer].deaths = 0;
        g_save.players[g_curPlayer].collisionsTaken = 0;
        g_save.players[g_curPlayer].bombPickups = 0;
        g_save.players[g_curPlayer].pickupCount = 0;
        g_save.players[g_curPlayer].shopVisits = 0;

        g_save.players[g_curPlayer].gemPickups = 0;
        g_save.players[g_curPlayer].rocketsFired = 0;
        g_enemyFrameCounter = 0;
    } else if ((g_save.players[g_curPlayer].level - 1) % LEVEL_THEME_CYCLE == 0 && g_save.players[g_curPlayer].level > MARATHON_START_LEVEL) {
        if (!g_save.players[g_curPlayer].levelMilestoneHandled) {
            g_bonusWeight[36] = 60;
            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                UpdateMarathonScore(g_profileIndex, g_save.players[g_curPlayer].score);
                if (g_save.players[g_curPlayer].score >= MARATHON_MILESTONE_SCORE) {
                    g_save.players[g_curPlayer].secretFound26 = 1;
                    MarkSecretFound(g_profileIndex, 26);
                }
            }
            g_save.players[g_curPlayer].levelMilestoneHandled = 1;
        }
        g_fireDelayBiasA -= 50;
        if (g_fireDelayBiasA < -500)
            g_fireDelayBiasA = -500;

        g_fireDelayBiasB -= 50;
        if (g_fireDelayBiasB < -500)
            g_fireDelayBiasB = -500;
        g_diffHpBonusA += 1.0;
        g_diffHpBonusB += 5.0;
        g_enemyBulletSpeed *= 1.025f;
        g_gameSpeedMul += 0.12f;
        g_save.players[g_curPlayer].gameSpeedSetting += g_diffScoreBonus;
        EmptyPostTransitionHook();
    }
    if (g_gameMode == MODE_SINGLE) {
        idx = g_save.players[g_curPlayer].level > MAX_LEVEL_RECS - 1 ? MAX_LEVEL_RECS - 1 : g_save.players[g_curPlayer].level;
        g_save.players[g_curPlayer].deaths = 0;
        g_save.players[g_curPlayer].collisionsTaken = 0;
        g_save.players[g_curPlayer].bombPickups = 0;
        g_save.players[g_curPlayer].pickupCount = 0;
        g_save.players[g_curPlayer].shopVisits = 0;
        g_save.players[g_curPlayer].gemPickups = 0;

        g_save.players[g_curPlayer].rocketsFired = 0;
        g_enemyFrameCounter = 0;
    }
    // ---- background index ----
    if (g_gameMode != MODE_TIME_TRIAL) {
        // background graphic rotates every 25 levels within each 100-level theme
        bgLevelMod = g_save.players[g_curPlayer].level % LEVEL_THEME_CYCLE;
        if (bgLevelMod < 26)
            g_bgIndex = 1;
        if (bgLevelMod > 25 && bgLevelMod < 51)
            g_bgIndex = 2;
        if (bgLevelMod > 50 && bgLevelMod < 76)
            g_bgIndex = 3;
        if (bgLevelMod > 75 && bgLevelMod < 100)
            g_bgIndex = 4;
    }
    deathExplosionGfx = 0;
    deathExplosionLife = 150;
    // Death-explosion graphic/lifetime/color set by which 4-level band of the 100-level
    // theme the level falls in (NOTE: 25 and 50 fall through both neighboring bands' `if`s
    // unchanged, since the ranges below skip them).
    if (g_gameMode != MODE_TIME_TRIAL) {
        themeLevelMod = g_save.players[g_curPlayer].level % LEVEL_THEME_CYCLE;

        if (themeLevelMod >= 1 && themeLevelMod <= 4) {
            SET_DEATH_FX(15, 100, 100, 255, 0)
        }
        if (themeLevelMod >= 5 && themeLevelMod <= 8) {
            SET_DEATH_FX(14, 100, 255, 180, 0)
        }
        if (themeLevelMod >= 9 && themeLevelMod <= 12) {
            SET_DEATH_FX(10, 150, 128, 200, 255)
        }
        if (themeLevelMod >= 13 && themeLevelMod <= 16) {
            SET_DEATH_FX(7, 150, 100, 255, 255)
        }
        if (themeLevelMod >= 17 && themeLevelMod <= 20) {
            SET_DEATH_FX(1, 150, 255, 128, 255)
        }
        if (themeLevelMod >= 21 && themeLevelMod <= 24) {
            SET_DEATH_FX(16, 100, 50, 128, 255)
        }
        if (themeLevelMod >= 26 && themeLevelMod <= 29) {
            SET_DEATH_FX(5, 150, 255, 225, 100)
        }
        if (themeLevelMod >= 30 && themeLevelMod <= 33) {
            SET_DEATH_FX(17, 150, 180, 150, 255)
        }

        if (themeLevelMod >= 34 && themeLevelMod <= 37) {
            SET_DEATH_FX(18, 130, 90, 255, 80)
        }
        if (themeLevelMod >= 38 && themeLevelMod <= 41) {
            SET_DEATH_FX(1, 150, 45, 255, 255)
        }
        if (themeLevelMod >= 42 && themeLevelMod <= 45) {
            SET_DEATH_FX(19, 150, 255, 55, 255)
        }
        if (themeLevelMod >= 46 && themeLevelMod <= 49) {
            SET_DEATH_FX(20, 100, 255, 255, 55)
        }
        if (themeLevelMod >= 51 && themeLevelMod <= 54) {
            SET_DEATH_FX(5, 100, 155, 255, 255)
        }
        if (themeLevelMod >= 55 && themeLevelMod <= 58) {
            SET_DEATH_FX(23, 100, 255, 100, 255)
        }
        if (themeLevelMod >= 59 && themeLevelMod <= 62) {
            SET_DEATH_FX(25, 90, 100, 255, 100)
        }
        if (themeLevelMod >= 63 && themeLevelMod <= 66) {
            SET_DEATH_FX(26, 140, 255, 200, 200)
        }

        if (themeLevelMod >= 67 && themeLevelMod <= 70) {
            SET_DEATH_FX(27, 100, 255, 200, 200)
        }
        if (themeLevelMod >= 71 && themeLevelMod <= 74) {
            SET_DEATH_FX(28, 80, 255, 255, 0)
        }
        if (themeLevelMod >= 76 && themeLevelMod <= 79) {
            SET_DEATH_FX(29, 80, 100, 255, 100)
        }
        if (themeLevelMod >= 80 && themeLevelMod <= 83) {
            SET_DEATH_FX(30, 80, 255, 100, 0)
        }
        if (themeLevelMod >= 84 && themeLevelMod <= 87) {
            SET_DEATH_FX(31, 100, 255, 100, 0)
        }
        if (themeLevelMod >= 88 && themeLevelMod <= 91) {
            SET_DEATH_FX(32, 100, 255, 128, 0)
        }
        if (themeLevelMod >= 92 && themeLevelMod <= 95) {
            SET_DEATH_FX(33, 100, 255, 200, 0)
        }
        if (themeLevelMod >= 96 && themeLevelMod <= 99) {
            SET_DEATH_FX(14, 100, 200, 128, 255)
        }
    }

    if (g_gameMode == MODE_TIME_TRIAL) {
        // time trial: theme cycles every 10 levels instead
        themeLevelMod10 = g_save.players[g_curPlayer].level % 10;
        if (themeLevelMod10 == 1) {
            SET_DEATH_FX(1, 100, 200, 128, 255)
        }
        if (themeLevelMod10 == 2) {
            SET_DEATH_FX(10, 150, 128, 200, 255)
        }
        if (themeLevelMod10 == 3) {
            SET_DEATH_FX(14, 100, 255, 100, 0)
        }
        if (themeLevelMod10 == 4) {
            SET_DEATH_FX(7, 150, 100, 255, 255)
        }
        if (themeLevelMod10 == 5) {
            SET_DEATH_FX(1, 150, 255, 128, 255)
        }

        if (themeLevelMod10 == 6) {
            SET_DEATH_FX(16, 100, 50, 128, 255)
        }
        if (themeLevelMod10 == 7) {
            SET_DEATH_FX(5, 150, 255, 225, 100)
        }
        if (themeLevelMod10 == 8) {
            SET_DEATH_FX(17, 150, 180, 150, 255)
        }

        if (themeLevelMod10 == 9) {
            SET_DEATH_FX(18, 130, 90, 255, 80)
        }
        if (themeLevelMod10 == 0) {
            SET_DEATH_FX(32, 100, 255, 128, 0)
        }
    }
#undef SET_DEATH_FX

    // ---- load level data ----
    g_mirrorLevel = g_save.players[g_curPlayer].level / LEVEL_THEME_CYCLE & 1; // alternates every 100 levels
    g_curLevelNum = g_save.players[g_curPlayer].level;
    LoadLevelData();
    g_loadedLevel = g_save.players[g_curPlayer].level;
    g_levelDataLoaded = 1;
    g_save.players[g_curPlayer].hurryupComboCount = 0;
    g_guardCount = 1;
    g_levelStartFlag = 1;
    // ---- reset groups / level objects / enemies ----
    for (i = 0; i < MAX_ENEMY_GROUPS; i++) {
        g_groupEnemyCount[i] = 0;
        g_typeKillCombo[i] = 0;
        g_groupKillCount[i] = 0;
        g_bonusKillCombo[i] = 0;
    }

    for (i = 0; i < MAX_LEVEL_OBJS; i++) {
        g_levelObj[i].active = 0;
        g_levelObj[i].x = 0;
        g_levelObj[i].y = 0;
        g_levelObj[i].vx = 0;
        g_levelObj[i].vy = 0;
        g_levelObj[i].type = LOBJ_SHOT;
        g_levelObj[i].unusedTimer = 0;
        g_levelObj[i].hitOffsetX = 0;
        g_levelObj[i].hitOffsetY = 0;
        g_levelObj[i].w = 0;
        g_levelObj[i].h = 0;
        g_levelObj[i].turnDelay = g_defaultObjAlpha;
        g_levelObj[i].f28 = (float)RandRange(0, 2);
    }

    for (i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[g_curPlayer][i].type != ENEMY_CAPTURED) {
            g_enemies[g_curPlayer][i].forcedDir = -1;
            g_enemies[g_curPlayer][i].active = 0;
            g_enemies[g_curPlayer][i].animFrame = (float)RandRange(0, 6);
            g_enemies[g_curPlayer][i].animDelay = 4.0f;
            g_enemies[g_curPlayer][i].animTimer = 4.0f;
            g_enemies[g_curPlayer][i].animReverse = RandRange(0, 2);
            g_enemies[g_curPlayer][i].offsetX = 0;
            g_enemies[g_curPlayer][i].offsetY = 0;
            g_enemies[g_curPlayer][i].oscillatePhase = 0;
            g_enemies[g_curPlayer][i].altFireActive = 0;
            g_enemies[g_curPlayer][i].pairedEnemyIdx = -1;
            g_enemies[g_curPlayer][i].unusedF7c = 10.0f;
            g_enemies[g_curPlayer][i].unusedF80 = RandFloat(0.3f, 2.0f) / 5.0f;

            g_enemies[g_curPlayer][i].fireDelay = 500;
            g_enemies[g_curPlayer][i].zigzagVelX = 0;
            g_enemies[g_curPlayer][i].zigzagTimer = 1;
            g_enemies[g_curPlayer][i].hitFlashTimer = 0;
            g_enemies[g_curPlayer][i].fixedFireDelay = 0;
            g_enemies[g_curPlayer][i].flashActive = 0;
            g_enemies[g_curPlayer][i].flashTimer = 0;

            for (j = 0; j < MAX_BOSS_GUNS; j++) {
                g_enemies[g_curPlayer][i].bossGunASlot[j] = 0;
                g_enemies[g_curPlayer][i].bossGunBSlot[j] = 0;
                g_enemies[g_curPlayer][i].bossGunCSlot[j] = 0;
            }
            g_enemies[g_curPlayer][i].hoverX = g_screenW / 2.0f;
            g_enemies[g_curPlayer][i].hoverY = 50.0f;
        }
    }

    g_bossGunCountA = 0;
    g_bossGunCountB = 0;
    g_bossGunCountC = 0;
    SetHurryUpTimer();

    g_save.players[g_curPlayer].enemySwayX = 0;
    g_save.players[g_curPlayer].enemySwayVelX = 0.5f;
    g_save.players[g_curPlayer].enemySwayMax = 9.0f;
    g_save.players[g_curPlayer].enemySwayMin = -9.0f;
    g_save.players[g_curPlayer].enemySwayAccel = 0.01f;
    g_save.players[g_curPlayer].enemySwayY = 0;
    g_save.players[g_curPlayer].unusedF208 = 0;
    g_save.players[g_curPlayer].unusedF210 = 0.46f;
    g_save.players[g_curPlayer].unusedF214 = -0.46f;
    g_save.players[g_curPlayer].unusedF20c = 0.0154f;
    g_save.players[g_curPlayer].levelEnemyDataCount = g_curLevelData.count;

    // ---- per-mode enemy totals ----
    if (g_gameMode == MODE_DUAL) {
        g_save.players[0].totalEnemies = 0;
        g_save.players[1].totalEnemies = 0;
        g_save.players[0].killed = 0;
        g_save.players[1].killed = 0;
        g_save.players[0].escaped = 0;
        g_save.players[1].escaped = 0;
        g_save.players[0].done = 0;
        g_save.players[1].done = 0;
    } else {
        g_save.players[g_curPlayer].totalEnemies = 0;
        g_save.players[g_curPlayer].spawnReserve = RandRange(2, 6);
        g_save.players[g_curPlayer].killed = 0;
        g_save.players[g_curPlayer].escaped = 0;
        g_save.players[g_curPlayer].done = 0;
    }

    g_save.players[0].chainBonusValue = 2000;
    g_save.players[1].chainBonusValue = 2000;
    SetHurryUpTimer();
    g_enemyAimAtPlayer = 0;
    g_fastEnemyBullets = 0;
    // ---- level-type flags ----
// Sets the per-level ship-destroyed/track-kills flags for the current level type, applying to
// both players in dual mode or just the current player otherwise.
#define SET_LEVEL_FLAGS(destroyed, trackKills)                            \
    if (g_gameMode == MODE_DUAL) {                                        \
        g_save.players[0].shipDestroyedThisLevel = (destroyed);           \
        g_save.players[0].trackKillsFlag = (trackKills);                  \
        g_save.players[1].shipDestroyedThisLevel = (destroyed);           \
        g_save.players[1].trackKillsFlag = (trackKills);                  \
    } else {                                                              \
        g_save.players[g_curPlayer].shipDestroyedThisLevel = (destroyed); \
        g_save.players[g_curPlayer].trackKillsFlag = (trackKills);        \
    }
    switch (g_curLevelData.type) {
    case LEVEL_WAVE:
        g_isWaveLevel = 1;
        if (g_save.players[g_curPlayer].level > WAVE_AIM_LEVEL)
            g_enemyAimAtPlayer = 1;
        if (g_save.players[g_curPlayer].level > WAVE_FAST_BULLET_LEVEL)
            g_fastEnemyBullets = 1;
        SET_LEVEL_FLAGS(0, 0)
        break;

    case LEVEL_WAVE_AIMED:
        if (g_save.players[g_curPlayer].level > WAVE_AIM_LEVEL)
            g_fastEnemyBullets = 1;
        g_enemyAimAtPlayer = 1;
        g_isWaveLevel = 1;
        SET_LEVEL_FLAGS(0, 0)
        break;

    case LEVEL_BONUS_WAVE:
        if (g_save.players[g_curPlayer].level > WAVE_FAST_BULLET_LEVEL)
            g_enemyAimAtPlayer = 1;
        g_diffHurryUpSpeedMax += 0.1f;
        g_isWaveLevel = 1;
        SET_LEVEL_FLAGS(1, 0)
        break;
    case LEVEL_RACE:
        g_diffHurryUpSpeedMax += 0.1f;
        g_killCount++;

        g_isBossLevel = 1;
        g_isWaveLevel = 0;
        SET_LEVEL_FLAGS(0, 1)
        break;

    case LEVEL_BOSS:
        g_isBossLevel = 1;
        g_isWaveLevel = 0;
        SET_LEVEL_FLAGS(0, 0)
        break;
    }
#undef SET_LEVEL_FLAGS
    if (g_gameMode == MODE_TIME_TRIAL) {
        g_isBossLevel = 1;
        g_isWaveLevel = 0;
        g_save.players[g_curPlayer].shipDestroyedThisLevel = 0;
        g_save.players[g_curPlayer].trackKillsFlag = 0;
    }

    g_alienAttackTimer = 0;
    maxEnemyHp = 0;
    spawnedCount = 0;
    g_save.players[g_curPlayer].primaryEnemyCount = 0;
    g_save.players[g_curPlayer].secondaryEnemyCount = 0;
    g_type3EnemySpawnCount = 0;
    g_save.players[g_curPlayer].bonusKilled = 0;
    if (g_gameMode == MODE_DUAL) {
        g_save.players[0].bonusKilled = 0;
        g_save.players[1].bonusKilled = 0;
    }

    // ---- boss spawn ----
    k = 0;
    noFreeSlot = 0;
    if (g_curLevelData.type == LEVEL_BOSS) {
        // boss level: a single boss enemy, spawned into the first free slot in g_enemies[]
        PlayBossMusic();
        slotSearchDone = 0;
        do {
            if (!slotSearchDone) {
                if (g_enemies[g_curPlayer][k].active) {
                    k++;
                    if (k >= MAX_ENEMIES) {
                        slotSearchDone = 1;
                        noFreeSlot = 1;
                    }
                } else {
                    slotSearchDone = 1;
                }
            }
        } while (!slotSearchDone);

        if (!noFreeSlot) {
            g_enemies[g_curPlayer][k].flashActive = 0;
            g_enemies[g_curPlayer][k].flashTimer = 0;
            g_enemies[g_curPlayer][k].fixedFireDelay = 0;
            g_enemies[g_curPlayer][k].active = 1;
            g_enemies[g_curPlayer][k].homing = 0;
            g_enemies[g_curPlayer][k].x = (g_curLevelData.grp[0].spawnX - 16.0f) + g_screenW / 2.0f;
            g_enemies[g_curPlayer][k].y = g_curLevelData.grp[0].spawnY - 16.0f;
            g_enemies[g_curPlayer][k].velX = g_curLevelData.grp[0].velX;
            g_enemies[g_curPlayer][k].velY = g_curLevelData.grp[0].velY;
            g_enemies[g_curPlayer][k].accelX = g_curLevelData.obj[0][0].pathX;
            g_enemies[g_curPlayer][k].accelY = g_curLevelData.obj[0][0].pathY;

            g_enemies[g_curPlayer][k].patternTimer = 1.0f * g_frameDt;
            g_enemies[g_curPlayer][k].patternStep = 0;
            g_enemies[g_curPlayer][k].type = ENEMY_BOSS;
            g_enemies[g_curPlayer][k].locked = 0;
            g_enemies[g_curPlayer][k].hp = (float)((int)(g_diffHpBonusB * 20.0f) + g_curLevelData.hdr[0].hp);
            g_enemies[g_curPlayer][k].maxHp = g_enemies[g_curPlayer][k].hp;
            g_enemies[g_curPlayer][k].attackStaggerTimer = 0;
            g_enemies[g_curPlayer][k].groupIndex = 0;
            g_enemies[g_curPlayer][k].animDelay = (float)RandRange(2, 5);
            g_enemies[g_curPlayer][k].animTimer = g_enemies[g_curPlayer][k].animDelay;
            g_enemies[g_curPlayer][k].animPingPong = g_curLevelData.aux[0].y1;
            g_enemies[g_curPlayer][k].animFrame = 0;

            g_enemies[g_curPlayer][k].score = g_curLevelData.h[0];
            if (g_enemies[g_curPlayer][k].score < 0)
                g_enemies[g_curPlayer][k].score = 0;
            if (g_enemies[g_curPlayer][k].score > ENEMY_SCORE_CAP_BOSS)
                g_enemies[g_curPlayer][k].score = ENEMY_SCORE_CAP_BOSS;

            g_samples[g_curPlayer][k] =
                SoundPlayChannel(g_samples[g_curPlayer][k], g_sfxBossLoop, -1, 250, 0.0f, 127);
            g_engineDroneFreq = RandRange(g_engineFreqMin, g_engineFreqMax);
            g_engineDroneFreqStep = RandRange(-100, 100);
// Records a boss-gun spawn point of type `gunKind` (7/8/9 = gun A/B/C) into the boss enemy's
// next bossGun<letter> slot and marks that gun family active.
#define FILL_BOSS_GUN(letter, gunKind)                                             \
    if (g_curLevelData.grp[bossGunIdx].kind == (gunKind)) {                        \
        g_enemies[g_curPlayer][k].bossGun##letter##Slot[g_bossGunCount##letter] = 1; \
        g_enemies[g_curPlayer][k].bossGun##letter##X[g_bossGunCount##letter] =     \
            (float)g_curLevelData.grp[bossGunIdx].spawnX;                          \
        g_enemies[g_curPlayer][k].bossGun##letter##Y[g_bossGunCount##letter] =     \
            g_curLevelData.grp[bossGunIdx].spawnY + 64.0f - 16.0f;                 \
        g_bossGunCount##letter++;                                                  \
        g_bossGunActive##letter = 1;                                               \
    }
            for (bossGunIdx = 0; bossGunIdx < g_curLevelData.count; bossGunIdx++) {
                FILL_BOSS_GUN(A, 7)

                FILL_BOSS_GUN(B, 8)

                FILL_BOSS_GUN(C, 9)
            }
#undef FILL_BOSS_GUN
            g_enemies[g_curPlayer][k].fireDelay =
                g_enemyFireRateMin > g_curLevelData.hdr[0].fireRateMin ?
                    g_enemyFireRateMin : g_curLevelData.hdr[0].fireRateMin;
            g_enemies[g_curPlayer][k].fireDelayStep = g_curLevelData.hdr[0].fireRateMax;
            k++;
            spawnedCount++;
        }

        if (g_gameMode == MODE_DUAL) {
            g_save.players[0].primaryEnemyCount = spawnedCount;
            g_save.players[0].totalEnemies = spawnedCount;
            g_save.players[1].primaryEnemyCount = spawnedCount;
            g_save.players[1].totalEnemies = spawnedCount;
        } else {
            g_save.players[g_curPlayer].primaryEnemyCount = spawnedCount;
            g_save.players[g_curPlayer].totalEnemies = spawnedCount;
        }
    } else {
        // regular (wave) level: spawn each group's enemies from g_curLevelData.grp[],
        // searching for a free g_enemies[] slot for each one

        if (g_gameMode == MODE_TIME_TRIAL && g_musicMode != 2)
            PlayGameMusic();
        if (g_gameMode != MODE_TIME_TRIAL && g_musicMode != 1)
            PlayGameMusic();
        for (j = 0; j < g_curLevelData.count; j++) {
            spawnStagger = g_curLevelData.grp[j].spawnDelay;
            for (i = 0; i < g_curLevelData.grp[j].count; i++) {
                slotSearchDone = 0;
                do {
                    if (!slotSearchDone) {
                        if (g_enemies[g_curPlayer][k].active) {
                            k++;
                            if (k >= MAX_ENEMIES) {
                                slotSearchDone = 1;
                                noFreeSlot = 1;
                            }
                        } else {
                            slotSearchDone = 1;
                        }
                    }
                } while (!slotSearchDone);

                if (!noFreeSlot) {
                    g_enemyDamageStage[g_curPlayer][k] = 0;
                    switch (g_curLevelData.grp[j].sub[i].type) {
                    case 1:
                        INIT_HAZARD_ENEMY(0, g_curLevelData.w[0], ENEMY_SCORE_CAP_GROUP, g_rectsA)
                        g_enemyDamageStage[g_curPlayer][k] = 0;
                        break;
                    case 2:
                        INIT_HAZARD_ENEMY(1, g_curLevelData.w[1], ENEMY_SCORE_CAP_GROUP, g_rectsA)
                        g_enemyDamageStage[g_curPlayer][k] = 1;
                        break;
                    case 3:
                        INIT_HAZARD_ENEMY(2, g_curLevelData.w[2], ENEMY_SCORE_CAP_GROUP, g_rectsA)
                        g_enemyDamageStage[g_curPlayer][k] = 2;
                        break;

                    case 4:
                        INIT_HAZARD_ENEMY(3, g_curLevelData.w[3], ENEMY_SCORE_CAP_GROUP, g_rectsA)
                        g_enemyDamageStage[g_curPlayer][k] = 3;
                        break;
                    case 5:
                        INIT_HAZARD_ENEMY(4, g_curLevelData.w[4], ENEMY_SCORE_CAP_GROUP, g_rectsA)
                        g_enemyDamageStage[g_curPlayer][k] = 4;
                        break;
                    case 6:
                        INIT_HAZARD_ENEMY(5, g_curLevelData.w[5], ENEMY_SCORE_CAP_GROUP, g_rectsA)
                        g_enemyDamageStage[g_curPlayer][k] = 5;
                        break;
                    }
                    g_enemies[g_curPlayer][k].flashActive = 0;
                    g_enemies[g_curPlayer][k].flashTimer = 0;
                    g_enemies[g_curPlayer][k].fixedFireDelay = 0;
                    g_enemies[g_curPlayer][k].deathExplosionGfx = deathExplosionGfx;
                    g_enemies[g_curPlayer][k].deathExplosionLife = deathExplosionLife;
                    g_enemies[g_curPlayer][k].deathExplosionR = g_levelEnemyColorR;
                    g_enemies[g_curPlayer][k].deathExplosionG = g_levelEnemyColorG;
                    g_enemies[g_curPlayer][k].deathExplosionB = g_levelEnemyColorB;

                    g_enemies[g_curPlayer][k].hazardType = g_curLevelData.grp[j].sub[i].type;
                    g_enemies[g_curPlayer][k].active = 1;
                    g_enemies[g_curPlayer][k].forcedDir = -1;
                    if (!g_mirrorLevel)
                        g_enemies[g_curPlayer][k].hoverX =
                            g_curLevelData.grp[j].sub[i].xOffset + g_screenW / 2.0f;
                    else
                        g_enemies[g_curPlayer][k].hoverX =
                            g_screenW / 2.0f - g_curLevelData.grp[j].sub[i].xOffset;
                    g_enemies[g_curPlayer][k].hoverY = (float)g_curLevelData.grp[j].sub[i].yOffset;
                    if (!g_mirrorLevel)
                        g_enemies[g_curPlayer][k].x =
                            (g_curLevelData.grp[j].spawnX - 16.0f) + g_screenW / 2.0f;
                    else
                        g_enemies[g_curPlayer][k].x =
                            g_screenW / 2.0f - (g_curLevelData.grp[j].spawnX + 16.0f);
                    g_enemies[g_curPlayer][k].y = g_curLevelData.grp[j].spawnY - 16.0f;
                    if (!g_mirrorLevel)
                        g_enemies[g_curPlayer][k].velX = g_curLevelData.grp[j].velX;
                    else
                        g_enemies[g_curPlayer][k].velX = 0 - g_curLevelData.grp[j].velX;
                    g_enemies[g_curPlayer][k].velY = g_curLevelData.grp[j].velY;

                    if (!g_mirrorLevel)
                        g_enemies[g_curPlayer][k].accelX = g_curLevelData.obj[j][0].pathX;
                    else
                        g_enemies[g_curPlayer][k].accelX = 0 - g_curLevelData.obj[j][0].pathX;
                    g_enemies[g_curPlayer][k].accelY = g_curLevelData.obj[j][0].pathY;
                    g_enemies[g_curPlayer][k].oscillateMul = g_curLevelData.grp[j].sub[i].pathId;
                    g_enemies[g_curPlayer][k].oscillateAccel =
                        g_curLevelData.grp[j].sub[i].fireDelay / 512.0f;
                    g_enemies[g_curPlayer][k].patternTimer = 1.0f * g_frameDt;
                    g_enemies[g_curPlayer][k].animPingPong = g_curLevelData.aux[0].y1;
                    g_enemies[g_curPlayer][k].groupIndex = j;
                    g_groupEnemyCount[j]++;
                    g_enemies[g_curPlayer][k].bonusGroupIndex = g_curLevelData.grp[j].groupId;
                    g_groupKillCount[g_curLevelData.grp[j].groupId]++;
                    g_enemies[g_curPlayer][k].patternStep = 0;
                    g_enemies[g_curPlayer][k].type = ENEMY_PATTERNED;
                    g_enemies[g_curPlayer][k].locked = 0;
                    g_enemies[g_curPlayer][k].hp =
                        (float)(g_curLevelData.grp[j].sub[i].hp + (int)g_diffHpBonusA);
                    g_enemies[g_curPlayer][k].maxHp =
                        (float)(g_curLevelData.grp[j].sub[i].hp + (int)g_diffHpBonusA);

                    if (g_enemies[g_curPlayer][k].hp > maxEnemyHp)
                        maxEnemyHp = g_enemies[g_curPlayer][k].hp;
                    g_enemies[g_curPlayer][k].fireDelay =
                        g_enemyFireRateMin > g_curLevelData.grp[j].sub[i].fireRateMin + g_fireDelayBiasB ?
                            g_enemyFireRateMin : g_curLevelData.grp[j].sub[i].fireRateMin + g_fireDelayBiasB;
                    g_enemies[g_curPlayer][k].fireDelayStep = g_curLevelData.grp[j].sub[i].fireRateMax;
                    if (g_curLevelData.grp[j].sub[i].fireDelay == 0 &&
                        g_curLevelData.grp[j].sub[i].pathId == 0) {
                        g_enemies[g_curPlayer][k].attackDelay = g_curLevelData.grp[j].sub[i].fireDelay;
                        g_enemies[g_curPlayer][k].fixedFireDelay = 1;
                    } else {
                        g_enemies[g_curPlayer][k].attackDelay =
                            g_fireDelayMin > g_curLevelData.grp[j].sub[i].fireDelay + g_fireDelayBiasA ?
                                g_fireDelayMin : g_curLevelData.grp[j].sub[i].fireDelay + g_fireDelayBiasA;
                    }
                    if (g_curLevelData.grp[j].sub[i].fireDelay == FIXED_FIRE_DELAY)
                        g_enemies[g_curPlayer][k].attackDelay = g_curLevelData.grp[j].sub[i].fireDelay * 10;
                    g_enemies[g_curPlayer][k].attackDelayStep = g_curLevelData.grp[j].sub[i].pathId;
                    g_enemies[g_curPlayer][k].attackStaggerTimer = (float)spawnStagger;
                    if (g_curLevelData.type != LEVEL_BONUS_WAVE)
                        spawnStagger += g_curLevelData.grp[j].spawnStep;
                    k++;
                    spawnedCount++;

                    if (g_curLevelData.type == LEVEL_RACE)
                        g_type3EnemySpawnCount++;
                }
            }
        }
        if (g_gameMode == MODE_DUAL) {
            g_save.players[0].primaryEnemyCount = spawnedCount;
            g_save.players[0].totalEnemies = spawnedCount;
            g_save.players[1].primaryEnemyCount = spawnedCount;
            g_save.players[1].totalEnemies = spawnedCount;
        } else {
            g_save.players[g_curPlayer].primaryEnemyCount = spawnedCount;
            g_save.players[g_curPlayer].totalEnemies = spawnedCount;
        }
        spawnedCount = 0;
        k = 0;
        noFreeSlot = 0;
        // additional "header" spawn groups (used e.g. by turret/hazard levels), up to 4
        for (j = 0; j < 4; j++) {
            if (g_curLevelData.hdr[j].count > 0) {

                for (i = 0; i < g_curLevelData.hdr[j].count; i++) {
                    slotSearchDone = 0;
                    do {
                        if (!slotSearchDone) {
                            if (g_enemies[g_curPlayer][k].active) {
                                k++;
                                if (k >= MAX_ENEMIES) {
                                    slotSearchDone = 1;
                                    noFreeSlot = 1;
                                }
                            } else {
                                slotSearchDone = 1;
                            }
                        }
                    } while (!slotSearchDone);
                    if (!noFreeSlot) {
                        switch (g_curLevelData.hdr[j].type) {
                        case 1:
                            INIT_HAZARD_ENEMY(0, g_curLevelData.h[0], ENEMY_SCORE_CAP_HEADER, g_rectsB)
                            break;
                        case 2:
                            INIT_HAZARD_ENEMY(1, g_curLevelData.h[1], ENEMY_SCORE_CAP_HEADER, g_rectsB)
                            break;
                        case 3:
                            INIT_HAZARD_ENEMY(2, g_curLevelData.h[2], ENEMY_SCORE_CAP_HEADER, g_rectsB)
                            break;
                        case 4:
                            INIT_HAZARD_ENEMY(3, g_curLevelData.h[3], ENEMY_SCORE_CAP_HEADER, g_rectsB)
                            break;

                        case 5:
                            INIT_HAZARD_ENEMY(4, g_curLevelData.h[4], ENEMY_SCORE_CAP_HEADER, g_rectsB)
                            break;
                        case 6:
                            INIT_HAZARD_ENEMY(5, g_curLevelData.h[5], ENEMY_SCORE_CAP_HEADER, g_rectsB)
                            break;
                        }
#undef INIT_HAZARD_ENEMY
                        g_enemies[g_curPlayer][k].flashActive = 0;
                        g_enemies[g_curPlayer][k].flashTimer = 0;
                        g_enemies[g_curPlayer][k].fixedFireDelay = 0;
                        g_enemies[g_curPlayer][k].deathExplosionGfx = deathExplosionGfx;
                        g_enemies[g_curPlayer][k].deathExplosionLife = deathExplosionLife;
                        g_enemies[g_curPlayer][k].deathExplosionR = g_levelEnemyColorR;
                        g_enemies[g_curPlayer][k].deathExplosionG = g_levelEnemyColorG;
                        g_enemies[g_curPlayer][k].deathExplosionB = g_levelEnemyColorB;
                        g_enemies[g_curPlayer][k].active = 1;

                        g_enemies[g_curPlayer][k].x =
                            (float)(g_screenW / 4 + RandRange(0, (int)g_screenW >> 1));
                        g_enemies[g_curPlayer][k].y = -110.0f;
                        g_enemies[g_curPlayer][k].speedX = 1.0f;
                        g_enemies[g_curPlayer][k].speedY = 1.0f;
                        g_enemies[g_curPlayer][k].patternTimer = 0;
                        g_enemies[g_curPlayer][k].type = ENEMY_WRAPPER;
                        g_enemies[g_curPlayer][k].dirStepTimer = 3.0f;
                        g_enemies[g_curPlayer][k].turnState = 2;
                        g_enemies[g_curPlayer][k].facing = RandRange(0, 5) + 18;
                        g_enemies[g_curPlayer][k].locked = 0;
                        g_enemies[g_curPlayer][k].hp =
                            (float)(g_curLevelData.hdr[j].hp + (int)g_diffHpBonusB);
                        if (g_enemies[g_curPlayer][k].hp > maxEnemyHp)
                            maxEnemyHp = g_enemies[g_curPlayer][k].hp;
                        g_enemies[g_curPlayer][k].fireDelay =
                            g_enemyFireRateMin > g_curLevelData.hdr[j].fireRateMin + g_fireDelayBiasB ?
                                g_enemyFireRateMin : g_curLevelData.hdr[j].fireRateMin + g_fireDelayBiasB;
                        g_enemies[g_curPlayer][k].fireDelayStep = g_curLevelData.hdr[j].fireRateMax;
                        g_enemies[g_curPlayer][k].attackDelay =
                            g_fireDelayMin > g_fireDelayBiasA + 100 ? g_fireDelayMin : g_fireDelayBiasA + 100;
                        g_enemies[g_curPlayer][k].attackDelayStep = 10;
                        g_enemies[g_curPlayer][k].attackStaggerTimer = 0;

                        g_enemies[g_curPlayer][k].turnTimer = RandFloat(0.0f, 200.0f) + 200.0f;
                        g_enemies[g_curPlayer][k].unusedAlpha = 64;
                        g_enemies[g_curPlayer][k].srcX = 0;
                        g_enemies[g_curPlayer][k].srcY = 0;
                        g_enemies[g_curPlayer][k].animDelay = (float)RandRange(2, 5);
                        g_enemies[g_curPlayer][k].animTimer = g_enemies[g_curPlayer][k].animDelay;
                        g_enemies[g_curPlayer][k].animReverse = 0;
                        g_enemies[g_curPlayer][k].animFrame = 0;
                        g_enemies[g_curPlayer][k].animSpeedDivisor = g_curLevelData.hdr[j].hp / 10.0f;
                        g_enemies[g_curPlayer][k].speedScale = 1.0f;
                        g_enemies[g_curPlayer][k].unusedSpawnDelay = RandRange(0, 10) + 6;
                        g_enemies[g_curPlayer][k].animFrameCount = g_curLevelData.aux[j].x1 - 1;
                        g_enemies[g_curPlayer][k].animPingPong = g_curLevelData.aux[j].y1;
                        spawnedCount++;
                        k++;
                    }
                }
            }
        }

        // ---- totals ----
        if (g_gameMode == MODE_DUAL) {
            g_save.players[0].secondaryEnemyCount = spawnedCount;
            g_save.players[0].totalEnemies += spawnedCount;
            g_save.players[1].secondaryEnemyCount = spawnedCount;
            g_save.players[1].totalEnemies += spawnedCount;
        } else {
            g_save.players[g_curPlayer].secondaryEnemyCount = spawnedCount;
            g_save.players[g_curPlayer].totalEnemies += spawnedCount;
        }
    }
}

// Clears map objects, explosions, explosion particles, enemies and level objects for a
// fresh level/game start. Enemies are reset for player 0 in dual-player mode, otherwise
// for the current player.
void ResetObjects()
{
    int i;
    int p;
    p = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        p = 0;

    for (i = 0; i < MAX_MAP_OBJS; i++) {
        g_mapObjs[i].x = 0;
        g_mapObjs[i].y = 0;
        g_mapObjs[i].active = 0;
    }

    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        g_explosions[i].active = 0;
        g_explosions[i].x = 0;
        g_explosions[i].y = 0;
        g_explosions[i].type = 0;
        g_explosions[i].frame = 0;
        g_explosions[i].delay = 0;
        g_explosions[i].angle = 0;
        g_explosions[i].angleVel = 0;
    }

    for (i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
        g_explosionParticles[i].active = 0;
        g_explosionParticles[i].x = 0;
        g_explosionParticles[i].y = 0;
    }

    // Enemies for the reset player.
    for (i = 0; i < MAX_ENEMIES; i++) {
        g_enemies[p][i].forcedDir = -1;
        g_enemies[p][i].active = 0;
        g_enemies[p][i].locked = 0;
        g_enemies[p][i].animFrame = (float)RandRange(0, 6);
        g_enemies[p][i].animDelay = 4.0f;
        g_enemies[p][i].animTimer = 4.0f;
        g_enemies[p][i].animReverse = RandRange(0, 2);
        g_enemies[p][i].offsetX = 0;
        g_enemies[p][i].offsetY = 0;
        g_enemies[p][i].unusedF7c = 10.0f;
        g_enemies[p][i].unusedF80 = RandFloat(0.3f, 2.0f) / 5.0f;
        g_enemies[p][i].fireDelay = 500;
        g_enemies[p][i].zigzagVelX = 0;
        g_enemies[p][i].zigzagTimer = 1;
        g_enemies[p][i].fixedFireDelay = 0;
        g_enemies[p][i].flashActive = 0;
        g_enemies[p][i].flashTimer = 0;
    }

    for (i = 0; i < MAX_LEVEL_OBJS; i++) {
        g_levelObj[i].active = 0;
        g_levelObj[i].x = 0;
        g_levelObj[i].y = 0;
        g_levelObj[i].vx = 0;
        g_levelObj[i].vy = 0;
        g_levelObj[i].type = LOBJ_SHOT;
        g_levelObj[i].unusedTimer = 0;
        g_levelObj[i].hitOffsetX = 0;
        g_levelObj[i].hitOffsetY = 0;
        g_levelObj[i].w = 0;
        g_levelObj[i].h = 0;
        g_levelObj[i].turnDelay = g_defaultObjAlpha;
        g_levelObj[i].f28 = (float)RandRange(0, 2);
    }
}

// Full reset used at game start/restart: clears sparkle flashes, beams, flash effects,
// items, map objects, explosions, explosion particles, enemies (all types, unlike
// ResetObjectsKeep) and level objects.
void ResetAllObjects()
{
    int i;
    int p;
    p = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        p = 0;

    for (i = 0; i < MAX_SPARKLE_FLASHES; i++)
        g_sparkleFlashes[i].active = 0;
    for (i = 0; i < MAX_BEAMS; i++)
        g_beams[i].active = 0;
    for (i = 0; i < MAX_FLASH_RINGS; i++)
        g_flash[i].active = 0;
    for (i = 0; i < MAX_ITEMS; i++) {
        g_items[i].alive = 0;
        g_items[i].active = 1;
    }

    for (i = 0; i < MAX_MAP_OBJS; i++) {
        g_mapObjs[i].x = 0;
        g_mapObjs[i].y = 0;
        g_mapObjs[i].active = 0;
    }
    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        g_explosions[i].active = 0;
        g_explosions[i].x = 0;
        g_explosions[i].y = 0;
        g_explosions[i].type = 0;
        g_explosions[i].frame = 0;
        g_explosions[i].delay = 0;
        g_explosions[i].angle = 0;
        g_explosions[i].angleVel = 0;
    }
    for (i = 0; i < MAX_EXPLOSION_PARTICLES; i++)
        g_explosionParticles[i].active = 0;

    // Enemies (all types, unlike ResetObjectsKeep).
    for (i = 0; i < MAX_ENEMIES; i++) {
        g_enemies[p][i].forcedDir = -1;
        g_enemies[p][i].active = 0;
        g_enemies[p][i].locked = 0;
        g_enemies[p][i].animFrame = (float)RandRange(0, 6);
        g_enemies[p][i].animDelay = 4.0f;
        g_enemies[p][i].animTimer = 4.0f;
        g_enemies[p][i].animReverse = RandRange(0, 2);
        g_enemies[p][i].offsetX = 0;
        g_enemies[p][i].offsetY = 0;
        g_enemies[p][i].unusedF7c = 10.0f;
        g_enemies[p][i].unusedF80 = RandFloat(0.3f, 2.0f) / 5.0f;
        g_enemies[p][i].fireDelay = 500;
        g_enemies[p][i].zigzagVelX = 0;
        g_enemies[p][i].zigzagTimer = 1;
        g_enemies[p][i].fixedFireDelay = 0;
        g_enemies[p][i].flashActive = 0;
        g_enemies[p][i].flashTimer = 0;
    }

    for (i = 0; i < MAX_LEVEL_OBJS; i++) {
        g_levelObj[i].active = 0;
        g_levelObj[i].x = 0;
        g_levelObj[i].y = 0;
        g_levelObj[i].vx = 0;
        g_levelObj[i].vy = 0;
        g_levelObj[i].type = LOBJ_SHOT;
        g_levelObj[i].unusedTimer = 0;
        g_levelObj[i].hitOffsetX = 0;
        g_levelObj[i].hitOffsetY = 0;
        g_levelObj[i].w = 0;
        g_levelObj[i].h = 0;
        g_levelObj[i].turnDelay = g_defaultObjAlpha;
        g_levelObj[i].f28 = (float)RandRange(0, 2);
    }
}

// Like ResetObjects, but also clears sparkle flashes and items, and skips enemies of
// type 8 (kept alive across the reset, e.g. a boss/persistent enemy).
void ResetObjectsKeep()
{
    int i;
    int p;
    p = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        p = 0;

    for (i = 0; i < MAX_SPARKLE_FLASHES; i++)
        g_sparkleFlashes[i].active = 0;
    for (i = 0; i < MAX_ITEMS; i++) {
        g_items[i].alive = 0;
        g_items[i].active = 1;
    }

    for (i = 0; i < MAX_MAP_OBJS; i++) {
        g_mapObjs[i].x = 0;
        g_mapObjs[i].y = 0;
        g_mapObjs[i].active = 0;
    }
    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        g_explosions[i].active = 0;
        g_explosions[i].x = 0;
        g_explosions[i].y = 0;
        g_explosions[i].type = 0;
        g_explosions[i].frame = 0;
        g_explosions[i].delay = 0;
        g_explosions[i].angle = 0;
        g_explosions[i].angleVel = 0;
    }
    for (i = 0; i < MAX_EXPLOSION_PARTICLES; i++)
        g_explosionParticles[i].active = 0;

    // Enemies for the reset player, except persistent type-8 enemies.
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[p][i].type != ENEMY_CAPTURED) {
            g_enemies[p][i].forcedDir = -1;
            g_enemies[p][i].active = 0;
            g_enemies[p][i].locked = 0;
            g_enemies[p][i].animFrame = (float)RandRange(0, 6);
            g_enemies[p][i].animDelay = 4.0f;
            g_enemies[p][i].animTimer = 4.0f;
            g_enemies[p][i].animReverse = RandRange(0, 2);
            g_enemies[p][i].offsetX = 0;
            g_enemies[p][i].offsetY = 0;
            g_enemies[p][i].unusedF7c = 10.0f;
            g_enemies[p][i].unusedF80 = RandFloat(0.3f, 2.0f) / 5.0f;
            g_enemies[p][i].fireDelay = 500;
            g_enemies[p][i].zigzagVelX = 0;
            g_enemies[p][i].zigzagTimer = 1;
            g_enemies[p][i].fixedFireDelay = 0;
            g_enemies[p][i].flashActive = 0;
            g_enemies[p][i].flashTimer = 0;
        }
    }

    for (i = 0; i < MAX_LEVEL_OBJS; i++) {
        g_levelObj[i].active = 0;
        g_levelObj[i].x = 0;
        g_levelObj[i].y = 0;
        g_levelObj[i].vx = 0;
        g_levelObj[i].vy = 0;
        g_levelObj[i].type = LOBJ_SHOT;
        g_levelObj[i].unusedTimer = 0;
        g_levelObj[i].hitOffsetX = 0;
        g_levelObj[i].hitOffsetY = 0;
        g_levelObj[i].w = 0;
        g_levelObj[i].h = 0;
        g_levelObj[i].turnDelay = g_defaultObjAlpha;
        g_levelObj[i].f28 = (float)RandRange(0, 2);
    }
}

// Same as UpdateGetReadyNewLevel but for respawning after death: rebuilds the
// get-ready/level-name banner, resets the sound queue and re-arms g_timerA for the
// next respawn cycle.
void UpdateGetReadyRespawn()
{
    int i;
    int r;

    if (g_timerA != 0 && g_time > g_timerA) {
        g_timerA = 0;
        g_introDone = 0;
        g_getReadyFlagA = 0;
        g_state = STATE_PLAYING;
        TimerStart1();
    }
    if (g_timerA != 0 && g_introDone == 0) {
        if (g_gameMode == MODE_DUAL)
            g_curPlayer = 0;
        if (g_save.players[g_curPlayer].started == 0) {
            g_save.players[g_curPlayer].started = 1;
            g_save.players[g_curPlayer].levelWarpPending = 0;
            StartLevel();
        }

        // Build the "get ready" and level-name banner text for this mode.
        switch (g_gameMode) {
        case MODE_SINGLE:
            sprintf(g_getReadyText, "G E T   R E A D Y");
            break;
        case MODE_TWO_PLAYER:
            sprintf(g_getReadyText, "GET READY PLAYER %d", g_curPlayer + 1);
            break;
        case MODE_DUAL:
            sprintf(g_getReadyText, "G E T   R E A D Y   P L A Y E R S");
            break;
        case MODE_TEAM:
            sprintf(g_getReadyText, "G E T   R E A D Y   T E A M");
            break;
        case MODE_ACE_TOURNAMENT:
            sprintf(g_getReadyText, "GET READY PLAYER %d", g_curPlayer + 1);
            break;
        case MODE_TIME_TRIAL:
            sprintf(g_getReadyText, "G E T   R E A D Y");
            break;
        }

        if (g_gameMode != MODE_TIME_TRIAL) {
            sprintf(g_levelBannerText, "LEVEL %d", g_save.players[g_curPlayer].level);
            for (i = 0; i < g_curLevelData.name1[0]; i++)
                g_levelName[i] = (&g_curLevelData.name1[1])[i];
            g_levelName[g_curLevelData.name1[0]] = 0;
        } else {
            g_levelBannerText[0] = 0;
            g_levelName[0] = 0;
        }
        g_levelBannerTime = g_time + 2000;
        SoundResetQueue();

        // Pick a random get-ready voice line among the ones that exist.
        r = 1;
        if (g_sfxGetReady != 0)
            r++;
        if (g_sfxGetReady2 != 0)
            r++;
        if (g_sfxGetReady3 != 0)
            r++;
        r = RandRange(0, r);
        if (r == 0)
            SoundQueueAdd(g_sfxGetReady, 50, 1);
        if (r == 1)
            SoundQueueAdd(g_sfxGetReady, 50, 1);
        if (r == 2)
            SoundQueueAdd(g_sfxGetReady2, 50, 1);
        if (r == 3)
            SoundQueueAdd(g_sfxGetReady3, 50, 1);

        // Queue the "player 1/2/both" voice line for 2-player modes.
        switch (g_gameMode) {
        case MODE_TWO_PLAYER:
            if (g_curPlayer == 0)
                SoundQueueAdd(g_sfxPlayer1, 50, 1);
            else
                SoundQueueAdd(g_sfxPlayer2, 50, 1);
            break;
        case MODE_DUAL:
            if (g_save.players[0].lives > g_shipDefs[g_save.players[0].ship]->minEnergy &&
                g_save.players[1].lives > g_shipDefs[g_save.players[1].ship]->minEnergy)
                SoundQueueAdd(g_sfxPlayers, 50, 1);
            else if (g_save.players[0].lives > g_shipDefs[g_save.players[0].ship]->minEnergy)
                SoundQueueAdd(g_sfxPlayer1, 50, 1);
            else
                SoundQueueAdd(g_sfxPlayer2, 50, 1);
            break;
        }
        g_introDone = 1;
        g_timerA = g_time + 2000;
    }
}

// Drives the "get ready" countdown when a fresh level starts: starts the level once,
// builds the get-ready/level-name banner text and queues a random get-ready voice line,
// then drops back to gameplay (state 2) once the timer runs out.
void UpdateGetReadyNewLevel()
{
    int i;
    int r;

    if (g_state == STATE_HISCORE_TABLE)
        return;
    if (g_timerA != 0 && g_time > g_timerA)
        g_timerA = 0;
    if (g_timerB != 0 && g_time > g_timerB) {
        g_timerB = 0;
        g_introDone = 0;
        g_getReadyFlagA = 0;
        g_state = STATE_PLAYING;
    }
    if (g_timerB != 0 && g_introDone == 0) {
        if (g_save.players[g_curPlayer].started == 0) {
            g_save.players[g_curPlayer].started = 1;
            StartLevel();
        }

        // Build the "get ready" and level-name banner text for this mode.
        switch (g_gameMode) {
        case MODE_SINGLE:
            sprintf(g_getReadyText, "G E T   R E A D Y");
            break;
        case MODE_TWO_PLAYER:
            sprintf(g_getReadyText, "GET READY PLAYER %d", g_curPlayer + 1);
            break;
        case MODE_DUAL:
            sprintf(g_getReadyText, "G E T   R E A D Y   P L A Y E R S");
            break;
        case MODE_TEAM:
            sprintf(g_getReadyText, "G E T   R E A D Y   P L A Y E R S");
            break;
        case MODE_ACE_TOURNAMENT:
            sprintf(g_getReadyText, "GET READY PLAYER %d", g_curPlayer + 1);
            break;
        case MODE_TIME_TRIAL:
            sprintf(g_getReadyText, "G E T   R E A D Y");
            break;
        }

        sprintf(g_levelBannerText, "LEVEL %d", g_save.players[g_curPlayer].level);
        for (i = 0; i < g_curLevelData.name1[0]; i++)
            g_levelName[i] = (&g_curLevelData.name1[1])[i];
        g_levelName[g_curLevelData.name1[0]] = 0;
        g_levelBannerTime = g_time + 2000;

        // Pick a random get-ready voice line among the ones that exist.
        r = 1;
        if (g_sfxGetReady != 0)
            r++;
        if (g_sfxGetReady2 != 0)
            r++;
        if (g_sfxGetReady3 != 0)
            r++;
        r = RandRange(0, r);
        if (r == 0)
            SoundQueueAdd(g_sfxGetReady, 50, 1);
        if (r == 1)
            SoundQueueAdd(g_sfxGetReady, 50, 1);
        if (r == 2)
            SoundQueueAdd(g_sfxGetReady2, 50, 1);
        if (r == 3)
            SoundQueueAdd(g_sfxGetReady3, 50, 1);

        // Queue the "player 1/2/both" voice line for 2-player modes.
        switch (g_gameMode) {
        case MODE_TWO_PLAYER:
            if (g_curPlayer == 0)
                SoundQueueAdd(g_sfxPlayer1, 50, 1);
            else
                SoundQueueAdd(g_sfxPlayer2, 50, 1);
            break;
        case MODE_DUAL:
            if (g_save.players[0].lives > g_shipDefs[g_save.players[0].ship]->minEnergy &&
                g_save.players[1].lives > g_shipDefs[g_save.players[1].ship]->minEnergy)
                SoundQueueAdd(g_sfxPlayers, 50, 1);
            else if (g_save.players[0].lives > g_shipDefs[g_save.players[0].ship]->minEnergy)
                SoundQueueAdd(g_sfxPlayer1, 50, 1);
            else
                SoundQueueAdd(g_sfxPlayer2, 50, 1);
            break;
        }
        g_introDone = 1;
    }
}
