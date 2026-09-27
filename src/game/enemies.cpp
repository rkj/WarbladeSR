// enemies.cpp: Enemies: UpdateEnemies (one switch on type) and drawing, fire boost, the money
// sucker and elite flyby.
#include "globals.h"
#include "game.h"
#include "bass.h"

// Level-pattern step commands: three separate tables whose values don't overlap in meaning.
enum {                              // g_curLevelData.obj[].cmd (patterned/hover/diving aliens)
    OPCMD_STOP_TURN = 1,
    OPCMD_FLASH     = 2,
    OPCMD_ESCAPE    = 6
};
enum {                              // g_patterns[].entries[].type (FP.type, standalone dive patterns)
    PATCMD_STOP_TURN = 1,
    PATCMD_ESCAPE    = 6
};
enum {                              // boss movement-pattern step command (PT.cmd)
    BOSSCMD_STOP  = 1,
    BURST_FIRE    = 2,
    ARM_GUNS      = 3,
    BOSSCMD_HOMING = 7
};

enum { PATTERN_HOLD_END = 100 };    // .obj[].holdTime == 100: step ends the group's path
enum { CHEAT_DETECT_TRIP = 40000 }; // g_cheatDetectFlag threshold that arms the ANTI_CHEAT roll

// Occasionally spawns a money-sucker enemy (type 11) once the player is carrying enough
// money; skipped on bonus levels (type 3/4) and rate-limited by g_moneySuckerCooldown.
void SpawnMoneySucker()
{
    int set;
    int n;
    int i;
    int j;

    set = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        set = 0;
    if (g_curLevelData.type == LEVEL_RACE || g_curLevelData.type == LEVEL_BOSS)
        return;

    // Tuning: 750 money threshold to start rolling, 1340 scaling divisor, 40000 roll ceiling,
    // 120000ms (2 min) spawn cooldown below.
    n = g_save.players[g_curPlayer].money;
    if (n > 750 && g_save.players[g_curPlayer].done == 0) {
        n = n / 1340 + RandRange(3, 10);
        if (n > 0) {
            if (RandRange(0, 40000) < n) {
                if (RandFloat(0.0f, 200.0f) < g_frameDt * 7.0 && g_time > g_moneySuckerCooldown) {
                    for (i = 0; i < MAX_ENEMIES; i++) {
                        if (g_enemies[set][i].type == ENEMY_MONEY_SUCKER)
                            return;
                    }
                    g_moneySuckerCooldown = g_time + 120000;

                    // Find a free enemy slot and spawn the sucker at a random screen edge.
                    for (j = 0; j < MAX_ENEMIES; j++) {
                        if (g_enemies[set][j].active == 0) {
                            if (RandRange(0, 100) < 50) {
                                g_enemies[set][j].turnState = 1;
                                g_enemies[set][j].x = (float)(g_screenW + 70);
                            } else {
                                g_enemies[set][j].turnState = 0;
                                g_enemies[set][j].x = -70.0f;
                            }
                            if (g_gameMode == MODE_DUAL) {
                                g_save.players[0].totalEnemies++;
                                g_save.players[1].totalEnemies++;
                            } else {
                                g_save.players[g_curPlayer].totalEnemies++;
                            }
                            g_enemies[set][j].speedX = RandFloat(0.5f, 1.5f);

                            g_enemies[set][j].active = 1;
                            g_enemies[set][j].y = (float)(RandRange(0, g_screenH - 450) + 200);
                            g_enemies[set][j].dirStepTimer = RandFloat(2.0f, 5.0f);
                            g_enemies[set][j].gfxA = g_gfxMoneySucker;
                            g_enemies[set][j].gfxB = g_gfxMoneySuckerMask;
                            g_enemies[set][j].shotFrame = g_hmaMoneySucker;
                            g_enemies[set][j].shotGfxW = g_moneySuckerWidth;
                            g_enemies[set][j].shotGfxH = g_moneySuckerHeight;
                            g_enemies[set][j].srcX = 0;
                            g_enemies[set][j].srcY = 0;
                            g_enemies[set][j].score = DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_50000]);
                            g_enemies[set][j].hp = (float)(g_moneySuckerBaseHp + (int)g_diffHpBonusB * 2);
                            g_enemies[set][j].locked = 0;
                            g_enemies[set][j].type = ENEMY_MONEY_SUCKER;

                            g_enemies[set][j].unused1c4 = 0;
                            g_enemies[set][j].unused1c8 = 0;
                            g_enemies[set][j].unused1cc = 0x80;
                            g_enemies[set][j].unused1d0 = 0x32;
                            g_enemies[set][j].attackStaggerTimer = 0.0f;
                            g_enemies[set][j].fixedFireDelay = 0;
                            g_enemies[set][j].flashActive = 0;
                            g_enemies[set][j].flashTimer = 0.0f;
                            return;
                        }
                    }
                }
            }
        }
    }
}

// Occasionally spawns an elite guard flyby enemy (type 18) once the player is past level
// 15, at least 10 levels since the last one; skipped in time-trial mode and on bonus levels.
void SpawnEliteFlyby()
{
    int set;
    int i;
    int j;

    set = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        set = 0;
    if (1 &&  // NOTE: always-true, kept for the byte match
        g_save.players[set].done == 0 && g_gameMode != MODE_TIME_TRIAL && g_save.players[set].level > 15
        && g_curLevelData.type != LEVEL_BOSS && g_curLevelData.type != LEVEL_RACE) {
        if (g_save.players[set].level - g_lastEliteSpawnLevel < 10)
            return;

        // Tuning: 60000/20.0 roll rate, then a further 1-in-500 chance (20000 range, > 19500) per attempt.
        if (RandFloat(0.0f, 60000.0f) < g_frameDt * 20.0 && RandRange(0, 20000) > 19500) {
            for (i = 0; i < MAX_ENEMIES; i++) {
                if (g_enemies[set][i].type == ENEMY_GUARD)
                    return;
            }

            // Find a free enemy slot and spawn the elite guard at a random screen edge.
            for (j = 0; j < MAX_ENEMIES; j++) {
                if (g_enemies[set][j].active == 0) {
                    if (RandRange(0, 100) < 50) {
                        g_enemies[set][j].turnState = 1;
                        g_enemies[set][j].x = (float)(g_screenW + 70);
                    } else {
                        g_enemies[set][j].turnState = 0;
                        g_enemies[set][j].x = -70.0f;
                    }
                    g_lastEliteSpawnLevel = g_save.players[set].level;
                    if (g_gameMode == MODE_DUAL) {
                        g_save.players[0].totalEnemies++;
                        g_save.players[1].totalEnemies++;
                    } else {
                        g_save.players[g_curPlayer].totalEnemies++;
                    }
                    g_enemies[set][j].speedX = RandFloat(0.3f, 1.0f) * g_frameDt;

                    g_enemies[set][j].active = 1;
                    g_enemies[set][j].y = (float)(RandRange(0, g_screenH - 450) + 200);
                    g_enemies[set][j].dirStepTimer = RandFloat(2.0f, 5.0f);
                    g_enemies[set][j].gfxA = g_gfxGuard;
                    g_enemies[set][j].gfxB = g_gfxGuardMask;
                    g_enemies[set][j].shotFrame = g_hmaGuard;
                    g_enemies[set][j].shotGfxW = g_guardWidth;
                    g_enemies[set][j].shotGfxH = g_guardHeight;
                    g_enemies[set][j].srcX = 0;
                    g_enemies[set][j].srcY = 0;
                    g_enemies[set][j].score = DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_10000000]);
                    g_enemies[set][j].hp = (float)((int)g_diffHpBonusB * 10 + g_eliteHpBonus);
                    g_enemies[set][j].type = ENEMY_GUARD;
                    g_enemies[set][j].locked = 0;

                    g_enemies[set][j].unused1c4 = 0;
                    g_enemies[set][j].unused1c8 = 0;
                    g_enemies[set][j].unused1cc = 0x80;
                    g_enemies[set][j].unused1d0 = 0x40;
                    g_enemies[set][j].attackStaggerTimer = 0.0f;
                    g_enemies[set][j].fixedFireDelay = 0;
                    g_enemies[set][j].flashActive = 0;
                    g_enemies[set][j].flashTimer = 0.0f;
                    g_samples[set][j] = SoundPlayChannel(g_samples[set][j], g_sfxGuardLoop, -1, SFX_VOL_FULL,
                                                          0.0f, SFX_PAN_CENTER);
                    return;
                }
            }
        }
    }
}

// Randomly discounts a fire-related value (`val`, e.g. a cooldown or damage stat) when
// the current player is within 50 px of `x`, closer discounts more: under 10px cuts it
// by 3/4, under 30px by half, under 50px by a quarter. Used to make weapons near the
// player a bit more forgiving/deadly.
int NearPlayerFireBoost(int x, int val)
{
    int d;
    int q;

    if ((d = (int)g_save.players[g_curPlayer].x - x) < 0)
        d = 0 - d;
    if (d < 50) {
        q = val / 4;
        if (Rand7f() < 10) {
            if (d < 10)
                return val -= q * 3;
            if (d < 30)
                return val -= q * 2;
            if (d < 50)
                return val -= q;
        }
    }
    return val;
}

// VF: forces the operand to be evaluated first (matches original operand order)
#define VF(x) (*(volatile float *)&(x))

#define PL g_save.players[g_curPlayer]

#define EN g_enemies[g_curPlayer][enemyIdx]   // the enemy slot currently being updated (outer loop index)

// E2: a second enemy slot, used when scanning for a pairing partner
#define E2 g_enemies[g_curPlayer][partnerIdx]

#define SH g_levelObj[objIdx]               // the level-object (shot/rocket) slot being filled in when firing

// PT: EN's current movement-pattern step (group path data)
#define PT g_curLevelData.obj[EN.groupIndex][EN.patternStep]

// FP: EN's current step in a standalone attack pattern
#define FP g_patterns[EN.patternId].entries[EN.patternStep]

// Spawns a shot at EN's gun position and plays the matching alien-shoot sound if it ended up on screen.
#define SHOT_SOUNDS \
    if (SH.x > 0.0 && SH.x < g_screenW && SH.y > 0.0 && SH.y < g_screenH) { \
        if ((int)EN.gfxA == (int)g_alienGfxCache[0].gfx1) \
            SoundPlay(g_sndAlienShoot10, -1, g_rampB[ClampY((int)SH.y)], \
                             g_panTable[ClampY((int)SH.x)], SFX_PAN_CENTER, g_sndFlags); \
        if ((int)EN.gfxA == (int)g_alienGfxCache[1].gfx1) \
            SoundPlay(g_sndAlienShoot12, -1, g_rampB[ClampY((int)SH.y)], \
                             g_panTable[ClampY((int)SH.x)], SFX_PAN_CENTER, g_sndFlags); \
        if ((int)EN.gfxA == (int)g_alienGfxCache[2].gfx1) \
            SoundPlay(g_sndAlienShoot5, -1, g_rampB[ClampY((int)SH.y)], \
                             g_panTable[ClampY((int)SH.x)], SFX_PAN_CENTER, g_sndFlags); \
    }

// Fires a straight-down shot from EN into a free g_levelObj slot (aimed at the player if aim-at-player
// is on).
#define SHOT_FIRE_A \
    for (objIdx = 0; objIdx < MAX_LEVEL_OBJS; objIdx++) { \
        if (SH.active == 0) { \
            SH.turnDelay = g_defaultObjAlpha; \
            SH.active = 1; \
            SH.x = EN.x + 13.0f; \
            SH.y = EN.y + 16.0f; \
            SH.type = LOBJ_SHOT; \
            if (g_gameMode == MODE_TIME_TRIAL || g_enemyAimAtPlayer) { \
                if (PL.x < EN.x) \
                    SH.vx = RandFloat(-1.5f, 0) * g_frameDt; \
                else \
                    SH.vx = RandFloat(0, 1.5f) * g_frameDt; \
            } else { \
                SH.vx = 0; \
            } \
            if (g_fastEnemyBullets) \
                SH.vy = g_enemyBulletSpeed * 1.25f * g_frameDt; \
            else \
                SH.vy = g_enemyBulletSpeed * g_frameDt; \
            SH.gfxA = (KGraphic *)EN.gfxA; \
            SH.gfxB = (KGraphic *)EN.gfxB; \
            SH.hma = (int)EN.shotFrame; \
            SH.hmaW = EN.shotGfxW; \
            SH.hmaH = EN.shotGfxH; \
            SH.hitOffsetX = EN.frameSet.aLeft; \
            SH.hitOffsetY = EN.frameSet.aTop; \
            SH.w = EN.frameSet.aWidth; \
            SH.h = EN.frameSet.aHeight; \
            SHOT_SOUNDS \
            break; \
        } \
    }

// NOTE: rare random check that self-corrupts g_cheatDetectFlag with a 0/0 divide (kept for the byte match) or
// sets a kill switch, meant to punish tampering with the cheat-detect counter.
#define ANTI_CHEAT \
    if (g_cheatDetectFlag == CHEAT_DETECT_TRIP && RandRange(0, g_cheatDetectFlag) == 1 && RandRange(0, 100) < 25) { \
        if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) \
            MarkCheatDetected(g_profileIndex); \
        if (RandRange(0, 100) < 50) \
            g_cheatDetectFlag = g_cheatDetectFlag / (g_cheatDetectFlag - g_cheatDetectFlag); \
        else \
            g_cheatKillSwitch = 1; \
    }

// Advances EN's 6-frame animation using the "alt" rotation-frame table (g_rot16Alt*), ping-ponging 0..5.
#define ANIM_B \
    EN.srcX = g_rot16AltSrcX[(int)EN.animFrame]; \
    EN.srcY = g_rot16AltSrcY[(int)EN.animFrame]; \
    EN.animTimer -= 1.0f * g_frameDt; \
    if (EN.animTimer < 0.0) { \
        EN.animTimer = EN.animDelay; \
        if (EN.animReverse) { \
            EN.animFrame -= 1.0; \
            if (EN.animFrame < 0.0) \
                EN.animFrame = 5.0f; \
        } else { \
            EN.animFrame += 1.0; \
            if (!(EN.animFrame < 6.0)) \
                EN.animFrame = 0; \
        } \
    }

// Periodic direction-change decision: when EN's turn timer runs out, pick a new turnState (bank left/right/
// straight) based on which screen edge or band (x/y) it's approaching, so it steers back toward open space.
#define F090_TURN \
    EN.turnTimer -= 1.0f * g_frameDt; \
    if (EN.turnTimer < 0.0) { \
        EN.turnState = 2; \
        EN.turnTimer = 30.0f; \
        if (EN.x > g_clipRight - 100 && EN.facing < 20) { \
            if (EN.facing > 9 && EN.facing < 30) { \
                EN.turnState = 3; \
                EN.turnTimer = RandFloat(0, 160.0f) + 40.0f; \
            } else { \
                EN.turnState = 1; \
                EN.turnTimer = RandFloat(0, 160.0f) + 40.0f; \
            } \
        } \
        if (EN.x < g_clipLeft + 36 && EN.facing > 19) { \
            if (EN.facing > 9 && EN.facing < 30) { \
                EN.turnState = 1; \
                EN.turnTimer = RandFloat(0, 160.0f) + 40.0f; \
            } else { \
                EN.turnState = 3; \
                EN.turnTimer = RandFloat(0, 160.0f) + 40.0f; \
            } \
        } \
        if (EN.y > 180.0 && EN.facing > 9 && EN.facing < 30) { \
            if (EN.facing < 20) { \
                EN.turnState = 1; \
                EN.turnTimer = RandFloat(0, 160.0f) + 40.0f; \
            } else { \
                EN.turnState = 3; \
                EN.turnTimer = RandFloat(0, 160.0f) + 40.0f; \
            } \
        } \
        if (EN.y < 80.0 && (EN.facing <= 9 || EN.facing >= 30)) { \
            if (EN.facing < 20) { \
                EN.turnState = 3; \
                EN.turnTimer = RandFloat(0, 160.0f) + 40.0f; \
            } else { \
                EN.turnState = 1; \
                EN.turnTimer = RandFloat(0, 160.0f) + 40.0f; \
            } \
        } \
    }

// Ends a paired alt-fire attack run: restores EN's normal fire delay/hp and clears the pairing.
#define EN_RESTORE \
    if (EN.altFireActive) { \
        EN.fireDelay = g_enemyFireRateMin > EN.savedFireDelay ? g_enemyFireRateMin : EN.savedFireDelay; \
        EN.hp = EN.savedHp; \
    } \
    EN.altFireActive = 0; \
    EN.pairedEnemyIdx = -1;

// Per-frame update for every active enemy slot of the current player: movement, firing, animation and
// type transitions. One giant switch on EN.type (enemyIdx); each `case` below is a distinct enemy behaviour.
// Most locals are scratch values reused across unrelated cases (direction lookups, pattern-group search
// indices, aim/spawn positions); a name reflects the meaning at its main use, not every reuse.
void UpdateEnemies()
{
    int enemyIdx;
    int objIdx;
    int dir;
    int dirScan;
    int partnerIdx;
    int enCenterX;
    int enCenterY;
    int partnerCenterX;
    int partnerCenterY;

    int shotSpreadCount;
    int itemIdx;
    int dropRoll;
    int itemGfxIdx;
    int itemType;
    int dropRollMax;

    int staggerIdx = 0;
    int unusedTemp = 0;   // NOTE: initialized but never read anywhere in the function
    int spawnedFlag = 0;
    int xSettled;
    int ySettled;

    float aimVx;
    float aimVy;
    float startX;
    float startY;
    float targetX;
    float targetY;
    float timeToTarget;
    float slope;
    float aimDirX;
    float aimDirY;
    float shotSpreadVx;
    float aimSpeed;
    float minStaggerTimer;
    int aimPlayer;

    float animDivisor;
    float animDivisor2;
    int triggerIdx;
    int triggerIdx2;
    int targetPlayer;
    int moneyStolen;
    float burstY;
    float beamStepUnused;
    int gunIdx;
    int aimPlayer2;

    int trapBossAnimStep;
    int trapFrameCountHigh;
    int trapFrameCountLow;
    int trapAnimFrameHigh;
    int trapAnimFrameLow;

    int homingGroupIdx;
    int groupSearchIdx;
    int burstGroupIdx;
    int subShotsLeft;
    int groupSearchIdx2;
    float aimDist;
    int homingGroupIdx2;
    int groupSearchIdx3;

    // ---- per-frame bookkeeping (idle timer, sway, fade queue) ----
    if (g_gameMode == MODE_DUAL)
        g_curPlayer = 0;
    if (!g_enemyActedThisFrame) {
        if (g_timerA == 0 && g_state == STATE_PLAYING && PL.freezeTimer == 0)
            g_levelIdleCounter++;
        else
            g_levelIdleCounter = 0;
    } else {
        g_levelIdleCounter = 0;
    }
    g_enemyActedThisFrame = 0;

    PL.enemySwayX = PL.enemySwayX + PL.enemySwayVelX * g_frameDt;
    PL.enemySwayVelX = PL.enemySwayVelX + PL.enemySwayAccel * g_frameDt;
    if (PL.enemySwayX > PL.enemySwayMax) {
        PL.enemySwayX = PL.enemySwayMax;
        PL.enemySwayVelX = 0 - PL.enemySwayVelX;
    }
    if (PL.enemySwayX < VF(PL.enemySwayMin)) {
        PL.enemySwayX = PL.enemySwayMin;
        PL.enemySwayVelX = 0 - PL.enemySwayVelX;
    }
    g_fadeQueueSample2 = 0;
    g_fadeQueueSample1 = 0;
    g_fadeQueueSample3 = 0;
    g_fadeQueueSample4 = 0;
    g_enemyFrameCounter++;
    g_bossIdx = -1;

    // ---- per-enemy update ----
    for (enemyIdx = 0; enemyIdx < MAX_ENEMIES; enemyIdx++) {
        if (EN.active == 1) {
            if (g_curLevelData.type != LEVEL_BONUS_WAVE)
                g_alienAttackTimer = g_alienAttackTimer + g_alienAttackTimerStep;
            if (EN.hitFlashTimer > 0)
                EN.hitFlashTimer--;
            if (PL.hyperspaceFade > 0.0) {
                if (EN.type != ENEMY_CAPTURED) {
                    EN.y += PL.scrollSpeedY * g_frameDt;
                    EN.y += PL.scrollSpeedY * g_frameDt;
                    if (EN.y > 700.0) {
                        EN.active = 0;
                        EN.type = ENEMY_PATTERNED;
                    }

                } else if (!g_save.players[EN.ownerPlayer].alienLock
                           && g_save.players[EN.ownerPlayer].dead == 0
                           && g_save.players[EN.ownerPlayer].lives
                                  > g_shipDefs[g_save.players[EN.ownerPlayer].ship]->minEnergy) {
                    EN.y += PL.scrollSpeedY * g_frameDt;
                    EN.y += PL.scrollSpeedY * g_frameDt;
                    if (EN.y > 700.0) {
                        EN.active = 0;
                        EN.type = ENEMY_PATTERNED;
                    }
                }
            }

            switch (EN.type) {

            // Type 5: falling debris chunk (from a destroyed hazard/mothership). Drifts by its stored
            // velocity, spawns a trailing spark particle each frame, and despawns once off the top.
            case ENEMY_DEBRIS:
                if (g_timerA)
                    break;
                EN.x += EN.debrisVelX * g_frameDt;
                EN.y += EN.debrisVelY * g_frameDt;
                if (EN.y < -30.0)
                    EN.active = 0;

                AddParticle(g_gfxSparkA, (int)EN.x + 16, (int)EN.y + 16, 64.0f, 0 - RandFloat(0.2f, 3.0f),
                                   RandFloat(0, 359.0f), RandFloat(0, 59.0f), RandRange(0, 359),
                                   255, 0, Randff(), 200, RandFloat(3.2f, 10.5f), 2.0f, 10,
                                   RandFloat(0.2f, 1.5f), 0, 0, 0, 1);
                g_enemyActedThisFrame = 1;

                EN.srcX = g_rot16SrcX[(int)EN.animFrame];
                EN.srcY = g_rot16SrcY[(int)EN.animFrame];
                EN.animTimer -= 1.0f * g_frameDt;
                if (EN.animTimer < 0.0) {
                    EN.animTimer = EN.animDelay;
                    if (EN.animReverse) {
                        EN.animFrame -= 1.0;
                        if (EN.animFrame < 0.0)
                            EN.animFrame = 15.0f;
                    } else {
                        EN.animFrame += 1.0;
                        if (!(EN.animFrame < 16.0))
                            EN.animFrame = 0;
                    }
                }
                break;

            // Type 8: player-attached "beam" hazard (e.g. tractor beam). Slides into position beside its
            // owner player (beamOffsetX/settled) then tracks the player's y; animates while unsettled or
            // once settled, depending on beamLevel.
            case ENEMY_CAPTURED:
                if (EN.beamSide == 0 && EN.settled == 0) {
                    xSettled = 1;
                    if (EN.beamOffsetX > -32) {
                        EN.beamOffsetX -= 1;
                        xSettled = 0;
                    } else if (EN.beamOffsetX < -32) {
                        EN.beamOffsetX += 1;
                        xSettled = 0;
                    }
                    ySettled = 1;
                    if ((int)EN.y < (int)g_save.players[EN.ownerPlayer].y - 1) {
                        EN.y += 1.0;
                        ySettled = 0;
                    }

                    if ((int)EN.y > (int)g_save.players[EN.ownerPlayer].y + 1) {
                        EN.y -= 1.0;
                        ySettled = 0;
                    }
                    if (xSettled && ySettled)
                        EN.settled = 1;
                }
                if (EN.beamSide == 1 && EN.settled == 0) {
                    xSettled = 1;
                    if (EN.beamOffsetX > 40) {
                        EN.beamOffsetX -= 1;
                        xSettled = 0;
                    } else if (EN.beamOffsetX < 40) {
                        EN.beamOffsetX += 1;
                        xSettled = 0;
                    }
                    ySettled = 1;
                    if ((int)EN.y < (int)g_save.players[EN.ownerPlayer].y - 1) {
                        EN.y += 1.0;

                        ySettled = 0;
                    }
                    if ((int)EN.y > (int)g_save.players[EN.ownerPlayer].y + 1) {
                        EN.y -= 1.0;
                        ySettled = 0;
                    }
                    if (xSettled && ySettled)
                        EN.settled = 1;
                }
                EN.x = g_save.players[EN.ownerPlayer].x + EN.beamOffsetX;
                if (EN.settled == 0 && g_beamLevel == 0) {
                    EN.srcX = g_rot16SrcX[(int)EN.animFrame];
                    EN.srcY = g_rot16SrcY[(int)EN.animFrame];
                    EN.animTimer -= 1.0f * g_frameDt;
                    if (EN.animTimer < 0.0) {
                        EN.animTimer = EN.animDelay;
                        if (EN.animReverse) {
                            EN.animFrame -= 1.0;

                            if (EN.animFrame < 0.0)
                                EN.animFrame = 15.0f;
                        } else {
                            EN.animFrame += 1.0;
                            if (!(EN.animFrame < 16.0))
                                EN.animFrame = 0;
                        }
                    }
                } else {
                    if (EN.animFrame > 5.0) {
                        EN.srcX = g_rot16SrcX[(int)EN.animFrame];
                        EN.srcY = g_rot16SrcY[(int)EN.animFrame];
                    } else {
                        EN.srcX = g_rot16AltSrcX[(int)EN.animFrame];
                        EN.srcY = g_rot16AltSrcY[(int)EN.animFrame];
                    }
                    EN.animTimer -= 1.0f * g_frameDt;
                    if (EN.animTimer < 0.0) {
                        EN.animTimer = EN.animDelay;

                        if (EN.animReverse == 0) {
                            EN.animFrame -= 1.0;
                            if (EN.animFrame < 0.0) {
                                if (EN.animPingPong == 0) {
                                    EN.animFrame = 5.0f;
                                } else {
                                    EN.animFrame = 1.0f;
                                    if (EN.animReverse == 0)
                                        EN.animReverse = 1;
                                    else
                                        EN.animReverse = 0;
                                }
                            }
                        } else {
                            EN.animFrame += 1.0;
                            if (!(EN.animFrame < 6.0)) {
                                if (EN.animPingPong == 0) {
                                    EN.animFrame = 0;
                                } else {

                                    EN.animFrame = 5.0f;
                                    if (EN.animReverse == 0)
                                        EN.animReverse = 1;
                                    else
                                        EN.animReverse = 0;
                                }
                            }
                        }
                    }
                }
                break;
            // Type 1: standard patterned alien. Fires straight down on a random chance, staggers group
            // attack timing across all type-1 enemies, then follows its level-pattern path (accel/velocity
            // steps from g_curLevelData), picking a facing direction from its velocity vector. Reaching the
            // end of its pattern can turn it into a hover/dive enemy (type 2/10) or mark it escaped.
            case ENEMY_PATTERNED:
                if (PL.freezeTimer)
                    break;

                if (g_timerA)
                    break;
                g_enemyActedThisFrame = 1;
                if (EN.y > -10.0 && RandFloat(0, EN.fireDelay) < 2.0 * g_frameDt
                    && g_curLevelData.type != LEVEL_RACE) {
                    for (objIdx = 0; objIdx < MAX_LEVEL_OBJS; objIdx++) {
                        if (g_levelObj[objIdx].active == 0) {
                            g_levelObj[objIdx].turnDelay = g_defaultObjAlpha;
                            g_levelObj[objIdx].active = 1;
                            g_levelObj[objIdx].x = EN.x + 13.0f;
                            g_levelObj[objIdx].y = EN.y + 16.0f;
                            g_levelObj[objIdx].hitOffsetX = EN.frameSet.aLeft;
                            g_levelObj[objIdx].hitOffsetY = EN.frameSet.aTop;
                            g_levelObj[objIdx].w = EN.frameSet.aWidth;
                            g_levelObj[objIdx].h = EN.frameSet.aHeight;
                            if (g_gameMode == MODE_TIME_TRIAL || g_enemyAimAtPlayer) {
                                if (PL.x < EN.x)
                                    g_levelObj[objIdx].vx = RandFloat(-1.5f, 0) * g_frameDt;
                                else
                                    g_levelObj[objIdx].vx = RandFloat(0, 1.5f) * g_frameDt;
                            } else {

                                g_levelObj[objIdx].vx = 0;
                            }
                            if (g_fastEnemyBullets)
                                g_levelObj[objIdx].vy = g_enemyBulletSpeed * 1.25f * g_frameDt;
                            else
                                g_levelObj[objIdx].vy = g_enemyBulletSpeed * g_frameDt;
                            g_levelObj[objIdx].type = LOBJ_SHOT;
                            g_levelObj[objIdx].gfxA = (KGraphic *)EN.gfxA;
                            g_levelObj[objIdx].gfxB = (KGraphic *)EN.gfxB;
                            g_levelObj[objIdx].hma = (int)EN.shotFrame;
                            g_levelObj[objIdx].hmaW = EN.shotGfxW;
                            g_levelObj[objIdx].hmaH = EN.shotGfxH;
                            if (g_levelObj[objIdx].x > 0.0 && g_levelObj[objIdx].x < g_screenW
                                && g_levelObj[objIdx].y > 0.0 && g_levelObj[objIdx].y < g_screenH) {
                                if ((int)EN.gfxA == (int)g_alienGfxCache[0].gfx1)
                                    SoundPlay(g_sndAlienShoot10, -1,
                                              g_rampB[ClampY((int)g_levelObj[objIdx].y)],
                                              g_panTable[ClampY((int)g_levelObj[objIdx].x)], SFX_PAN_CENTER, g_sndFlags);
                                if ((int)EN.gfxA == (int)g_alienGfxCache[1].gfx1)
                                    SoundPlay(g_sndAlienShoot12, -1,
                                              g_rampB[ClampY((int)g_levelObj[objIdx].y)],
                                              g_panTable[ClampY((int)g_levelObj[objIdx].x)], SFX_PAN_CENTER, g_sndFlags);

                                if ((int)EN.gfxA == (int)g_alienGfxCache[2].gfx1)
                                    SoundPlay(g_sndAlienShoot5, -1,
                                              g_rampB[ClampY((int)g_levelObj[objIdx].y)],
                                              g_panTable[ClampY((int)g_levelObj[objIdx].x)], SFX_PAN_CENTER, g_sndFlags);
                            }
                            break;
                        }
                    }
                }
                if (g_alienAttackTimer > 100) {
                    minStaggerTimer = 10000.0f;
                    for (staggerIdx = 0; staggerIdx < MAX_ENEMIES; staggerIdx++) {
                        if (g_enemies[g_curPlayer][staggerIdx].active == 1
                            && g_enemies[g_curPlayer][staggerIdx].type == 1
                            && g_enemies[g_curPlayer][staggerIdx].attackStaggerTimer < minStaggerTimer)
                            minStaggerTimer = g_enemies[g_curPlayer][staggerIdx].attackStaggerTimer;
                    }
                    if (minStaggerTimer < 0.0)
                        minStaggerTimer = 0;

                    for (staggerIdx = 0; staggerIdx < MAX_ENEMIES; staggerIdx++) {
                        if (g_enemies[g_curPlayer][staggerIdx].active == 1
                            && g_enemies[g_curPlayer][staggerIdx].type == 1)
                            g_enemies[g_curPlayer][staggerIdx].attackStaggerTimer -= minStaggerTimer;
                    }
                    g_alienAttackTimer = 0;
                }
                if (EN.attackStaggerTimer > 0.0) {
                    EN.attackStaggerTimer -= 1.0f * g_frameDt;
                    if ((int)EN.attackStaggerTimer <= 0) {
                        EN.attackStaggerTimer = 0;
                        g_alienAttackTimer = -150;
                        g_alienAttackTimerStep = 1;
                        if (g_curLevelData.type == LEVEL_BONUS_WAVE)
                            SoundPlay(g_sfxAlienAttack2, RandRange(30000, 40000), 220,
                                             g_panTable[ClampX((int)EN.x)], SFX_PAN_CENTER, g_sndFlags);
                    }
                } else {

                EN.oscillatePhase += EN.oscillateAccel * g_frameDt;
                if (g_curLevelData.type == LEVEL_BONUS_WAVE)
                    EN.x += (EN.velX + EN.oscillatePhase * EN.oscillateMul) * g_frameDt;
                else
                    EN.x += EN.velX * g_frameDt;
                EN.y += EN.velY * g_frameDt;
                EN.velX += EN.accelX * g_frameDt;
                EN.velY += EN.accelY * g_frameDt;
                EN.patternTimer += 1.0f * g_frameDt;
                g_enemyVelXScratch = EN.velX;
                g_enemyVelYScratch = EN.velY;
                if (EN.forcedDir == -1) {
                    if (g_enemyVelXScratch != 0.0)
                        slope = g_enemyVelYScratch / g_enemyVelXScratch;
                    else if (g_enemyVelYScratch > 0.0)
                        slope = 5000.0f;
                    else
                        slope = -5000.0f;

                    dir = 0;
                    if (g_enemyVelXScratch > 0.0 || g_enemyVelXScratch == 0.0) {
                        for (dirScan = 0; dirScan < 9; dirScan++) {
                            if (slope > g_dirSlopeRange[dirScan].lo && slope < g_dirSlopeRange[dirScan].hi) {
                                dir = dirScan;
                                break;
                            }
                        }
                    } else {
                        dir = 8;
                        for (dirScan = 0; dirScan < 9; dirScan++) {
                            if (slope > g_dirSlopeRange[dirScan].lo && slope < g_dirSlopeRange[dirScan].hi) {
                                dir = dir + dirScan;
                                break;
                            }
                        }
                    }
                    if (dir == 16)
                        dir = 0;

                    EN.forcedDir = -1;
                } else {
                    dir = EN.forcedDir;
                    EN.forcedDir = -1;
                }
                if (EN.useDirRemap) {
                    EN.srcX = g_rot16SrcX[g_dirRemap[dir]];
                    EN.srcY = g_rot16SrcY[g_dirRemap[dir]];
                } else {
                    EN.srcX = g_rot16SrcX[dir];
                    EN.srcY = g_rot16SrcY[dir];
                }
                if (EN.patternTimer > g_curLevelData.obj[EN.groupIndex][EN.patternStep].holdTime) {
                    EN.patternStep++;
                    if (!g_mirrorLevel)
                        EN.accelX = g_curLevelData.obj[EN.groupIndex][EN.patternStep].pathX;
                    else
                        EN.accelX = 0 - g_curLevelData.obj[EN.groupIndex][EN.patternStep].pathX;

                    EN.accelY = g_curLevelData.obj[EN.groupIndex][EN.patternStep].pathY;
                    EN.patternTimer = 1.0f * g_frameDt;
                    if (g_curLevelData.obj[EN.groupIndex][EN.patternStep].cmd == OPCMD_FLASH) {
                        EN.flashActive = 1;
                        EN.flashTimer = 15.0f;
                    }
                    if (g_curLevelData.obj[EN.groupIndex][EN.patternStep].cmd == OPCMD_STOP_TURN
                        && g_curLevelData.obj[EN.groupIndex][EN.patternStep].holdTime != PATTERN_HOLD_END) {
                        EN.forcedDir = dir;
                        EN.velX = 0;
                        EN.velY = 0;
                        EN.accelX = 0;
                        EN.accelY = 0;
                        EN.patternTimer = 1.0f;
                    }
                    if (g_curLevelData.obj[EN.groupIndex][EN.patternStep].cmd == OPCMD_STOP_TURN
                        && g_curLevelData.obj[EN.groupIndex][EN.patternStep].holdTime == PATTERN_HOLD_END) {
                        g_groupEnemyCount[EN.groupIndex] = 0;
                        switch (g_curLevelData.type) {
                        case LEVEL_WAVE:
                            EN.type = ENEMY_HOVER;

                            break;
                        case LEVEL_WAVE_AIMED:
                            EN.type = ENEMY_HOVER;
                            break;
                        case LEVEL_BONUS_WAVE:
                            EN.type = ENEMY_ESCAPER;
                            EN.descendVelY = 0;
                            EN.descendAccelY = RandFloat(0.2f, 0.4f);
                            EN.descendVelX = 0;
                            EN.descendAccelX = RandFloat(-0.1f, 0.1f);
                            EN.turnTimer2Max = RandRange(0, 5) + 10;
                            EN.turnTimer2 = EN.turnTimer2Max;
                            break;
                        case LEVEL_RACE:
                            EN.active = 0;
                            CreditEscape(g_curPlayer);
                            break;
                        }

                        break;
                    }
                    if (g_curLevelData.obj[EN.groupIndex][EN.patternStep].cmd == OPCMD_ESCAPE) {
                        if (g_curLevelData.type == LEVEL_BONUS_WAVE) {
                            EN.type = ENEMY_ESCAPER;
                            EN.descendVelY = 0;
                            EN.descendAccelY = RandFloat(0.2f, 0.4f);
                            EN.descendVelX = 0;
                            EN.descendAccelX = RandFloat(-0.1f, 0.1f);
                            EN.turnTimer2Max = RandRange(0, 5) + 10;
                            EN.turnTimer2 = EN.turnTimer2Max;
                        } else {
                            EN.active = 0;
                            CreditEscape(g_curPlayer);
                            EN.patternTimer = 1.0f;
                        }
                    }
                }

                if (g_curLevelData.type == LEVEL_RACE)
                    g_groupEnemyCount[EN.groupIndex] = 0;
                }
                break;
            // Type 2: hovering alien holding a formation slot (hoverX/hoverY). Eases toward its hover
            // point, occasionally launches into a dive attack (type 3) using a random pattern, sometimes
            // pairing up with a nearby type-2 enemy for a synchronized alt-fire attack, and can be recycled
            // into type 4 (looping flyby) once the player has enough enemies left to kill.
            case ENEMY_HOVER:
                g_alienAttackTimer = 0;
                g_alienAttackTimerStep = 1;
                if (PL.freezeTimer)
                    break;
                if (g_timerA)
                    break;
                g_enemyActedThisFrame = 1;
                if (EN.x != EN.hoverX) {
                    aimVx = (EN.x - EN.hoverX) / 20.0;

                    EN.x -= aimVx * g_frameDt;
                }
                ANTI_CHEAT
                aimVx = 0;
                if (EN.x > g_screenW + 150 || EN.x < -150.0) {
                    EN.x = RandFloat(0, g_screenW);
                    EN.y = 0 - RandFloat(50.0f, 100.0f);
                    EN.type = ENEMY_FLYBY;
                    if ((int)EN.gfxA == (int)g_alienGfxCache[0].gfx1)
                        EN.velY = RandFloat(1.0f, 3.0f);
                    else
                        EN.velY = RandFloat(2.0f, 5.0f);

                    EN.offsetY = 0;
                    EN_RESTORE
                    break;
                }
                if (EN.y != EN.hoverY) {
                    aimVy = (EN.y - EN.hoverY) / 20.0;
                    EN.y -= aimVy * g_frameDt;
                }
                aimVy = 0;
                if (aimVx != 0.0 && aimVy != 0.0) {
                    g_enemyVelXScratch = aimVx;
                    g_enemyVelYScratch = aimVy;
                    if (g_enemyVelXScratch != 0.0)
                        slope = g_enemyVelYScratch / g_enemyVelXScratch;
                    else if (g_enemyVelYScratch > 0.0)
                        slope = 5000.0f;
                    else
                        slope = -5000.0f;

                    dir = 0;
                    if (g_enemyVelXScratch > 0.0 || g_enemyVelXScratch == 0.0) {
                        for (dirScan = 0; dirScan < 9; dirScan++) {
                            if (slope > g_dirSlopeRange[dirScan].lo && slope < g_dirSlopeRange[dirScan].hi) {
                                dir = dirScan;
                                break;
                            }
                        }
                    } else {
                        dir = 8;
                        for (dirScan = 0; dirScan < 9; dirScan++) {
                            if (slope > g_dirSlopeRange[dirScan].lo && slope < g_dirSlopeRange[dirScan].hi) {
                                dir = dir + dirScan;
                                break;
                            }
                        }
                    }
                    if (dir == 16)
                        dir = 0;

                    if (EN.useDirRemap) {
                        EN.srcX = g_rot16SrcX[g_dirRemap[dir]];
                        EN.srcY = g_rot16SrcY[g_dirRemap[dir]];
                    } else {
                        EN.srcX = g_rot16SrcX[dir];
                        EN.srcY = g_rot16SrcY[dir];
                    }
                } else {
                    EN.srcX = g_rot16AltSrcX[(int)EN.animFrame];
                    EN.srcY = g_rot16AltSrcY[(int)EN.animFrame];
                    EN.animTimer -= 1.0f * g_frameDt;
                    if (EN.animTimer < 0.0) {
                        EN.animTimer = EN.animDelay;
                        if (EN.animReverse == 0) {
                            EN.animFrame -= 1.0;
                            if (EN.animFrame < 0.0) {
                                if (EN.animPingPong == 0) {
                                    EN.animFrame = 5.0f;
                                } else {

                                    EN.animFrame = 1.0f;
                                    if (EN.animReverse == 0)
                                        EN.animReverse = 1;
                                    else
                                        EN.animReverse = 0;
                                }
                            }
                        } else {
                            EN.animFrame += 1.0;
                            if (!(EN.animFrame < 6.0)) {
                                if (EN.animPingPong == 0) {
                                    EN.animFrame = 0;
                                } else {
                                    EN.animFrame = 5.0f;
                                    if (EN.animReverse == 0)
                                        EN.animReverse = 1;
                                    else
                                        EN.animReverse = 0;
                                }
                            }
                        }
                    }
                }

                EN.offsetX = PL.enemySwayX - 16.0f;
                EN.offsetY = PL.enemySwayY;
                if (EN.attackDelay && RandRange(0, EN.attackDelay) == 1) {
                    EN.x += EN.offsetX;
                    EN.y += EN.offsetY;
                    EN.type = ENEMY_DIVING;
                    EN.patternId = RandRange(0, g_patternCount);
                    EN.velX = g_patterns[EN.patternId].unused5 / 256.0;
                    EN.velY = g_patterns[EN.patternId].unused6 / 256.0;
                    EN.accelX = g_patterns[EN.patternId].entries[0].x / 256.0f;
                    EN.accelY = g_patterns[EN.patternId].entries[0].y / 256.0f;
                    EN.patternTimer = 1.0f * g_frameDt;
                    EN.patternStep = 0;
                    EN.patternEndAction = g_patterns[EN.patternId].unused8;
                    if (RandRange(0, 100) < g_diffEnemyFireChance && EN.altFireActive == 0
                        && EN.pairedEnemyIdx < 0) {
                        enCenterX = (int)EN.x + 16;
                        enCenterY = (int)EN.y + 16;
                        for (partnerIdx = 0; partnerIdx < MAX_ENEMIES; partnerIdx++) {
                            if (E2.active != 0 && partnerIdx != enemyIdx && E2.altFireActive == 0
                                && E2.pairedEnemyIdx < 0 && E2.type == 2) {
                                partnerCenterX = (int)E2.x + 16;

                                partnerCenterY = (int)E2.y + 16;
                                if (partnerCenterY - enCenterY > 16 && partnerCenterY - enCenterY < 60
                                    && partnerCenterX - enCenterX > -60 && partnerCenterX - enCenterX < 60) {
                                    g_altFireToggle = 1;
                                    E2.x += E2.offsetX;
                                    E2.y += E2.offsetY;
                                    E2.pairedEnemyIdx = enemyIdx;
                                    E2.type = 3;
                                    E2.patternId = EN.patternId;
                                    E2.velX = EN.velX;
                                    E2.velY = EN.velY;
                                    E2.accelX = EN.accelX;
                                    E2.accelY = EN.accelY;
                                    E2.patternTimer = 1.0f * g_frameDt;
                                    E2.patternStep = 0;
                                    E2.patternEndAction = EN.patternEndAction;
                                }
                            }
                        }

                        if (g_altFireToggle) {
                            EN.altFireActive = 1;
                            EN.savedFireDelay = EN.fireDelay;
                            EN.fireDelay = g_enemyFireRateMin > EN.fireDelay / 2
                                               ? g_enemyFireRateMin : EN.fireDelay / 2;
                            EN.savedHp = EN.hp;
                            EN.hp = EN.hp * 2.0f;
                        }
                    }
                    if (RandRange(0, 10) < g_diffEnemyFireChance + 1 && EN.altFireActive == 0
                        && EN.pairedEnemyIdx < 0 && (int)EN.gfxA != (int)g_alienGfxCache[0].gfx1) {
                        enCenterX = (int)EN.x + 16;
                        enCenterY = (int)EN.y + 16;
                        g_altFireToggle = 0;
                        for (partnerIdx = 0; partnerIdx < MAX_ENEMIES; partnerIdx++) {
                            if (E2.active != 0 && partnerIdx != enemyIdx && E2.altFireActive == 0
                                && E2.pairedEnemyIdx < 0 && E2.type == 2) {
                                partnerCenterX = (int)E2.x + 16;
                                partnerCenterY = (int)E2.y + 16;
                                if (partnerCenterY - enCenterY > 16 && partnerCenterY - enCenterY < 60
                                    && partnerCenterX - enCenterX > -60 && partnerCenterX - enCenterX < 60) {
                                    g_altFireToggle = 1;

                                    E2.x += E2.offsetX;
                                    E2.y += E2.offsetY;
                                    E2.pairedEnemyIdx = enemyIdx;
                                    E2.type = 3;
                                    E2.patternId = EN.patternId;
                                    E2.velX = EN.velX;
                                    E2.velY = EN.velY;
                                    E2.accelX = EN.accelX;
                                    E2.accelY = EN.accelY;
                                    E2.patternTimer = 1.0f * g_frameDt;
                                    E2.patternStep = 0;
                                    E2.patternEndAction = EN.patternEndAction;
                                }
                            }
                        }
                        if (g_altFireToggle) {
                            EN.altFireActive = 1;
                            EN.savedFireDelay = EN.fireDelay;

                            EN.fireDelay = g_enemyFireRateMin > EN.fireDelay / 2
                                               ? g_enemyFireRateMin : EN.fireDelay / 2;
                            EN.savedHp = EN.hp;
                            EN.hp = EN.hp * 2.0f;
                        }
                    }
                    if ((int)EN.gfxA == (int)g_alienGfxCache[0].gfx1)
                        SoundPlay(g_sfxAlienAttack, RandRange(35000, 50000), g_rampB[ClampY((int)EN.y)],
                                         g_panTable[ClampX((int)EN.x)], SFX_PAN_CENTER, g_sndFlags);
                    if ((int)EN.gfxA == (int)g_alienGfxCache[1].gfx1 && g_altFireToggle == 0)
                        SoundPlay(g_sfxAlienAttack3, RandRange(35000, 50000), g_rampB[ClampY((int)EN.y)],
                                         g_panTable[ClampX((int)EN.x)], SFX_PAN_CENTER, g_sndFlags);
                    if ((int)EN.gfxA == (int)g_alienGfxCache[1].gfx1 && g_altFireToggle != 0)
                        SoundPlay(g_sfxAlienAttack4, RandRange(35000, 50000), g_rampB[ClampY((int)EN.y)],
                                         g_panTable[ClampX((int)EN.x)], SFX_PAN_CENTER, g_sndFlags);
                    if ((int)EN.gfxA == (int)g_alienGfxCache[2].gfx1 && g_altFireToggle == 0)
                        SoundPlay(g_sfxAlienAttack5, RandRange(35000, 50000), g_rampB[ClampY((int)EN.y)],
                                         g_panTable[ClampX((int)EN.x)], SFX_PAN_CENTER, g_sndFlags);
                    if ((int)EN.gfxA == (int)g_alienGfxCache[2].gfx1 && g_altFireToggle != 0)
                        SoundPlay(g_sfxAlienAttack4, RandRange(35000, 50000), g_rampB[ClampY((int)EN.y)],
                                         g_panTable[ClampX((int)EN.x)], SFX_PAN_CENTER, g_sndFlags);
                }

                if (PL.totalEnemies - (PL.killed + PL.escaped) <= PL.spawnReserve) {
                    EN.x += EN.offsetX;
                    EN.y += EN.offsetY;
                    EN.type = ENEMY_FLYBY;
                    if ((int)EN.gfxA == (int)g_alienGfxCache[0].gfx1)
                        EN.velY = RandFloat(1.0f, 3.0f);
                    else
                        EN.velY = RandFloat(2.0f, 5.0f);
                    EN.offsetY = 0;
                    EN_RESTORE
                    break;
                }
                break;
            // Type 3: diving attack run (entered from type 2). Fires while diving (a 3-shot spread when
            // alt-fire is active), follows its attack pattern (FP), and at the end either returns to
            // hovering (type 2), falls further as a "falling" attack, or is recycled to type 4.
            case ENEMY_DIVING:
                g_alienAttackTimer = 0;

                g_alienAttackTimerStep = 1;
                if (PL.freezeTimer)
                    break;
                if (g_timerA)
                    break;
                g_enemyActedThisFrame = 1;
                ANTI_CHEAT
                if (RandFloat(0, NearPlayerFireBoost((int)(EN.x + 13.0), EN.fireDelay)) < 2.0 * g_frameDt
                    && EN.pairedEnemyIdx == -1) {
                    if (EN.altFireActive == 0) {
                        for (objIdx = 0; objIdx < MAX_LEVEL_OBJS; objIdx++) {
                            if (SH.active == 0) {
                                SH.turnDelay = g_defaultObjAlpha;

                                SH.active = 1;
                                SH.x = EN.x + 13.0f;
                                SH.y = EN.y + 16.0f;
                                if (g_gameMode == MODE_TIME_TRIAL || g_enemyAimAtPlayer) {
                                    if (PL.x < EN.x)
                                        SH.vx = RandFloat(-1.5f, 0) * g_frameDt;
                                    else
                                        SH.vx = RandFloat(0, 1.5f) * g_frameDt;
                                } else {
                                    SH.vx = 0;
                                }
                                if (g_fastEnemyBullets)
                                    SH.vy = g_enemyBulletSpeed * 1.25f * g_frameDt;
                                else
                                    SH.vy = g_enemyBulletSpeed * g_frameDt;
                                SH.type = LOBJ_SHOT;
                                SH.gfxA = (KGraphic *)EN.gfxA;
                                SH.gfxB = (KGraphic *)EN.gfxB;

                                SH.hma = (int)EN.shotFrame;
                                SH.hmaW = EN.shotGfxW;
                                SH.hmaH = EN.shotGfxH;
                                SH.hitOffsetX = EN.frameSet.aLeft;
                                SH.hitOffsetY = EN.frameSet.aTop;
                                SH.w = EN.frameSet.aWidth;
                                SH.h = EN.frameSet.aHeight;
                                break;
                            }
                        }
                        SoundPlay(g_sfxAlienShoot18, -1, g_rampB[ClampY((int)SH.y)],
                                         g_panTable[ClampY((int)SH.x)], SFX_PAN_CENTER, g_sndFlags);
                    } else {
                        shotSpreadVx = -0.3f;
                        shotSpreadCount = 0;
                        for (objIdx = 0; objIdx < MAX_LEVEL_OBJS; objIdx++) {
                            if (SH.active == 0) {
                                SH.turnDelay = g_defaultObjAlpha;

                                SH.active = 1;
                                SH.x = EN.x + 13.0f;
                                SH.y = EN.y + 16.0f;
                                SH.vx = shotSpreadVx * g_frameDt;
                                if (g_fastEnemyBullets)
                                    SH.vy = g_enemyBulletSpeed * 1.25f * g_frameDt;
                                else
                                    SH.vy = g_enemyBulletSpeed * g_frameDt;
                                SH.type = LOBJ_SHOT;
                                SH.gfxA = (KGraphic *)EN.gfxA;
                                SH.gfxB = (KGraphic *)EN.gfxB;
                                SH.hma = (int)EN.shotFrame;
                                SH.hmaW = EN.shotGfxW;
                                SH.hmaH = EN.shotGfxH;
                                SH.hitOffsetX = EN.frameSet.aLeft;
                                SH.hitOffsetY = EN.frameSet.aTop;
                                SH.w = EN.frameSet.aWidth;
                                SH.h = EN.frameSet.aHeight;

                                shotSpreadVx += 0.3f;
                                shotSpreadCount++;
                                if (shotSpreadCount > 2)
                                    break;
                            }
                        }
                        SHOT_SOUNDS
                    }
                }

                EN.x += EN.velX * g_frameDt;
                EN.y += EN.velY * g_frameDt;
                EN.velX += EN.accelX * g_frameDt;
                EN.velY += EN.accelY * g_frameDt;
                EN.patternTimer += 1.0f * g_frameDt;
                g_enemyVelXScratch = EN.velX;
                g_enemyVelYScratch = EN.velY;
                if (EN.forcedDir == -1) {
                    if (g_enemyVelXScratch != 0.0)
                        slope = g_enemyVelYScratch / g_enemyVelXScratch;
                    else if (g_enemyVelYScratch > 0.0)
                        slope = 5000.0f;
                    else
                        slope = -5000.0f;
                    dir = 0;
                    if (g_enemyVelXScratch >= 0.0) {
                        for (dirScan = 0; dirScan < 9; dirScan++) {
                            if (slope > g_dirSlopeRange[dirScan].lo && slope < g_dirSlopeRange[dirScan].hi) {
                                dir = dirScan;

                                break;
                            }
                        }
                    } else {
                        dir = 8;
                        for (dirScan = 0; dirScan < 9; dirScan++) {
                            if (slope > g_dirSlopeRange[dirScan].lo && slope < g_dirSlopeRange[dirScan].hi) {
                                dir = dir + dirScan;
                                break;
                            }
                        }
                    }
                    if (dir == 16)
                        dir = 0;
                    EN.forcedDir = -1;
                } else {
                    dir = EN.forcedDir;
                    EN.forcedDir = -1;
                }

                if (EN.useDirRemap) {
                    EN.srcX = g_rot16SrcX[g_dirRemap[dir]];
                    EN.srcY = g_rot16SrcY[g_dirRemap[dir]];
                } else {
                    EN.srcX = g_rot16SrcX[dir];
                    EN.srcY = g_rot16SrcY[dir];
                }
                if ((int)EN.patternTimer > FP.tParam) {
                    EN.patternStep++;
                    EN.accelX = FP.x / 256.0f;
                    EN.accelY = FP.y / 256.0f;
                    EN.patternTimer = 1.0f * g_frameDt;
                    if (FP.type == PATCMD_STOP_TURN) {
                        EN.forcedDir = dir;
                        EN.velX = 0;
                        EN.velY = 0;
                        EN.accelX = 0;
                        EN.accelY = 0;

                        EN.patternTimer = 1.0f;
                    }
                    if (FP.type == PATCMD_ESCAPE) {
                        EN.active = 0;
                        CreditEscape(g_curPlayer);
                        EN.patternTimer = 1.0f;
                    }
                    if (FP.tParam == 0) {
                        if (EN.patternEndAction == 2) {
                            if (PL.totalEnemies - (PL.killed + PL.escaped) <= PL.spawnReserve) {
                                EN.type = ENEMY_FLYBY;
                                if ((int)EN.gfxA == (int)g_alienGfxCache[0].gfx1)
                                    EN.velY = RandFloat(1.0f, 3.0f);
                                else
                                    EN.velY = RandFloat(2.0f, 5.0f);
                                EN.offsetY = 0;
                                EN_RESTORE
                                break;
                            }

                            EN.unusedF7c = 1.0f;
                            EN.type = ENEMY_HOVER;
                            EN.x -= EN.offsetX;
                            EN.y -= EN.offsetY;
                            EN_RESTORE
                            break;
                        }
                        if (EN.patternEndAction == 3) {
                            if (PL.totalEnemies - (PL.killed + PL.escaped) <= PL.spawnReserve) {
                                EN.type = ENEMY_FLYBY;
                                if ((int)EN.gfxA == (int)g_alienGfxCache[0].gfx1)
                                    EN.velY = RandFloat(1.0f, 3.0f);
                                else
                                    EN.velY = RandFloat(2.0f, 5.0f);
                                EN.offsetY = 0;
                                EN_RESTORE
                                break;
                            }

                            SoundPlay(g_sfxFalling, -1, 150, 0, SFX_PAN_CENTER, g_sndFlags);
                            EN.y = -150.0f;
                            EN.type = ENEMY_HOVER;
                            EN.x -= EN.offsetX;
                            EN.y -= EN.offsetY;
                            EN_RESTORE
                            break;
                        }
                        EN.x -= EN.offsetX;
                        EN.y -= EN.offsetY;
                        EN_RESTORE
                        EN.type = ENEMY_HOVER;
                        break;
                    }
                }
                break;
            // Type 4: looping flyby alien (recycled from type 2/3). Fires occasionally, zigzags
            // horizontally (bounded, randomized acceleration reversals) while descending, and wraps back
            // to the top once it scrolls off the bottom.
            case ENEMY_FLYBY:
                if (PL.freezeTimer)
                    break;

                if (g_timerA)
                    break;
                g_enemyActedThisFrame = 1;
                if (RandFloat(0, NearPlayerFireBoost((int)(EN.x + 13.0), EN.fireDelay)) < 3.0 * g_frameDt) {
                    for (objIdx = 0; objIdx < MAX_LEVEL_OBJS; objIdx++) {
                        if (SH.active == 0) {
                            SH.turnDelay = g_defaultObjAlpha;
                            SH.active = 1;
                            SH.x = EN.x + 13.0f;
                            SH.y = EN.y + 16.0f;
                            SH.type = LOBJ_SHOT;
                            if (g_gameMode == MODE_TIME_TRIAL || g_enemyAimAtPlayer) {
                                if (PL.x < EN.x)
                                    SH.vx = RandFloat(-1.5f, 0) * g_frameDt;
                                else
                                    SH.vx = RandFloat(0, 1.5f) * g_frameDt;
                            } else {
                                SH.vx = 0;
                            }

                            if (g_fastEnemyBullets)
                                SH.vy = g_enemyBulletSpeed * 1.25f * g_frameDt;
                            else
                                SH.vy = g_enemyBulletSpeed * g_frameDt;
                            SH.gfxA = (KGraphic *)EN.gfxA;
                            SH.gfxB = (KGraphic *)EN.gfxB;
                            SH.hma = (int)EN.shotFrame;
                            SH.hmaW = EN.shotGfxW;
                            SH.hmaH = EN.shotGfxH;
                            SH.hitOffsetX = EN.frameSet.aLeft;
                            SH.hitOffsetY = EN.frameSet.aTop;
                            SH.w = EN.frameSet.aWidth;
                            SH.h = EN.frameSet.aHeight;
                            SHOT_SOUNDS
                            break;
                        }
                    }
                }
                EN.x += EN.zigzagVelX * g_frameDt;
                ANTI_CHEAT
                if (EN.x > g_clipRight + 32)
                    EN.x = g_clipLeft - 32;

                if (EN.x < g_clipLeft - 32)
                    EN.x = g_clipRight + 32;
                if (EN.x + 32.0 > g_clipRight - 100 && EN.zigzagAccel > 0.0) {
                    EN.zigzagTimer = RandRange(0, 20) + 30;
                    EN.zigzagAccel = 0 - RandFloat(0.01f, 0.2f);
                }
                if (EN.x < g_clipLeft + 100 && EN.zigzagAccel < 0.0) {
                    EN.zigzagTimer = RandRange(0, 20) + 30;
                    EN.zigzagAccel = RandFloat(0.01f, 0.2f);
                }
                EN.zigzagVelX += EN.zigzagAccel * g_frameDt;
                if (EN.zigzagVelX > 4.0 && EN.zigzagAccel > 0.0) {
                    EN.zigzagTimer = RandRange(0, 20) + 30;
                    EN.zigzagAccel = 0 - RandFloat(0.01f, 0.2f);
                }
                if (EN.zigzagVelX < -4.0 && EN.zigzagAccel < 0.0) {
                    EN.zigzagTimer = RandRange(0, 20) + 30;
                    EN.zigzagAccel = RandFloat(0.01f, 0.2f);
                }

                if (!EN.zigzagTimer--) {
                    EN.zigzagTimer = RandRange(0, 20) + 30;
                    if (EN.zigzagAccel > 0.0)
                        EN.zigzagAccel = 0 - RandFloat(0.01f, 0.2f);
                    else
                        EN.zigzagAccel = RandFloat(0.01f, 0.2f);
                }
                EN.y += EN.velY * g_frameDt;
                if (EN.y > g_screenH + 15) {
                    if (PL.hyperspaceFade > 0.0) {
                        EN.active = 0;
                        CreditEscape(g_curPlayer);
                    } else {
                        EN.y = -50.0f;
                        SoundPlay(g_sfxFalling, -1, 150, 0, SFX_PAN_CENTER, g_sndFlags);
                    }
                }
                ANIM_B
                break;
            // Type 10: ascending "escaper" alien (recycled from a completed pattern in bonus-round levels).
            // Fires downward, oscillates its horizontal drift, and climbs off the top of the screen to
            // escape (with easing deceleration on its vertical speed).
            case ENEMY_ESCAPER:
                g_alienAttackTimer = 0;

                g_alienAttackTimerStep = 1;
                if (PL.freezeTimer)
                    break;
                if (g_timerA)
                    break;
                g_enemyActedThisFrame = 1;
                ANTI_CHEAT
                if (RandFloat(0, NearPlayerFireBoost((int)(EN.x + 13.0), EN.fireDelay)) < 3.0 * g_frameDt) {
                    SHOT_FIRE_A
                }
                EN.turnTimer2 -= 1.0f * g_frameDt;
                if (EN.turnTimer2 < 0.0) {
                    EN.turnTimer2 = EN.turnTimer2Max;
                    EN.descendAccelX = -EN.descendAccelX;
                }
                EN.x += EN.descendVelX * g_frameDt;
                EN.descendVelX += EN.descendAccelX * g_frameDt;
                EN.y -= EN.descendVelY * g_frameDt;

                if (EN.y + 32.0 < g_clipTop - 100) {
                    EN.active = 0;
                    CreditEscape(g_curPlayer);
                }
                EN.descendVelY += EN.descendAccelY * g_frameDt;
                EN.descendAccelY /= 1.0f + g_frameDt * 0.05f;
                ANIM_B
                break;
            // Type 6: free-roaming alien that wraps around all four screen edges, aims a slow homing-style
            // shot at a (possibly randomly chosen, in 2-player mode) player, and turns/faces in 40 discrete
            // directions based on a periodic turn timer.
            case ENEMY_WRAPPER:
                if (PL.freezeTimer)
                    break;
                if (g_timerA)
                    break;
                g_enemyActedThisFrame = 1;
                EN.x += EN.speedX * g_dirVecX2[EN.facing] * EN.speedScale * g_frameDt;

                EN.y += EN.speedY * g_dirVecY2[EN.facing] * EN.speedScale * g_frameDt;
                if (EN.x > g_screenW + 120)
                    EN.x = -120.0f;
                if (EN.x < -120.0)
                    EN.x = g_screenW + 110.0f;
                if (EN.y > g_screenH + 120)
                    EN.y = -120.0f;
                if (EN.y < -120.0)
                    EN.y = g_screenH + 110.0f;
                ANTI_CHEAT
                if (RandFloat(0, EN.fireDelay) < 2.0 * g_frameDt) {
                    if (g_state == STATE_MALFUNCTION)
                        timeToTarget = RandFloat(30.0f, 60.0f) * g_diffEnemyTimerMul;
                    else
                        timeToTarget = RandFloat(45.0f, 55.0f) * g_diffEnemyTimerMul;
                    startX = EN.x;
                    startY = EN.y;
                    if (g_gameMode == MODE_DUAL) {
                        aimPlayer = RandRange(0, 2);

                        if (g_save.players[aimPlayer].lives
                            < g_shipDefs[g_save.players[aimPlayer].ship]->minEnergy
                                  + g_shipDefs[g_save.players[aimPlayer].ship]->cost) {
                            if (aimPlayer == 0)
                                aimPlayer = 1;
                            else
                                aimPlayer = 0;
                        }
                        targetX = g_save.players[aimPlayer].x - RandFloat(-40.0f, 40.0f);
                        targetY = g_save.players[aimPlayer].y - RandFloat(-40.0f, 40.0f);
                    } else {
                        targetX = PL.x - RandFloat(-40.0f, 40.0f);
                        targetY = PL.y - RandFloat(-40.0f, 40.0f);
                    }
                    if (timeToTarget == 0.0)
                        timeToTarget = 1.0f;
                    aimVx = (targetX - startX) / timeToTarget;
                    aimVy = (targetY - startY) / timeToTarget;
                    for (objIdx = 0; objIdx < MAX_LEVEL_OBJS; objIdx++) {
                        if (SH.active == 0) {
                            SH.turnDelay = g_defaultObjAlpha;

                            SH.active = 1;
                            SH.x = EN.x + 32.0f;
                            SH.y = EN.y + 25.0f;
                            SH.vx = aimVx * g_frameDt;
                            SH.vy = aimVy * g_frameDt;
                            SH.type = LOBJ_AIMED_SHOT;
                            SH.gfxA = (KGraphic *)EN.gfxA;
                            SH.gfxB = (KGraphic *)EN.gfxB;
                            SH.hma = (int)EN.shotFrame;
                            SH.hmaW = EN.shotGfxW;
                            SH.hmaH = EN.shotGfxH;
                            SH.hitOffsetX = EN.frameSet.bLeft;
                            SH.hitOffsetY = EN.frameSet.bTop;
                            SH.w = EN.frameSet.bWidth;
                            SH.h = EN.frameSet.bHeight;
                            g_fadeQueueSample3 = g_sfxAlienShoot2;
                            g_sfxPanIdx = g_rampB[ClampY((int)SH.x)];
                            g_sfxVolume = g_rampB[ClampY((int)SH.y)];

                            g_sfxFreq = RandRange(24000, 30000);
                            break;
                        }
                    }
                }
                EN.turnTimer -= 1.0f * g_frameDt;
                if (EN.turnTimer < 0.0) {
                    EN.turnState = 2;
                    EN.turnTimer = 30.0f;
                    if (EN.x > g_clipRight - 100 && EN.facing < 20) {
                        if (EN.facing > 9 && EN.facing < 30) {
                            EN.turnState = 3;
                            EN.turnTimer = RandFloat(0, 160.0f) + 40.0f;
                        } else {
                            EN.turnState = 1;
                            EN.turnTimer = RandFloat(0, 160.0f) + 40.0f;
                        }
                    }

                    if (EN.x < g_clipLeft + 36 && EN.facing > 19) {
                        if (EN.facing > 9 && EN.facing < 30) {
                            EN.turnState = 1;
                            EN.turnTimer = RandFloat(0, 160.0f) + 40.0f;
                        } else {
                            EN.turnState = 3;
                            EN.turnTimer = RandFloat(0, 160.0f) + 40.0f;
                        }
                    }
                    if (EN.y > 180.0 && EN.facing > 9 && EN.facing < 30) {
                        if (EN.facing < 20) {
                            EN.turnState = 1;
                            EN.turnTimer = RandFloat(0, 160.0f) + 40.0f;
                        } else {
                            EN.turnState = 3;
                            EN.turnTimer = RandFloat(0, 160.0f) + 40.0f;
                        }
                    }

                    if (EN.y < 80.0 && (EN.facing <= 9 || EN.facing >= 30)) {
                        if (EN.facing < 20) {
                            EN.turnState = 3;
                            EN.turnTimer = RandFloat(0, 160.0f) + 40.0f;
                        } else {
                            EN.turnState = 1;
                            EN.turnTimer = RandFloat(0, 160.0f) + 40.0f;
                        }
                    }
                }
                EN.dirStepTimer -= 1.0f * g_frameDt;
                if (EN.dirStepTimer < 0.0) {
                    if (g_state == STATE_MALFUNCTION)
                        EN.dirStepTimer = EN.animStepTime;
                    else
                        EN.dirStepTimer = 10.0f;
                    if (EN.turnState == 3) {
                        EN.facing++;

                        if (EN.facing > 39)
                            EN.facing = 0;
                    }
                    if (EN.turnState == 1) {
                        EN.facing--;
                        if (EN.facing < 0)
                            EN.facing = 39;
                    }
                }
                EN.srcY = (int)EN.animFrame * 64;
                EN.animTimer -= 1.0f * g_frameDt;
                if (EN.animTimer < 0.0) {
                    animDivisor = EN.animSpeedDivisor;
                    if (animDivisor == 0.0)
                        animDivisor = 1.0f;
                    EN.animTimer = EN.hp / (double)animDivisor;
                    if (EN.animReverse == 0) {
                        EN.animFrame -= 1.0;

                        if (EN.animFrame < 0.0) {
                            if (EN.animPingPong == 0) {
                                EN.animFrame = EN.animFrameCount;
                            } else {
                                EN.animFrame = 1.0f;
                                if (EN.animReverse == 0)
                                    EN.animReverse = 1;
                                else
                                    EN.animReverse = 0;
                            }
                        }
                    } else {
                        EN.animFrame += 1.0;
                        if (!(EN.animFrame < EN.animFrameCount)) {
                            if (EN.animPingPong == 0) {
                                EN.animFrame = 0;
                            } else {
                                EN.animFrame = EN.animFrameCount - 1.0;

                                if (EN.animReverse == 0)
                                    EN.animReverse = 1;
                                else
                                    EN.animReverse = 0;
                            }
                        }
                    }
                }
                break;
            // Type 12: "money ship" that wraps around all four screen edges like type 6 but does not fire;
            // uses the shared F090_TURN direction logic and a separate 128px-wide animation strip.
            case ENEMY_MONEY_SHIP:
                if (PL.freezeTimer)
                    break;
                if (g_timerA)
                    break;
                EN.x += EN.speedX * g_dirVecX2[EN.facing] * EN.speedScale * g_frameDt;
                EN.y += EN.speedY * g_dirVecY2[EN.facing] * EN.speedScale * g_frameDt;

                if (EN.x > g_screenW + 120)
                    EN.x = -120.0f;
                if (EN.x < -120.0)
                    EN.x = g_screenW + 110;
                if (EN.y > g_screenH + 120)
                    EN.y = -120.0f;
                if (EN.y < -120.0)
                    EN.y = g_screenH + 110;
                F090_TURN
                EN.dirStepTimer -= 1.0f * g_frameDt;
                if (EN.dirStepTimer < 0.0) {
                    EN.dirStepTimer = EN.animStepTime;
                    if (EN.turnState == 3) {
                        EN.facing++;
                        if (EN.facing > 39)
                            EN.facing = 0;
                    }
                    if (EN.turnState == 1) {
                        EN.facing--;

                        if (EN.facing < 0)
                            EN.facing = 39;
                    }
                }
                EN.srcX = (int)EN.animFrame * 128;
                EN.animTimer -= 1.0f * g_frameDt;
                if (EN.animTimer < 0.0) {
                    animDivisor2 = EN.animSpeedDivisor;
                    if (animDivisor2 == 0.0)
                        animDivisor2 = 1.0f;
                    EN.animTimer = EN.hp / (double)animDivisor2 / 4.0;
                    if (EN.animReverse == 0) {
                        EN.animFrame -= 1.0;
                        if (EN.animFrame < 0.0) {
                            if (EN.animPingPong == 0) {
                                EN.animFrame = EN.animFrameCount;
                            } else {
                                EN.animFrame = 1.0f;

                                if (EN.animReverse == 0)
                                    EN.animReverse = 1;
                                else
                                    EN.animReverse = 0;
                            }
                        }
                    } else {
                        EN.animFrame += 1.0;
                        if (EN.animFrame > EN.animFrameCount) {
                            if (EN.animPingPong == 0) {
                                EN.animFrame = 0;
                            } else {
                                EN.animFrame = EN.animFrameCount - 1;
                                if (EN.animReverse == 0)
                                    EN.animReverse = 1;
                                else
                                    EN.animReverse = 0;
                            }
                        }
                    }
                }

                break;
            // Type 9: mothership (spawned by HurryUp()). Flies straight across the screen firing rockets
            // when the player crosses one of the guard trigger points (g_triggerX); stops its engine-hum
            // sample and restores the hurry-up timer once it exits the far side.
            case ENEMY_MOTHERSHIP:
                if (PL.freezeTimer)
                    break;
                g_enemyActedThisFrame = 1;
                g_guardTriggerFlag = 0;
                if (EN.turnState == 0) {
                    for (triggerIdx = 0; triggerIdx < g_guardCount; triggerIdx++) {
                        if (EN.x > g_triggerX[triggerIdx] && g_triggerX[triggerIdx] != 0) {
                            g_triggerX[triggerIdx] = 0;
                            g_guardTriggerFlag = 1;
                        }
                    }
                } else {
                    for (triggerIdx2 = 0; triggerIdx2 < g_guardCount; triggerIdx2++) {
                        if (EN.x < g_triggerX[triggerIdx2] && g_triggerX[triggerIdx2] != 0) {
                            g_triggerX[triggerIdx2] = 0;

                            g_guardTriggerFlag = 1;
                        }
                    }
                }
                if (g_guardTriggerFlag) {
                    for (objIdx = 0; objIdx < MAX_LEVEL_OBJS; objIdx++) {
                        if (SH.active == 0) {
                            SH.turnDelay = g_defaultObjAlpha;
                            SH.active = 1;
                            SH.x = EN.x + 32.0f;
                            SH.y = EN.y + 25.0f;
                            SH.row = 0;
                            SH.frameDelay = RandRange(0, 3) + 4;
                            SH.frameTimer = RandRange(0, 3) + 3;
                            SH.turnDelay = RandRange(0, 5) + 3;
                            SH.turnTimer = SH.turnDelay;
                            SH.soundDelay = 1.0f;
                            SH.soundTimer = SH.soundDelay;

                            SH.fuse = RandRange(0, g_diffShotFuseRange) + g_diffShotFuseBase;
                            if (g_gameMode == MODE_DUAL) {
                                SH.player = RandRange(0, 2);
                                if (g_save.players[SH.player].lives
                                    < g_shipDefs[g_save.players[SH.player].ship]->minEnergy
                                          + g_shipDefs[g_save.players[SH.player].ship]->cost) {
                                    if (SH.player == 0)
                                        SH.player = 1;
                                    else
                                        SH.player = 0;
                                }
                            } else {
                                SH.player = g_curPlayer;
                            }
                            SH.type = LOBJ_ROCKET;
                            SH.frame = 17;
                            SH.speed = RandFloat(g_diffShotSpeedMin, g_diffShotSpeedMax);
                            SH.gfxA = g_gfxRocket;
                            SH.hma = (int)g_hmaRocket;

                            SH.hmaW = g_rocketGfxW;
                            SH.hmaH = g_rocketGfxH;
                            SH.hitOffsetX = 0;
                            SH.hitOffsetY = 0;
                            SH.w = 24;
                            SH.h = 24;
                            if (g_cfg.particlesOn)
                                AddParticle(g_gfxFlare4, (int)SH.x + 12, (int)SH.y + 12,
                                            RandFloat(10.0f, 15.0f), RandFloat(8.0f, 10.0f),
                                            RandFloat(0, 359.0f), RandFloat(0, 40.0f) - 20.0, 0,
                                            RandRange(0, 55), RandRange(200, 255), RandRange(0, 55), 500,
                                            RandFloat(10.0f, 20.0f), 0, 0, 0.8f, 0, 0, 0, 1);
                            SoundPlay(g_sfxRocket, 32000, SFX_VOL_FULL,
                                      g_panTable[ClampX((int)SH.x)], SFX_PAN_CENTER, g_sndFlags);
                            break;
                        }
                    }
                }
                if (EN.turnState == 0) {
                    EN.x += EN.speedX * g_frameDt;

                    if (EN.x > g_screenW + 70) {
                        EN.active = 0;
                        CreditEscape(g_curPlayer);
                        g_samples[g_curPlayer][enemyIdx] = SoundStop(g_samples[g_curPlayer][enemyIdx]);
                        switch (g_gameMode) {
                        case MODE_SINGLE:
                        case MODE_TWO_PLAYER:
                            PL.time = g_time + PL.effectDuration;
                            break;
                        case MODE_DUAL:
                            g_lastEventTime = g_time + g_bonusDuration;
                            break;
                        case MODE_TEAM:
                            g_lastEventTime = g_time + g_bonusDuration;
                            break;
                        case MODE_UNUSED_4:
                            break;
                        case MODE_ACE_TOURNAMENT:
                            break;
                        case MODE_TIME_TRIAL:
                            PL.time = g_time + PL.effectDuration;
                        }
                    }
                } else {

                    EN.x -= EN.speedX * g_frameDt;
                    if (EN.x < -70.0) {
                        EN.active = 0;
                        CreditEscape(g_curPlayer);
                        g_samples[g_curPlayer][enemyIdx] = SoundStop(g_samples[g_curPlayer][enemyIdx]);
                        switch (g_gameMode) {
                        case MODE_SINGLE:
                        case MODE_TWO_PLAYER:
                            PL.time = g_time + PL.effectDuration;
                            break;
                        case MODE_DUAL:
                            g_lastEventTime = g_time + g_bonusDuration;
                            break;
                        case MODE_TEAM:
                            g_lastEventTime = g_time + g_bonusDuration;
                            break;
                        case MODE_UNUSED_4:
                            break;
                        case MODE_ACE_TOURNAMENT:
                            break;
                        case MODE_TIME_TRIAL:
                            PL.time = g_time + PL.effectDuration;
                        }
                    }
                }

                g_animRowScratch = (int)EN.animFrame / 8;
                g_animColScratch = (int)EN.animFrame % 8;
                EN.srcY = g_animRowScratch * 96;
                EN.srcX = g_animColScratch * 57;
                EN.animTimer -= 1.0f * g_frameDt;
                if (EN.animTimer < 0.0) {
                    if (EN.animReverse) {
                        EN.animTimer = EN.dirStepTimer;
                        EN.animFrame -= 1.0;
                        if (EN.animFrame < 0.0)
                            EN.animFrame = 19.0f;
                    } else {
                        EN.animTimer = EN.dirStepTimer;
                        EN.animFrame += 1.0;
                        if (!(EN.animFrame < 20.0))
                            EN.animFrame = 0;
                    }
                }

                break;
            // Type 11: money/bonus-item dropper. Sweeps left-right across a band of the screen; while a
            // player has money it periodically drains some of it and spawns a matching bonus item drop, then
            // flies off the top (turnState 3) once the player's money is exhausted or by chance.
            case ENEMY_MONEY_SUCKER:
                if (PL.freezeTimer)
                    break;
                g_enemyActedThisFrame = 1;
                if (EN.x + 48.0 > 50.0 && EN.x + 48.0 < g_screenW - 50
                    && RandFloat(0, 100.0f) < g_frameDt * 10.0) {
                    targetPlayer = g_curPlayer;
                    if (g_gameMode == MODE_DUAL)
                        targetPlayer = RandRange(0, 2);
                    if (g_save.players[targetPlayer].money > 0.0 && g_save.players[targetPlayer].dead == 0
                        && g_enemies[targetPlayer][enemyIdx].turnState != 3
                        && g_save.players[targetPlayer].lives
                               > g_shipDefs[g_save.players[targetPlayer].ship]->minEnergy) {
                        for (itemIdx = 0; itemIdx < MAX_ITEMS; itemIdx++) {
                            if (g_items[itemIdx].alive == 0) {
                                dropRollMax = 40;

                                if (g_save.players[targetPlayer].money > 50)
                                    dropRollMax = 68;
                                if (g_save.players[targetPlayer].money > 100)
                                    dropRollMax = 88;
                                if (g_save.players[targetPlayer].money > 200)
                                    dropRollMax = 100;
                                itemGfxIdx = ITEM_MONEY_SMALL;
                                itemType = ITEM_MONEY_SMALL_TALLY;
                                moneyStolen = DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_10]);
                                dropRoll = RandRange(0, dropRollMax);
                                if (dropRoll <= MONEY_ROLL_SMALL_MAX) {
                                    itemGfxIdx = ITEM_MONEY_SMALL;
                                    itemType = ITEM_MONEY_SMALL_TALLY;
                                    moneyStolen = DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_10]);
                                }
                                if (dropRoll > MONEY_ROLL_SMALL_MAX && dropRoll <= MONEY_ROLL_MEDIUM_MAX) {
                                    itemGfxIdx = ITEM_MONEY_MEDIUM;
                                    itemType = ITEM_MONEY_MEDIUM_TALLY;

                                    moneyStolen = DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_50]);
                                }
                                if (dropRoll > MONEY_ROLL_MEDIUM_MAX && dropRoll <= MONEY_ROLL_LARGE_MAX) {
                                    itemGfxIdx = ITEM_MONEY_LARGE;
                                    itemType = ITEM_MONEY_LARGE_TALLY;
                                    moneyStolen = DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_100_B]);
                                }
                                if (dropRoll > MONEY_ROLL_LARGE_MAX) {
                                    itemGfxIdx = ITEM_MONEY_BLUE;
                                    itemType = ITEM_MONEY_BLUE_TALLY;
                                    moneyStolen = DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_200]);
                                }
                                g_save.players[targetPlayer].money =
                                    g_save.players[targetPlayer].money - moneyStolen;
                                if (g_save.players[targetPlayer].money > 0
                                    && RandRange(0, g_save.players[targetPlayer].money) < 32)
                                    EN.turnState = 3;
                                if (g_save.players[targetPlayer].money <= 0) {
                                    g_save.players[targetPlayer].money = 0;
                                    EN.turnState = 3;
                                }

                                g_moneyBlinkTimer = 0;
                                g_viewTransitionFlag = 2;
                                g_stateFn = SetViewHud;
                                EmptyViewChangeHook();
                                g_drawBordersFn = DrawBorders;
                                g_items[itemIdx].active = 0;
                                g_items[itemIdx].alive = 1;
                                g_items[itemIdx].x = g_save.players[targetPlayer].x;
                                g_items[itemIdx].y = g_save.players[targetPlayer].y;
                                g_items[itemIdx].vx = RandFloat(100.0f, 200.0f);
                                g_items[itemIdx].frameDelay = RandFloat(3.0f, 7.0f);
                                g_items[itemIdx].frameTimer = g_items[itemIdx].frameDelay;
                                g_items[itemIdx].gfx = g_gfxBonus;
                                g_items[itemIdx].srcX = g_itemBonusSrcX[itemGfxIdx];
                                g_items[itemIdx].srcY = g_itemBonusSrcY[itemGfxIdx];
                                g_items[itemIdx].h = g_itemHeightTable[itemGfxIdx];
                                g_items[itemIdx].w = g_itemWidthTable[itemGfxIdx];
                                g_items[itemIdx].frameCount = g_itemFrameCountTable[itemGfxIdx];

                                g_items[itemIdx].frame = RandRange(0, 10);
                                g_items[itemIdx].type = itemType;
                                break;
                            }
                        }
                    }
                }
                if (EN.turnState == 0) {
                    EN.x += EN.speedX * g_frameDt;
                    if (EN.x > g_screenW + 100) {
                        EN.turnState = 1;
                        EN.y = RandRange(0, g_screenH - 450) + 200;
                    }
                }
                if (EN.turnState == 1) {
                    EN.x -= EN.speedX * g_frameDt;
                    if (EN.x < -100.0) {
                        EN.turnState = 0;

                        EN.y = RandRange(0, g_screenH - 450) + 200;
                    }
                }
                if (EN.turnState == 3) {
                    EN.y -= EN.speedX * g_frameDt;
                    if (EN.y < -100.0) {
                        EN.active = 0;
                        CreditEscape(g_curPlayer);
                    }
                }
                g_targetX = EN.x + 60.0;
                g_targetY = EN.y + 20.0;
                EN.srcY = 0;
                EN.srcX = (int)EN.animFrame * 51;
                EN.animTimer -= 1.0f * g_frameDt;
                if (EN.animTimer < 0.0) {
                    if (EN.animReverse) {
                        EN.animTimer = EN.dirStepTimer;

                        EN.animFrame -= 1.0;
                        if (EN.animFrame < 0.0)
                            EN.animFrame = 10.0f;
                    } else {
                        EN.animTimer = EN.dirStepTimer;
                        EN.animFrame += 1.0;
                        if (!(EN.animFrame < 11.0))
                            EN.animFrame = 0;
                    }
                }
                break;
            // Type 18: boss "burst" turret. Randomly starts a burst window (g_bossBurstTimer), during which
            // it periodically fires a full-height wall of beam shots down the screen; otherwise flies
            // horizontally off one edge like type 6/12 (no wrap, just escapes).
            case ENEMY_GUARD:
                g_enemyActedThisFrame = 1;
                if (RandRange(0, 1000) < 5)
                    g_bossBurstTimer = RandRange(50, 150);

                if (g_bossBurstTimer > 0) {
                    g_bossBurstTimer--;
                    if (RandRange(0, 99) < 10) {
                        burstY = EN.y + 30.0f;
                        // NOTE: assigned but never read; the loop below uses the 70.0f literal directly
                        beamStepUnused = 70.0f;
                        AddParticle(g_gfxBossBurst, (int)EN.x + 64, (int)burstY, 256.0f,
                                    RandFloat(2.0f, 15.0f) + 4.2f, 0, 0, 0, 200, 255, 200, 600, 15.0f, 0, -1,
                                    0.2f, 0, 0, 0, 1);
                        do {
                            for (objIdx = 0; objIdx < MAX_LEVEL_OBJS; objIdx++) {
                                if (SH.active == 0) {
                                    SH.turnDelay = g_defaultObjAlpha;
                                    SH.active = 1;
                                    SH.x = EN.x + 64.0f - 32.0f;
                                    SH.y = burstY;
                                    SH.vx = 0;
                                    SH.vy = 0;
                                    SH.type = LOBJ_BEAM;
                                    SH.gfxA = g_gfxBeam;

                                    SH.gfxB = g_gfxBeam;
                                    SH.hma = (int)g_hmaLogoBirdFlare;
                                    SH.hmaW = g_logoBirdFlareGfxW;
                                    SH.hmaH = g_logoBirdFlareGfxH;
                                    SH.hitOffsetX = 0;
                                    SH.hitOffsetY = 0;
                                    SH.w = 64;
                                    SH.h = 70;
                                    SH.f28 = 5.0f;
                                    break;
                                }
                            }
                            burstY += 70.0f;
                        } while (burstY < g_screenH);
                        SoundPlay(g_sfxWooing, RandRange(12000, 20000), SFX_VOL_FULL,
                                         g_panTable[ClampX((int)SH.x)], SFX_PAN_CENTER, g_sndFlags);
                    }
                }

                if (g_bossBurstTimer < 1) {
                    if (EN.turnState == 0) {
                        EN.x += EN.speedX;
                        if (EN.x > g_screenW + 100) {
                            EN.active = 0;
                            CreditEscape(g_curPlayer);
                            g_samples[g_curPlayer][enemyIdx] = SoundStop(g_samples[g_curPlayer][enemyIdx]);
                        }
                    }
                    if (EN.turnState == 1) {
                        EN.x -= EN.speedX;
                        if (EN.x < -100.0) {
                            EN.active = 0;
                            CreditEscape(g_curPlayer);
                            g_samples[g_curPlayer][enemyIdx] = SoundStop(g_samples[g_curPlayer][enemyIdx]);
                        }
                    }
                }

                EN.srcY = 0;
                EN.srcX = (int)EN.animFrame * (g_guardHeight / 10);
                EN.animTimer -= 1.0f * g_frameDt;
                if (EN.animTimer < 0.0) {
                    if (EN.animReverse) {
                        EN.animTimer = EN.dirStepTimer;
                        EN.animFrame -= 1.0;
                        if (EN.animFrame < 0.0)
                            EN.animFrame = 9.0f;
                    } else {
                        EN.animTimer = EN.dirStepTimer;
                        EN.animFrame += 1.0;
                        if (!(EN.animFrame < 10.0))
                            EN.animFrame = 0;
                    }
                }
                break;

            // Type 13: level boss. Slides its engine-drone pitch around, fires from up to g_bossGunCountA gun
            // hardpoints when its "gun A" group is active, records itself as the current boss (g_bossIdx),
            // homes in on a saved position between pattern groups, and can trigger a screen-wide debris burst
            // (pattern cmd 2) or arm its guns (cmd 3) partway through its movement pattern.
            // NOTE: the five `if (<invariant holds>) {} else { l2xx = 0; l2xx = l2xx / l2xx; }` blocks below
            // are the original's range-check assertions: they do nothing when the invariant holds and
            // deliberately divide by zero (crash) when it doesn't, in place of a proper assert(). Kept as-is
            // for the byte match.
            case ENEMY_BOSS:
                g_enemyActedThisFrame = 1;

                g_engineDroneFreq += g_engineDroneFreqStep;
                if (g_engineDroneFreq > g_engineFreqMax) {
                    g_engineDroneFreqStep = 0 - RandRange(10, 100);
                    g_engineDroneFreq = g_engineFreqMax;
                    g_engineFreqMin = RandRange(15000, g_engineFreqMax);
                }
                if (g_engineDroneFreq < g_engineFreqMin) {
                    g_engineDroneFreqStep = RandRange(10, 100);
                    g_engineDroneFreq = g_engineFreqMin;
                    g_engineFreqMax = RandRange(g_engineFreqMin, 30000);
                }
                if (RandRange(0, 1000) < 6 && g_samples[g_curPlayer][enemyIdx] != 0)
                    BASS_ChannelSlideAttribute(g_samples[g_curPlayer][enemyIdx], BASS_ATTRIB_FREQ, g_engineDroneFreq,
                                               RandRange(1000, 2000));
                if (EN.groupIndex == 1 && g_bossGunActiveA) {
                    for (gunIdx = 0; gunIdx < g_bossGunCountA; gunIdx++) {
                        if (RandFloat(0, EN.fireDelay) < 3.0 * g_frameDt) {
                            timeToTarget = RandFloat(45.0f, 55.0f) * g_diffEnemyTimerMul;
                            startX = EN.x;

                            startY = EN.y;
                            if (g_gameMode == MODE_DUAL) {
                                aimPlayer2 = RandRange(0, 2);
                                if (g_save.players[aimPlayer2].lives
                                    < g_shipDefs[g_save.players[aimPlayer2].ship]->minEnergy
                                          + g_shipDefs[g_save.players[aimPlayer2].ship]->cost) {
                                    if (aimPlayer2 == 0)
                                        aimPlayer2 = 1;
                                    else
                                        aimPlayer2 = 0;
                                }
                                targetX = g_save.players[aimPlayer2].x - RandFloat(-40.0f, 40.0f);
                                targetY = g_save.players[aimPlayer2].y - RandFloat(-40.0f, 40.0f);
                            } else {
                                targetX = PL.x - RandFloat(-40.0f, 40.0f);
                                targetY = PL.y - RandFloat(-40.0f, 40.0f);
                            }
                            if (timeToTarget == 0.0)
                                timeToTarget = 1.0f;

                            aimVx = (targetX - startX) / timeToTarget;
                            aimVy = (targetY - startY) / timeToTarget;
                            for (objIdx = 0; objIdx < MAX_LEVEL_OBJS; objIdx++) {
                                if (SH.active == 0) {
                                    SH.turnDelay = g_defaultObjAlpha;
                                    SH.active = 1;
                                    SH.x = EN.x + EN.bossGunAX[gunIdx];
                                    SH.y = EN.y + EN.bossGunAY[gunIdx];
                                    AddParticle(g_gfxMissileSpark, (int)SH.x, (int)SH.y, 48.0f,
                                                RandFloat(0, 5.0f) + 2.0f, RandFloat(0, 359.0f), 0, 0, 255,
                                                200, 100, 400, 15.0f, 0, -1, 0, 0, 0, 0, 1);
                                    SH.vx = aimVx * g_frameDt;
                                    SH.vy = aimVy * g_frameDt;
                                    SH.type = LOBJ_BOSS_SHOT;
                                    SH.gfxA = g_alienGfxCache[0].gfx1;
                                    SH.gfxB = g_alienGfxCache[0].gfx2;
                                    SH.hma = (int)g_alienGfxMem[0];
                                    SH.hmaW = g_hazard0GfxW;

                                    SH.hmaH = g_hazard0GfxH;
                                    SH.hitOffsetX = 8;
                                    SH.hitOffsetY = 8;
                                    SH.w = 24;
                                    SH.h = 24;
                                    SoundPlay(g_sfxBigSmall, RandRange(28000, 32000), SFX_VOL_FULL,
                                                     g_panTable[ClampX((int)SH.x)], SFX_PAN_CENTER, g_sndFlags);
                                    break;
                                }
                            }
                        }
                    }
                }
                g_bossIdx = enemyIdx;
                if ((int)g_bossAnimStep == 1 || (int)g_bossAnimStep == -1) {
                } else {
                    trapBossAnimStep = 0;
                    trapBossAnimStep = trapBossAnimStep / trapBossAnimStep;
                }

                if (g_animFrameCount > 6) {
                    trapFrameCountHigh = 0;
                    trapFrameCountHigh = trapFrameCountHigh / trapFrameCountHigh;
                }
                if (g_animFrameCount < 1) {
                    trapFrameCountLow = 0;
                    trapFrameCountLow = trapFrameCountLow / trapFrameCountLow;
                }
                if ((int)EN.animFrame > 5) {
                    trapAnimFrameHigh = 0;
                    trapAnimFrameHigh = trapAnimFrameHigh / trapAnimFrameHigh;
                }
                if ((int)EN.animFrame < 0) {
                    trapAnimFrameLow = 0;
                    trapAnimFrameLow = trapAnimFrameLow / trapAnimFrameLow;
                }
                EN.animTimer -= 1.0f * g_frameDt;
                if (EN.animTimer < 0.0) {
                    EN.animTimer = EN.animDelay;

                    EN.animFrame += g_bossAnimStep;
                    if (EN.animPingPong == 0) {
                        if (!(EN.animFrame < g_animFrameCount))
                            EN.animFrame = 0;
                        if (EN.animFrame < 0.0)
                            EN.animFrame = g_animFrameCount - 1.0f;
                    } else {
                        if (!(EN.animFrame < g_animFrameCount)) {
                            EN.animFrame = g_animFrameCount - 1.0f;
                            g_bossAnimStep = -1.0f;
                        }
                        if (EN.animFrame < 0.0) {
                            EN.animFrame = 0;
                            g_bossAnimStep = 1.0f;
                        }
                    }
                }
                if (EN.homing) {
                    EN.x = EN.homeX + (EN.x - EN.homeX) * 0.96f;

                    EN.y = EN.homeY + (EN.y - EN.homeY) * 0.96f;
                    if (abs((int)(EN.x - EN.homeX)) < 1.0f * g_frameDt
                        && abs((int)(EN.y - EN.homeY)) < 1.0f * g_frameDt) {
                        EN.homing = 0;
                        EN.x = EN.homeX;
                        EN.y = EN.homeY;
                        EN.patternStep = 0;
                        homingGroupIdx = 0;
                        for (groupSearchIdx = 0; groupSearchIdx < g_curLevelData.count; groupSearchIdx++) {
                            if (g_curLevelData.grp[groupSearchIdx].kind == 5)
                                homingGroupIdx = groupSearchIdx;
                        }
                        EN.velX = g_curLevelData.grp[homingGroupIdx].velX;
                        EN.velY = g_curLevelData.grp[homingGroupIdx].velY;
                        EN.accelX = PT.pathX;
                        EN.accelY = PT.pathY;
                    }
                    break;
                }

                EN.x += EN.velX * g_frameDt;
                EN.y += EN.velY * g_frameDt;
                EN.velX += EN.accelX * g_frameDt;
                EN.velY += EN.accelY * g_frameDt;
                EN.patternTimer += 1.0f * g_frameDt;
                if ((int)EN.patternTimer > PT.holdTime) {
                    EN.patternStep++;
                    if (PT.cmd == BOSSCMD_HOMING) {
                        EN.homing = 1;
                        if (EN.velX < 0.0)
                            EN.velX = 0 - EN.velX;
                        if (EN.velY < 0.0)
                            EN.velY = 0 - EN.velY;
                        EN.accelX = 0;
                        EN.accelY = 0;
                        EN.patternTimer = 0;
                        break;
                    }

                    if (PT.cmd == ARM_GUNS) {
                        g_bossGunActiveA = 1;
                        break;
                    }
                    if (PT.cmd == BOSSCMD_STOP) {
                        EN.velX = 0;
                        EN.velY = 0;
                        EN.accelX = 0;
                        EN.accelY = 0;
                        EN.patternTimer = 0;
                        break;
                    }
                    if (PT.cmd == BURST_FIRE) {
                        burstGroupIdx = 0;
                        subShotsLeft = 0;
                        for (groupSearchIdx2 = 0; groupSearchIdx2 < g_curLevelData.count; groupSearchIdx2++) {
                            if (g_curLevelData.grp[groupSearchIdx2].kind == 6) {
                                burstGroupIdx = groupSearchIdx2;

                                subShotsLeft = g_curLevelData.grp[burstGroupIdx].count;
                                do {
                                    spawnedFlag = 0;
                                    for (objIdx = 0; objIdx < MAX_LEVEL_OBJS; objIdx++) {
                                        if (SH.active == 0) {
                                            SH.turnDelay = g_defaultObjAlpha;
                                            SH.active = 1;
                                            SH.x = EN.x + g_curLevelData.grp[burstGroupIdx].spawnX;
                                            SH.y = EN.y + g_curLevelData.grp[burstGroupIdx].spawnY
                                                       + 64.0f - 16.0f;
                                            aimVx = g_curLevelData.grp[burstGroupIdx]
                                                        .sub[subShotsLeft - 1].xOffset;
                                            aimVy = g_curLevelData.grp[burstGroupIdx]
                                                        .sub[subShotsLeft - 1].yOffset;
                                            aimDist = Sqrt(aimVx * aimVx + aimVy * aimVy);
                                            if (aimDist == 0.0)
                                                aimDist = 1.0f;
                                            aimDirX = aimVx / aimDist;
                                            aimDirY = aimVy / aimDist;
                                            aimSpeed = g_curLevelData.grp[burstGroupIdx].sub[0].hp / 10.0;
                                            SH.vx = aimSpeed * aimDirX * g_frameDt;

                                            SH.vy = aimSpeed * aimDirY * g_frameDt;
                                            SH.type = LOBJ_BOSS_BURST;
                                            SH.gfxA = g_alienGfxCache[0].gfx1;
                                            SH.gfxB = g_alienGfxCache[0].gfx2;
                                            SH.hma = (int)g_alienGfxMem[0];
                                            SH.hmaW = g_hazard0GfxW;
                                            SH.hmaH = g_hazard0GfxH;
                                            SH.hitOffsetX = 4;
                                            SH.hitOffsetY = 4;
                                            SH.w = 28;
                                            SH.h = 28;
                                            SH.row = 0;
                                            SH.frameDelay = RandRange(0, 3) + 1;
                                            SH.frameTimer = RandRange(0, 3) + 1;
                                            g_fadeQueueSample3 = g_sfxAlienShoot2;
                                            g_sfxPanIdx = g_rampB[ClampY((int)SH.x)];
                                            g_sfxVolume = g_rampB[ClampY((int)SH.y)];
                                            g_sfxFreq = RandRange(24000, 30000);

                                            spawnedFlag = 1;
                                            subShotsLeft--;
                                            break;
                                        }
                                    }
                                } while (subShotsLeft > 0 && spawnedFlag != 0);
                                AddParticle(g_gfxExplodeDebris,
                                            (int)EN.x + g_curLevelData.grp[burstGroupIdx].spawnX + 16,
                                            (int)EN.y + g_curLevelData.grp[burstGroupIdx].spawnY + 64,
                                            200.0f, RandFloat(0, 5.0f) + 2.0f, RandFloat(0, 359.0f), 0, 0,
                                            200, 255, 200, 800, 20.0f, 0, -1, 0, 0, 0, 0, 1);
                                SoundPlay(g_sfxBigFire, RandRange(28000, 32000), SFX_VOL_FULL, 0, SFX_PAN_CENTER, g_sndFlags);
                            }
                        }
                    }
                    EN.accelX = PT.pathX;
                    EN.accelY = PT.pathY;
                    EN.patternTimer = 0;
                    if (PT.holdTime == 0) {
                        if (g_curLevelData.grp[EN.groupIndex].kind == 5) {
                            EN.x = g_savedEnemyX;

                            EN.y = g_savedEnemyY;
                            EN.patternStep = 0;
                            EN.velX = g_curLevelData.grp[1].velX;
                            EN.velY = g_curLevelData.grp[1].velY;
                            EN.accelX = 0;
                            EN.accelY = 0;
                        }
                        if (g_curLevelData.grp[EN.groupIndex].kind == 4) {
                            g_savedEnemyX = EN.x;
                            g_savedEnemyY = EN.y;
                            EN.homeX = EN.x;
                            EN.homeY = EN.y;
                            homingGroupIdx2 = 0;
                            for (groupSearchIdx3 = 0; groupSearchIdx3 < g_curLevelData.count;
                                 groupSearchIdx3++) {
                                if (g_curLevelData.grp[groupSearchIdx3].kind == 5)
                                    homingGroupIdx2 = groupSearchIdx3;
                            }
                            EN.groupIndex = homingGroupIdx2;

                            EN.patternStep = 0;
                            EN.velX = g_curLevelData.grp[homingGroupIdx2].velX;
                            EN.velY = g_curLevelData.grp[homingGroupIdx2].velY;
                            EN.accelX = PT.pathX;
                            EN.accelY = PT.pathY;
                        }
                    }
                }
                break;
            }
        }
    }
}

#undef VF
#undef PL
#undef EN
#undef E2
#undef SH
#undef PT
#undef FP
#undef SHOT_SOUNDS
#undef SHOT_FIRE_A
#undef ANTI_CHEAT
#undef ANIM_B
#undef F090_TURN
#undef EN_RESTORE

#define E g_enemies[g_curPlayer][i]   // the enemy slot being drawn (loop index i)

// True when the sprite rect (x,y,w,h) overlaps the current clip rect at all.
#define VISIBLE() CLIP_VISIBLE(x, y, w, h)

// Clips (x,y,w,h) and the matching source rect to the clip rect, adjusting src.x1/y1 for any left/top cut.
#define CLIP() CLIP_SRC_RECT(x, y, w, h, src)

// Caches the (unclipped) source rect on E, used later for hit-testing against player shots.
#define SETRECT() \
    E.srcRect.x1 = src.x1; \
    E.srcRect.y1 = src.y1; \
    E.srcRect.x2 = src.x1 + w; \
    E.srcRect.y2 = src.y1 + h;

// Blits E's normal frame (gfxA), or its alternate frame (gfxB) while altFrameCounter is still counting down.
#define BLIT_AB() \
    if (E.altFrameCounter == 0) { \
        QueueBlit((float)x, (float)y, E.gfxA, &src); \
    } else { \
        QueueBlit((float)x, (float)y, E.gfxB, &src); \
        E.altFrameCounter--; \
    }

// Like BLIT_AB, but flickers to the hit-flash graphics for a random fraction of frames while hitFlashTimer
// is counting down (visual feedback for a recent hit).
#define BLIT_ALL() \
    if (E.hitFlashTimer > 0) { \
        if (RandRange(0, 100) < E.hitFlashTimer) { \
            if (E.altFrameCounter == 0) { \
                QueueBlit((float)x, (float)y, E.hitFlashGfxA, &src); \
            } else { \
                QueueBlit((float)x, (float)y, E.hitFlashGfxB, &src); \
                E.altFrameCounter--; \
            } \
        } else { \
            BLIT_AB() \
        } \
    } else { \
        BLIT_AB() \
    }

// Boss sprite blit: the boss is split across 6 cached graphics (g_alienGfxCache[0..5]), one per anim frame,
// selected by E.animFrame; altFrameCounter again picks between the .gfx1/.gfx2 half of each cached pair.
#define BLIT_BOSS() \
    if (E.altFrameCounter == 0) { \
        if ((int)E.animFrame == 0) QueueBlit((float)x, (float)y, g_alienGfxCache[0].gfx1, &src); \
        if ((int)E.animFrame == 1) QueueBlit((float)x, (float)y, g_alienGfxCache[1].gfx1, &src); \
        if ((int)E.animFrame == 2) QueueBlit((float)x, (float)y, g_alienGfxCache[2].gfx1, &src); \
        if ((int)E.animFrame == 3) QueueBlit((float)x, (float)y, g_alienGfxCache[3].gfx1, &src); \
        if ((int)E.animFrame == 4) QueueBlit((float)x, (float)y, g_alienGfxCache[4].gfx1, &src); \
        if ((int)E.animFrame == 5) QueueBlit((float)x, (float)y, g_alienGfxCache[5].gfx1, &src); \
    } else { \
        if ((int)E.animFrame == 0) QueueBlit((float)x, (float)y, g_alienGfxCache[0].gfx2, &src); \
        if ((int)E.animFrame == 1) QueueBlit((float)x, (float)y, g_alienGfxCache[1].gfx2, &src); \
        if ((int)E.animFrame == 2) QueueBlit((float)x, (float)y, g_alienGfxCache[2].gfx2, &src); \
        if ((int)E.animFrame == 3) QueueBlit((float)x, (float)y, g_alienGfxCache[3].gfx2, &src); \
        if ((int)E.animFrame == 4) QueueBlit((float)x, (float)y, g_alienGfxCache[4].gfx2, &src); \
        if ((int)E.animFrame == 5) QueueBlit((float)x, (float)y, g_alienGfxCache[5].gfx2, &src); \
        E.altFrameCounter--; \
    }

// Draws every active enemy slot for the current player: builds the source rect for its current frame,
// clips it to the screen, and queues the blit. `count` tracks how many were actually drawn so the level
// stall watchdog (LevelStallWatchdog) can tell whether anything is still visibly happening on screen.
void DrawEnemies()
{
    Rect16 src;
    int i;
    int x;
    int y;
    int w;
    int h;
    int count;

    count = 0;
    if (g_gameMode == MODE_DUAL)
        g_curPlayer = 0;

    for (i = 0; i < MAX_ENEMIES; i++) {
        switch (E.type) {
        // Type 2 (hovering): 32x32, drawn at its hover offset (offsetX/offsetY included in position).
        case ENEMY_HOVER:
            if (E.active == 1) {
                while (1) {
                    x = (int)(E.x + E.offsetX);
                    y = (int)(E.y + E.offsetY);
                    w = 32;
                    h = 32;
                    src.x1 = E.srcY;
                    src.y1 = E.srcX;
                    SETRECT()
                    if (!VISIBLE())
                        break;
                    count++;
                    CLIP()
                    BLIT_ALL()
                    break;
                }
            }
            break;

        // Types 5 (falls through to force altFrameCounter=1, i.e. always draw gfxB), 1, 3, 4, 7, 8, 10:
        // shared 32x32 draw path (standard alien, dive/fall variants, hazard, beam, escaper). Skipped while
        // its attack-stagger timer is running; if EN.flashActive, spawns a spark instead of drawing a frame
        // (hit-flash placeholder), and mirrors type-8 beams to the other side when the owner's mirror mode
        // is on. Type 8 (beam) is drawn but not counted toward the stall-watchdog `count`.
        case ENEMY_DEBRIS:
            E.altFrameCounter = 1;
        case ENEMY_PATTERNED:
        case ENEMY_DIVING:
        case ENEMY_FLYBY:
        case ENEMY_UNKNOWN_7:
        case ENEMY_CAPTURED:
        case ENEMY_ESCAPER:
            if (E.active == 1 && E.attackStaggerTimer <= 0.0) {
                while (1) {
                    x = (int)E.x;
                    if (E.type == ENEMY_CAPTURED && g_save.players[E.ownerPlayer].mirrorTime != 0) {
                        if (Rand7f() < 64)
                            x = g_screenW - x - 32;
                    }
                    y = (int)E.y;
                    w = 32;
                    h = 32;

                    if (E.flashActive) {
                        AddParticle(g_gfxSpark, x + 16, y + 16, 35.0f, 1 + RandFloat(0.0f, 5.0f),
                                           RandFloat(0.0f, 359.0f), RandFloat(-5.0f, 5.0f),
                                           0, 255, 255, 255, 600, 15.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);
                        E.flashTimer -= 1.0f * g_frameDt;
                        if (E.flashTimer < 0.0)
                            E.flashActive = 0;
                        break;
                    }

                    src.x1 = E.srcY;
                    src.y1 = E.srcX;
                    SETRECT()
                    if (!VISIBLE())
                        break;
                    if (E.type != ENEMY_CAPTURED)
                        count++;
                    CLIP()
                    BLIT_ALL()
                    break;
                }
            }
            break;

        // Type 6 (wrap-around alien): 64x64.
        case ENEMY_WRAPPER:
            if (E.active == 1) {
                while (1) {
                    x = (int)E.x;
                    y = (int)E.y;
                    w = 64;
                    h = 64;
                    src.x1 = E.srcY;
                    src.y1 = E.srcX;
                    SETRECT()
                    if (!VISIBLE())
                        break;
                    count++;
                    CLIP()
                    BLIT_AB()
                    break;
                }
            }
            break;

        // Type 12 (money ship): 128x128, single row of frames selected by srcX (or row 0 while
        // altFrameCounter is active, i.e. showing the alternate/"hit" graphic in BLIT_AB).
        case ENEMY_MONEY_SHIP:
            if (E.active != 0) {
                while (1) {
                    x = (int)E.x;
                    y = (int)E.y;
                    w = 128;
                    h = 128;
                    src.x1 = 0;
                    if (E.altFrameCounter == 0)
                        src.y1 = E.srcX;
                    else
                        src.y1 = 0;
                    SETRECT()
                    if (!VISIBLE())
                        break;
                    count++;
                    CLIP()
                    BLIT_AB()
                    break;
                }
            }
            break;

        // Type 9 (mothership): 96x57.
        case ENEMY_MOTHERSHIP:
            if (E.active == 1) {
                while (1) {
                    x = (int)E.x;
                    y = (int)E.y;
                    w = 96;
                    h = 57;
                    src.x1 = E.srcY;
                    src.y1 = E.srcX;
                    SETRECT()
                    if (!VISIBLE())
                        break;
                    count++;
                    CLIP()
                    BLIT_AB()
                    break;
                }
            }
            break;

        // Type 11 (money/bonus-item dropper): 128x51.
        case ENEMY_MONEY_SUCKER:
            if (E.active == 1) {
                while (1) {
                    x = (int)E.x;
                    y = (int)E.y;
                    w = 128;
                    h = 51;
                    src.x1 = E.srcY;
                    src.y1 = E.srcX;
                    SETRECT()
                    if (!VISIBLE())
                        break;
                    count++;
                    CLIP()
                    BLIT_AB()
                    break;
                }
            }
            break;

        // Type 18 (boss burst turret): 128x64. Draws gfxA normally; while flashing to gfxB (its beam-fire
        // frame) it forces src.y1 to row 0, and Rand7f() has a chance each frame to cut the flash short by
        // resetting altFrameCounter to 0.
        case ENEMY_GUARD:
            if (E.active == 1) {
                while (1) {
                    x = (int)E.x;
                    y = (int)E.y;
                    w = 128;
                    h = 64;
                    src.x1 = E.srcY;
                    src.y1 = E.srcX;
                    SETRECT()
                    if (!VISIBLE())
                        break;
                    count++;
                    CLIP()

                    if (E.altFrameCounter == 0) {
                        QueueBlit((float)x, (float)y, E.gfxA, &src);
                    } else {
                        src.y1 = 0;
                        src.y2 = src.y1 + h;
                        QueueBlit((float)x, (float)y, E.gfxB, &src);
                        E.altFrameCounter--;
                        if (Rand7f() < 100)
                            E.altFrameCounter = 0;
                    }
                    break;
                }
            }
            break;

        // Type 13 (boss): drawn as two stacked 256x64 halves (top then bottom, offset +64 in y and in the
        // source atlas), each picking one of 6 cached boss graphics by animFrame via BLIT_BOSS. Only the top
        // half counts toward the stall-watchdog `count`.
        case ENEMY_BOSS:
            if (E.active != 0) {
                while (1) {
                    x = (int)E.x - 112;
                    y = (int)E.y;
                    w = 256;
                    h = 64;
                    src.x1 = 0;
                    src.y1 = 0;
                    if (!VISIBLE())
                        break;
                    count++;
                    CLIP()
                    BLIT_BOSS()
                    break;
                }

                while (1) {
                    x = (int)E.x - 112;
                    y = (int)E.y + 64;
                    w = 256;
                    h = 64;
                    src.x1 = 256;
                    src.y1 = 0;
                    if (!VISIBLE())
                        break;
                    CLIP()
                    BLIT_BOSS()
                    break;
                }
            }
            break;
        }
    }
    if (count > 0)
        g_playerStallTime = g_time;
}

#undef E
#undef VISIBLE
#undef CLIP
#undef SETRECT
#undef BLIT_AB
#undef BLIT_ALL
#undef BLIT_BOSS
