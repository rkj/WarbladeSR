// hurryup.cpp: Hurry-up: the timer and the mothership event.
#include <stdio.h>
#include "globals.h"
#include "game.h"
#include "bass.h"

// Hurry-up pacing: interval floor (ms until the next event can't shrink below this), the amount
// it shrinks by on each event, the mothership's temporary effect duration, and how often (every
// Nth hurry-up) a bonus money-ship also spawns.
enum {
    HURRYUP_INTERVAL_MIN  = 40,
    HURRYUP_INTERVAL_STEP = 8,
    HURRYUP_EFFECT_MS     = 10000,
    MONEY_SHIP_EVERY      = 8
};


// Arms the hurry-up event timer (g_bonusDuration from now) for the active player(s) or, in bonus-round
// mode, the shared g_lastEventTime; also unconditionally sets g_lastEventTime at the end (kept as-is).
void SetHurryUpTimer()
{
    switch (g_gameMode) {
    case MODE_SINGLE:
        g_save.players[g_curPlayer].effectDuration = g_bonusDuration;
        g_save.players[g_curPlayer].time = g_time + g_save.players[g_curPlayer].effectDuration;
        break;

    case MODE_TWO_PLAYER:
        g_save.players[g_curPlayer].effectDuration = g_bonusDuration;
        g_save.players[g_curPlayer].time = g_time + g_save.players[g_curPlayer].effectDuration;
        break;

    case MODE_DUAL:
        g_save.players[0].effectDuration = g_bonusDuration;
        g_save.players[0].time = g_time + g_save.players[0].effectDuration;
        break;

    case MODE_TEAM:
        g_lastEventTime = g_time + g_bonusDuration;
        break;

    case MODE_UNUSED_4:
        break;

    case MODE_ACE_TOURNAMENT:
        break;

    case MODE_TIME_TRIAL:
        g_save.players[g_curPlayer].effectDuration = g_bonusDuration;
        g_save.players[g_curPlayer].time = g_time + g_save.players[g_curPlayer].effectDuration;
    }

    // NOTE: runs unconditionally after the switch, so g_lastEventTime is set the same way in every mode.
    g_lastEventTime = g_time + g_bonusDuration;
}

// Returns the active player's hurry-up deadline (g_time value); mirrors the per-mode player selection in
// SetHurryUpTimer/GetHurryUpTimer's callers.
unsigned int GetHurryUpTimer(void)
{
    int t;

    t = g_save.players[g_curPlayer].time;
    switch (g_gameMode) {
    case MODE_SINGLE:
        t = g_save.players[g_curPlayer].time;
        break;
    case MODE_TWO_PLAYER:
        t = g_save.players[g_curPlayer].time;
        break;
    case MODE_DUAL:
        t = g_save.players[0].time;
        break;
    case MODE_TEAM:
        break;
    case MODE_UNUSED_4:
        break;
    case MODE_ACE_TOURNAMENT:
        break;
    case MODE_TIME_TRIAL:
        t = g_save.players[g_curPlayer].time;
    }
    return t;
}

// Checks whether it's time for a "hurry up" event (past the hurry-up deadline, level still running, not a
// bonus/boss level) and if so spawns a mothership enemy (type 9) flying in from a random side, queues its
// sound/message, nudges the guard trigger points, and shortens the hurry-up interval for next time. Every
// 8th hurry-up also spawns a bonus money-ship (type 12) alongside it.
void HurryUp()
{
    int i;
    int p;
    int r;
    int j;
    int k;

    p = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        p = 0;

    if (g_time > GetHurryUpTimer() && g_save.players[g_curPlayer].done == 0 && g_curLevelData.type != LEVEL_BOSS
        && g_gameMode != MODE_TIME_TRIAL && g_curLevelData.type != LEVEL_RACE) {
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (g_enemies[p][i].active == 0) {
                // 50/50: enter from the right (flying left, turnState 1) or from the left (turnState 0).
                if (RandRange(0, 100) < 50) {
                    g_enemies[p][i].turnState = 1;
                    g_enemies[p][i].x = g_screenW;
                    SoundPlay(g_sfxMothership, -1, 0xf0, 1.0f, 0x7f, g_sndFlags);
                } else {
                    g_enemies[p][i].turnState = 0;
                    g_enemies[p][i].x = 0.0f - g_mothershipMaskW;
                    SoundPlay(g_sfxMothership, -1, 0xf0, -1.0f, 0x7f, g_sndFlags);
                }
                g_samples[p][i] = SoundPlayChannel(g_samples[p][i], g_sfxMotherLoop, -1, 0, 0.0f, 0x7f);
                // Slide the engine-hum channel's volume up to its table-190 level over 800ms.
                BASS_ChannelSlideAttribute(g_samples[p][i], BASS_ATTRIB_VOL, (float)(g_sfxVolTable[190] / 255.0), 800);

                // Pick and queue the "hurry up" message/voice.
                sprintf(g_alertMsg, "H U R R Y   U P");
                r = 0;
                if (g_sfxHurryUp1 != 0)
                    r++;
                if (g_sfxHurryUp2 != 0)
                    r++;
                r = RandRange(0, r);
                if (r == 0 && g_sfxHurryUp1 != 0)
                    SoundQueueAdd(g_sfxHurryUp1, 50, 1);
                if (r == 1)
                    SoundQueueAdd(g_sfxHurryUp2, 50, 1);
                g_msgColor = 0;
                g_msgTimer = g_time + 1000;

                // Nudge the guard trigger points and spawn the mothership.
                for (j = 0; j < g_guardCount; j++)
                    g_triggerX[j] = RandRange(0x80, g_screenW - 0x80);
                g_guardCount++;
                if (g_guardCount > 8)
                    g_guardCount = 8;
                g_enemies[p][i].speedX = RandFloat(2.0f, g_diffHurryUpSpeedMax);
                g_enemies[p][i].active = 1;
                g_enemies[p][i].y = 20.0f;
                g_enemies[p][i].dirStepTimer = RandFloat(2.0f, 4.0f);
                g_enemies[p][i].gfxA = g_gfxMothership;
                g_enemies[p][i].gfxB = g_gfxMothershipMask;
                g_enemies[p][i].shotFrame = (void *)g_hmaMothership;
                g_enemies[p][i].shotGfxW = g_mothershipMaskW;
                g_enemies[p][i].shotGfxH = g_mothershipMaskH;
                g_enemies[p][i].srcX = 0;
                g_enemies[p][i].srcY = 0;
                g_enemies[p][i].score = DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_2500]);
                g_enemies[p][i].hp = (int)g_diffHpBonusA + g_diffEnemyHpBonus;
                g_enemies[p][i].locked = 0;
                g_enemies[p][i].type = ENEMY_MOTHERSHIP;

                if (g_gameMode == MODE_DUAL) {
                    g_save.players[0].totalEnemies++;
                    g_save.players[1].totalEnemies++;
                } else {
                    g_save.players[g_curPlayer].totalEnemies++;
                }
                g_save.players[g_curPlayer].hurryupComboCount++;

                // Every 8th hurry-up event also spawns a bonus money-ship in a free slot.
                if (g_save.players[g_curPlayer].hurryupComboCount == MONEY_SHIP_EVERY) {
                    g_save.players[g_curPlayer].hurryupComboCount = 0;
                    for (k = 0; k < MAX_ENEMIES; k++) {
                        if (g_enemies[p][k].active == 0) {
                            if (g_gameMode == MODE_DUAL) {
                                g_save.players[0].totalEnemies++;
                                g_save.players[1].totalEnemies++;
                            } else {
                                g_save.players[g_curPlayer].totalEnemies++;
                            }

                            g_enemies[p][k].gfxA = g_gfxMoneyShip;
                            g_enemies[p][k].gfxB = g_gfxMoneyShipMask;
                            g_enemies[p][k].shotFrame = g_hmaMoneyShip;
                            g_enemies[p][k].shotGfxW = g_moneyShipGfxW;
                            g_enemies[p][k].shotGfxH = g_moneyShipGfxH;
                            g_enemies[p][k].score = DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_25000_B]);
                            g_enemies[p][k].active = 1;
                            g_enemies[p][k].x = RandRange(0, (int)(g_screenW - SPAWN_X_SPAN) >> 1) + SPAWN_X_MARGIN;
                            g_enemies[p][k].y = -110.0f;
                            g_enemies[p][k].speedX = 1.0f;
                            g_enemies[p][k].speedY = 1.0f;
                            g_enemies[p][k].patternTimer = 0.0f;
                            g_enemies[p][k].type = ENEMY_MONEY_SHIP;
                            g_enemies[p][k].dirStepTimer = 3.0f;
                            g_enemies[p][k].turnState = 2;
                            g_enemies[p][k].facing = RandRange(0, 5) + 18;
                            g_enemies[p][k].hp = (int)g_diffHpBonusA + g_hurryUpHpBonus;
                            g_enemies[p][k].locked = 0;
                            g_enemies[p][k].attackStaggerTimer = 0.0f;
                            g_enemies[p][k].turnTimer = RandFloat(0.0f, 100.0f) + 50;
                            g_enemies[p][k].unusedAlpha = 0x80;
                            g_enemies[p][k].srcX = 0;
                            g_enemies[p][k].srcY = 0;

                            g_enemies[p][k].animSpeedDivisor = g_enemies[p][i].hp / 5;
                            if (g_enemies[p][k].animSpeedDivisor > 0)
                                g_enemies[p][k].animTimer =
                                    g_enemies[p][k].hp / g_enemies[p][k].animSpeedDivisor / 4.0;
                            else
                                g_enemies[p][k].animTimer = g_enemies[p][k].hp / 1 / 4.0;
                            g_enemies[p][k].animReverse = RandRange(0, 2);
                            g_enemies[p][k].animFrame = 0.0f;
                            g_enemies[p][k].speedScale = RandFloat(0.8f, 3.0f);
                            g_enemies[p][k].unusedSpawnDelay = RandRange(0, 3) + 2;
                            g_enemies[p][k].animStepTime = RandFloat(0.0f, 8.0f) + 3;
                            g_enemies[p][k].animFrameCount = 9;
                            g_enemies[p][k].animPingPong = 1;
                            g_enemies[p][k].unused1c4 = 0;
                            g_enemies[p][k].unused1c8 = 0;

                            // NOTE: unused1cc assigned twice (0x80) in the original; kept for the byte match.
                            g_enemies[p][k].unused1cc = 0x80;
                            g_enemies[p][k].unused1cc = 0x80;
                            g_enemies[p][k].fixedFireDelay = 0;
                            g_enemies[p][k].flashActive = 0;
                            g_enemies[p][k].flashTimer = 0.0f;
                            g_samples[p][k] =
                                SoundPlayChannel(g_samples[p][k], g_sfxShipHumLoop, 40000, 0xff, 0.0f, 0x7f);
                            break;
                        }
                    }
                }
                // Restart the hurry-up effect duration (10s) for the mode's active player(s), and shrink
                // the interval until the next hurry-up (floor 40) so events come faster as the level goes on.
                switch (g_gameMode) {
                case MODE_SINGLE:
                case MODE_TWO_PLAYER:
                    g_save.players[g_curPlayer].effectDuration = HURRYUP_EFFECT_MS;
                    g_save.players[g_curPlayer].time = g_time + g_save.players[g_curPlayer].effectDuration;
                    g_hurryUpInterval -= HURRYUP_INTERVAL_STEP;
                    if (g_hurryUpInterval < HURRYUP_INTERVAL_MIN)
                        g_hurryUpInterval = HURRYUP_INTERVAL_MIN;
                    break;

                case MODE_DUAL:
                    g_save.players[0].effectDuration = HURRYUP_EFFECT_MS;
                    g_save.players[0].time = g_time + g_save.players[0].effectDuration;
                    g_hurryUpInterval -= HURRYUP_INTERVAL_STEP;
                    if (g_hurryUpInterval < HURRYUP_INTERVAL_MIN)
                        g_hurryUpInterval = HURRYUP_INTERVAL_MIN;
                    break;

                case MODE_TEAM:
                    g_bonusDuration = HURRYUP_EFFECT_MS;
                    g_lastEventTime = g_time + g_bonusDuration;
                    g_hurryUpInterval -= HURRYUP_INTERVAL_STEP;
                    if (g_hurryUpInterval < HURRYUP_INTERVAL_MIN)
                        g_hurryUpInterval = HURRYUP_INTERVAL_MIN;
                    break;

                case MODE_UNUSED_4:
                    break;

                case MODE_ACE_TOURNAMENT:
                    break;

                case MODE_TIME_TRIAL:
                    g_save.players[g_curPlayer].effectDuration = HURRYUP_EFFECT_MS;
                    g_save.players[g_curPlayer].time = g_time + g_save.players[g_curPlayer].effectDuration;
                    g_hurryUpInterval -= HURRYUP_INTERVAL_STEP;
                    if (g_hurryUpInterval < HURRYUP_INTERVAL_MIN)
                        g_hurryUpInterval = HURRYUP_INTERVAL_MIN;
                    break;
                }
                break;
            }
        }
    }
}
