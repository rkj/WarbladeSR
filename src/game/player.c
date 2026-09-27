// player.c: The player ships: update, demo AI, firing, being hit, placement and initialisation,
// ship effects.
#include <stdio.h>
#include "globals.h"
#include "game.h"


// True once player idx has collected all 6 letter-marks (bits 0-5 of `marks` all set).
int PlayerHasAllMarks(int idx)
{
    if ((short)g_save.players[idx].marks == MARKS_ALL)
        return 1;
    else
        return 0;
}

// Clears both players' shield and scoop power-up timers and stops all sound (used when starting a fresh
// game/level so leftover power-up state doesn't carry over).
void ResetPlayerTimers()
{
    g_save.players[0].shieldTimer = 0;
    g_save.players[1].shieldTimer = 0;
    g_save.players[0].scoopTimer = 0;
    g_save.players[1].scoopTimer = 0;
    SoundStopAll();
}

// Applies the penalty for the player being hit: drops autofire/superAuto/alien-lock,
// steps the weapon down one level, and shaves speed/buff duration/bullet speed back
// toward their base values. No-op in time-trial mode (MODE_TIME_TRIAL).
void PlayerHit(int p)
{
    g_save.players[p].autofire = 0;
    if (g_gameMode == MODE_TIME_TRIAL)
        return;
    g_save.players[p].alienLock = 0;
    g_save.players[p].superAuto = 0;
    g_save.players[p].autofireInterval = 100;
    if (g_save.players[p].bullets > 5)
        g_save.players[p].bullets--;

    if (g_save.players[p].weaponFloorAtOne == 0)
    {
        if (g_save.players[p].weapon > 0)
            g_save.players[p].weapon--;
    }
    else
    {
        if (g_save.players[p].weapon > 1)
            g_save.players[p].weapon--;
    }

    g_save.players[p].speed = g_save.players[p].speed - g_speedStep;
    if (g_save.players[p].speed < g_speedBase)
        g_save.players[p].speed = g_speedBase;
    g_save.players[p].buffDuration = g_save.players[p].buffDuration - 5;
    if (g_save.players[p].buffDuration < g_speedMin)
        g_save.players[p].buffDuration = g_speedMin;
    g_save.players[p].bulletSpeedMult = (g_save.players[p].bulletSpeedMult - 1.0f) * 0.5f + 1.0f;
    if (g_save.players[p].bulletSpeedMult < 1.0f)
        g_save.players[p].bulletSpeedMult = 1.0f;
}

// Fires the current player's weapon (and any shield-orb "transformed" side shots) if
// their energy is below `level`. `countsAsShot` is forwarded to SpawnDebris for the main
// shot only (1 = main shot, counts toward shots-fired stats; 0 = shield-orb mirrored shots,
// which always pass 0 for their own SpawnDebris calls below regardless of this parameter).
void FirePlayer(int level, int countsAsShot)
{
    int transveapon[10];  // NOTE: "transformed weapon" lookup for shield-orb side shots; misspelling
                           // kept — it's an array, so its name is fixed by the /RTC1 descriptor.
    int weapon;
    int shots;
    int pl;
    float spread;
    float rnd;
    bool played;

    transveapon[0] = 16;
    transveapon[1] = 15;
    transveapon[2] = 14;
    transveapon[3] = 3;
    transveapon[4] = 13;
    transveapon[5] = 11;
    transveapon[6] = 17;
    transveapon[7] = 10;
    transveapon[8] = 12;
    transveapon[9] = 9;

    // current-effective player index and shot-spread jitter
    pl = g_curPlayer;
    if (g_gameMode == MODE_DUAL) {
        pl = 0;
    }
    spread = (g_save.players[pl].gameSpeedSetting - 50.0) / 20.0;
    rnd = RandFloat(0, spread);
    played = false;
    if (g_save.players[g_curPlayer].energy < level) {
        shots = 1;
        weapon = g_save.players[g_curPlayer].weapon;
        g_debrisSpawnedCount = 0;
        SpawnDebris(g_save.players[g_curPlayer].x, g_save.players[g_curPlayer].y,
                        g_curPlayer, g_shotType[weapon], g_shotSpeed[weapon], countsAsShot,
                        g_save.players[g_curPlayer].bulletSpeedMult, rnd);
        if (g_debrisSpawnedCount > 0 && g_save.players[g_curPlayer].nextShotSnd < g_time) {
            played = true;
            g_save.players[g_curPlayer].nextShotSnd = g_time + 50;
            SoundPlay2(g_sampleHandle[g_save.players[g_curPlayer].weapon],
                              g_sampleRate[g_save.players[g_curPlayer].weapon],
                              g_sampleVol[g_save.players[g_curPlayer].weapon],
                              g_panTable[ClampX((int)g_save.players[g_curPlayer].x)],
                              RandRange(235, 187), g_sndFlags2);
        }

        // left shield-orb side shot
        if (g_save.players[g_curPlayer].shieldL != 0 &&
            g_enemies[pl][g_save.players[g_curPlayer].shieldLIdx].settled != 0) {
            g_debrisSpawnedCount = 0;
            SpawnDebris(g_save.players[g_curPlayer].x - 36.0, g_save.players[g_curPlayer].y,
                            g_curPlayer, g_shotType[transveapon[weapon]], g_shotSpeed[transveapon[weapon]], 0,
                            g_save.players[g_curPlayer].bulletSpeedMult, rnd);
            if (g_debrisSpawnedCount > 0 && played && g_save.players[g_curPlayer].nextShotSnd < g_time) {
                g_save.players[g_curPlayer].nextShotSnd = g_time + 50;
                SoundPlay2(g_sampleHandle[g_save.players[g_curPlayer].weapon],
                                  g_sampleRate[g_save.players[g_curPlayer].weapon],
                                  64,
                                  g_panTable[ClampX((int)g_save.players[g_curPlayer].x)],
                                  RandRange(235, 187), g_sndFlags2);
            }
        }

        // right shield-orb side shot
        if (g_save.players[g_curPlayer].shieldR != 0 &&
            g_enemies[pl][g_save.players[g_curPlayer].shieldRIdx].settled != 0) {
            g_debrisSpawnedCount = 0;
            SpawnDebris(g_save.players[g_curPlayer].x + 36.0, g_save.players[g_curPlayer].y,
                            g_curPlayer, g_shotType[transveapon[weapon]], g_shotSpeed[transveapon[weapon]], 0,
                            g_save.players[g_curPlayer].bulletSpeedMult, rnd);
            if (g_debrisSpawnedCount > 0 && played && g_save.players[g_curPlayer].nextShotSnd < g_time) {
                g_save.players[g_curPlayer].nextShotSnd = g_time + 50;
                SoundPlay2(g_sampleHandle[g_save.players[g_curPlayer].weapon],
                                  g_sampleRate[g_save.players[g_curPlayer].weapon],
                                  64,
                                  g_panTable[ClampX((int)g_save.players[g_curPlayer].x)],
                                  RandRange(235, 187), g_sndFlags2);
            }
        }
    }
}

#define P g_save.players[g_curPlayer]

#define E g_enemies[pl][j]

#define C g_mapObjs[i]

// Per-frame update for the current player (g_curPlayer): left/right movement and banking
// (including mirrored-ship handling), rocket firing (locking onto a random eligible alien,
// weighted so bosses/turrets get more lock chances), regular/autofire shot firing, the
// scoop and shield power-up timers, and death/respawn handling per game mode.
// P/E/C below are local macros: P = the current player's save data, E = enemy `j` on side
// `pl`, C = map-object `i` in the rocket-lock array.
void UpdatePlayer()
{
    int moved;
    float speed;
    int k;
    int pl;
    int j;
    int AlienQue[500];
    int count;
    int i;

    moved = 0;
    pl = 0;
    j = 0;
    if (g_time > P.drunkModeTimer)
        P.drunkModeTimer = 0;

    // ---- horizontal movement (mirrored X when the mirror powerup is active) ----
    if (InputLeft(g_curPlayer)) {
        speed = (P.speed * g_joystickSpeedMul > 14.0) ? 14.0 : P.speed * g_joystickSpeedMul;
        if (!g_autoplay && P.drunkModeTimer > 0)
            speed = 0 - speed;
        moved = 1;
        P.bank = P.bank - g_tiltStep;
        if (P.bank < 0.0)
            P.bank = 0;

        if (P.mirrorTime != 0) {
            P.mirrorX = P.mirrorX - speed * g_frameDt;
            if (P.mirrorX < 64.0)
                P.mirrorX = 64;
            if (P.mirrorX > g_screenW - 104)
                P.mirrorX = g_screenW - 104;
            P.x = P.mirrorX;
        } else {
            P.x = P.x - speed * g_frameDt;
            if (P.x < 64.0)
                P.x = 64;
            if (P.x > g_screenW - 104)
                P.x = g_screenW - 104;
        }
    } else if (InputRight(g_curPlayer)) {
        speed = P.speed * g_joystickSpeedMul;
        speed = (P.speed * g_joystickSpeedMul > 14.0) ? 14.0 : P.speed * g_joystickSpeedMul;
        if (!g_autoplay && P.drunkModeTimer > 0)
            speed = 0 - speed;
        moved = 1;
        P.bank = P.bank + g_tiltStep;
        if (!(P.bank < 11.0))
            P.bank = 10;

        if (P.mirrorTime != 0) {
            P.mirrorX = P.mirrorX + speed * g_frameDt;
            if (P.mirrorX > g_screenW - 104)
                P.mirrorX = g_screenW - 104;
            if (P.mirrorX < 64.0)
                P.mirrorX = 64;
            P.x = P.mirrorX;
        } else {
            P.x = P.x + speed * g_frameDt;
            if (P.x > g_screenW - 104)
                P.x = g_screenW - 104;
            if (P.x < 64.0)
                P.x = 64;
        }
    }

    if (InputRocket(g_curPlayer)) {
        if (g_state != STATE_SHOP && g_state != STATE_BONUS_RACE && g_state != STATE_MEMORY_STATION &&
            g_state != STATE_SHOP_GATE && g_state != STATE_METEOR_STORM && P.dead == 0 &&
            P.keyLatchRocket != 0 && P.rockets > 0) {
            P.keyLatchRocket = 0;

            // ---- find a free rocket slot and pick a target enemy for it ----
            for (i = 0; i < MAX_MAP_OBJS; i++) {
                if (C.active == 0) {
                    pl = g_curPlayer;
                    if (g_gameMode == MODE_DUAL)
                        pl = 0;
                    count = 0;
                    for (j = 0; j < 500; j++)
                        AlienQue[j] = 0;
                    for (j = 0; j < MAX_ENEMIES; j++) {
                        if (E.active != 0 && E.attackStaggerTimer < 1.0 && E.type != ENEMY_CAPTURED &&
                            !E.locked && E.type != ENEMY_DEBRIS) {
                            switch (E.type) {
                            case ENEMY_WRAPPER:
                            case ENEMY_MOTHERSHIP:
                            case ENEMY_MONEY_SUCKER:
                            case ENEMY_MONEY_SHIP:
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;

                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                break;

                            case ENEMY_BOSS:
                            case ENEMY_GUARD:
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;

                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                AlienQue[count] = j; count++;
                                break;

                            default:
                                AlienQue[count] = j; count++;
                            }
                        }
                    }
                    if (count < 1)
                        break;

                    // ---- pick the target and lock it (boss types stay unlocked) ----
                    j = AlienQue[RandRange(0, count)];
                    C.enemy = j;
                    E.locked = 1;
                    switch (E.type) {
                    case ENEMY_WRAPPER:
                    case ENEMY_MOTHERSHIP:
                    case ENEMY_MONEY_SUCKER:
                    case ENEMY_MONEY_SHIP:
                        E.locked = 1;
                        break;
                    case ENEMY_BOSS:
                    case ENEMY_GUARD:
                        E.locked = 0;
                        break;
                    default:
                        E.locked = 1;
                    }

                    g_transitionLockUntil = g_time + 500;
                    g_transitionLock = 1;
                    g_isBossLevel = 1;
                    P.rockets = P.rockets - 1;
                    P.rocketsFired = P.rocketsFired + 1;

                    // ---- spawn the seeking-rocket entity ----
                    C.active = 1;
                    C.x = P.x + 9.0f;
                    C.y = P.y - 8.0f;
                    C.row = 0;
                    C.animDelay = RandRange(0, 3) + 4;
                    C.animCnt = RandRange(0, 3) + 3;
                    C.turnDelay = 1;
                    C.turnCnt = C.turnDelay;
                    C.unusedC = 1;
                    C.unusedD = C.unusedC;

                    C.life = 300;
                    C.player = g_curPlayer;
                    C.state = 200;
                    C.laser = 0;
                    C.dmg = 200;
                    C.frame = 1;
                    C.speed = 10;
                    g_mapObjs[i].gfx = g_gfxRocket;
                    g_mapObjs[i].drawnA = g_hmaRocket;
                    g_mapObjs[i].drawnB = g_rocketGfxW;
                    g_mapObjs[i].drawnC = g_rocketGfxH;
                    C.unusedA = 0;
                    C.unusedB = 0;
                    C.w = 24;
                    C.stepY = 24;
                    SoundPlay(g_sfxRocket, 32000, 255, g_panTable[ClampX((int)C.x)], 127, g_sndFlags);
                    break;
                }
            }
        }
    } else {
        P.keyLatchRocket = 1;
    }

    // ---- fire: manual shot, mirrored double shot, autofire, or race-mode boost charge ----
    if (InputFire(g_curPlayer) && g_transitionLock == 0 && g_state != STATE_SHOP && g_state != STATE_SHOP_GATE &&
        g_state != STATE_METEOR_STORM && g_state != STATE_MALFUNCTION_DEATH && g_bonusResultsTime == 0) {
        if (g_state == STATE_BONUS_RACE || g_state == STATE_GEM_DROP) {
            if (g_time > g_raceStartTime && g_boostCharge < g_chargeMax)
                g_boostCharge = g_chargeRate * g_frameDt + g_boostCharge;
        } else {
            if (P.keyLatchFire != 0) {
                if (P.dead == 0 && P.lives > g_shipDefs[P.ship]->minEnergy &&
                    g_state != STATE_BONUS_RACE) {
                    if (P.mirrorTime == 0)
                        FirePlayer(P.bullets, 1);
                    if (P.mirrorTime != 0) {
                        FirePlayer(100, 0);
                        P.x = (g_screenW - 40) - P.x;
                        FirePlayer(100, 0);
                        P.x = (g_screenW - 40) - P.x;
                    }
                }
                P.keyLatchFire = 0;
            }

            if (P.autofire != 0 && P.dead == 0 &&
                P.lives > g_shipDefs[P.ship]->minEnergy &&
                g_state != STATE_BONUS_RACE && g_time > P.autofireTimer) {
                if (P.mirrorTime == 0)
                    FirePlayer(P.bullets, 1);
                if (P.mirrorTime != 0) {
                    FirePlayer(100, 0);
                    P.x = (g_screenW - 40) - P.x;
                    FirePlayer(100, 0);
                    P.x = (g_screenW - 40) - P.x;
                }
                P.autofireTimer = g_time + P.autofireInterval;
            }
        }
    } else {
        if (g_state == STATE_BONUS_RACE && g_time > g_raceStartTime) {
            g_boostReleased = 1;
            if (g_boostCharge > 0.0)
                g_boostCharge = g_boostCharge - g_dischargeRate * g_frameDt;
            if (g_boostCharge < 0.0)
                g_boostCharge = 0;
        }
        P.keyLatchFire = 1;
    }

    if (moved == 0) {
        if (P.bank > 5.0)
            P.bank = P.bank - g_tiltStep;
        if (P.bank < 5.0)
            P.bank = P.bank + g_tiltStep;
    }

    if (P.scoopTimer != 0) {
        if (g_time > P.scoopTimer) {
            P.scoopTimer = 0;
            g_chanScopeHum = SoundStop(g_chanScopeHum);
        }
        for (k = 0; k < MAX_SCOOP; k++) {
            if (g_scoop[k].spawnDelay == 0) {
                g_scoop[k].pos = g_scoop[k].pos - g_scoop[k].vel;
                if (g_scoop[k].pos <= 0.0) {
                    g_scoop[k].vel = RandFloat(0.2f, 1.3f);
                    g_scoop[k].pos = g_scoopRange;
                    g_scoop[k].unusedF0c = 0;
                    g_scoop[k].sx = 0;
                }
            } else {
                g_scoop[k].spawnDelay = g_scoop[k].spawnDelay - 1;
            }
        }
        if (g_scoopRange < 45.0)
            g_scoopRange = g_scoopRange + 1.0;
    }

    if (P.shieldTimer != 0) {
        if (g_time > P.shieldTimer) {
            P.shieldTimer = 0;
            g_chanShieldHum = SoundStop(g_chanShieldHum);
        }
        P.unusedInvulnBlinkTick = P.unusedInvulnBlinkTick - 1;
        if (P.unusedInvulnBlinkTick < 0) {
            P.unusedInvulnBlinkTick = 2;
            P.unusedInvulnBlinkFrame = P.unusedInvulnBlinkFrame - 1;
            if (P.unusedInvulnBlinkFrame < 0)
                P.unusedInvulnBlinkFrame = 7;
        }
    }

    // ---- death / respawn ----
    if (P.dead != 0 && g_time > P.respawnTime) {
        if (g_playerUpdateFn == StateDemo) {
            ResetToTitle();
        } else {
            switch (g_gameMode) {
            case MODE_SINGLE:
                P.x = (g_screenW >> 1) - 20;
                P.y = g_floorY;
                P.dead = 0;
                P.lives = P.lives - g_shipDefs[P.ship]->cost;
                PlayerHit(g_curPlayer);
                break;

            case MODE_TWO_PLAYER:
                P.dead = 0;
                P.lives = P.lives - g_shipDefs[P.ship]->cost;
                PlayerHit(g_curPlayer);
                if (!IsGameOver())
                    SwitchPlayer();
                P.x = (g_screenW >> 1) - 20;
                P.y = g_floorY;
                break;

            case MODE_DUAL:
                if (g_curPlayer == 0)
                    P.x = (g_screenW / 6) * 2 - 20;
                else
                    P.x = (g_screenW / 6) * 4 - 20;
                P.y = g_floorY;
                P.dead = 0;
                P.lives = P.lives - g_shipDefs[P.ship]->cost;
                PlayerHit(g_curPlayer);
                break;

            case MODE_TEAM:
                break;
            case MODE_UNUSED_4:
                break;
            case MODE_ACE_TOURNAMENT:
                break;

            case MODE_TIME_TRIAL:
                P.x = (g_screenW >> 1) - 20;
                P.y = g_floorY;
                P.dead = 0;
                P.lives = P.lives - g_shipDefs[P.ship]->cost;
                PlayerHit(g_curPlayer);
            }

            g_viewTransitionFlag = 2;
            g_stateFn = SetViewHud;
            EmptyViewChangeHook();
            g_drawBordersFn = DrawBorders;
            g_bonusWeight[27] = 10;
            g_bonusWeight[33] = 8;
            g_bonusWeight[26] = 25;
            g_bonusWeight[19] = 35;

            if (IsGameOver())
                ShowHiscoreTable();

            if (P.lives > g_shipDefs[P.ship]->minEnergy) {
                switch (g_gameMode) {
                case MODE_SINGLE:
                    P.shieldTimer = g_time + 3000;
                    break;
                case MODE_TWO_PLAYER:
                    P.shieldTimer = g_time + 5000;
                    break;

                case MODE_DUAL:
                    P.shieldTimer = g_time + 3000;
                    break;

                case MODE_TEAM:
                    P.shieldTimer = g_time + 3000;
                    break;

                case MODE_UNUSED_4:
                    P.shieldTimer = g_time + 5000;
                    break;

                case MODE_ACE_TOURNAMENT:
                    P.shieldTimer = g_time + 5000;
                    break;

                case MODE_TIME_TRIAL:
                    P.shieldTimer = g_time + 1500;
                }
                g_chanShieldHum = SoundPlayChannel(g_chanShieldHum, g_sfxShieldHum, -1, 180, 0.0f, 223);
            }
        }
    }
}

#undef P
#undef E
#undef C

// Updates both players for this frame: in versus mode only the turn-taking player moves,
// otherwise player 2 then player 1 both update.
void UpdatePlayers()
{
    if (g_state == STATE_BONUS_RACE) {
        if (g_vsTurnPlayer == 0) {
            g_curPlayer = 0;
            UpdatePlayer();
        } else {
            g_curPlayer = 1;
            UpdatePlayer();
        }
    } else {
        g_curPlayer = 1;
        UpdatePlayer();
        g_curPlayer = 0;
        UpdatePlayer();
    }
}

// Attract-mode game state: drives the current player with simple left/right/fire AI
// (g_demoSteer) instead of real input, and returns to the title screen on any key press.
// NOTE: near-duplicate of UpdatePlayer's movement/fire/respawn logic, written out with
// full g_save.players[g_curPlayer] expressions instead of the P macro.
void StateDemo()
{
    int moved = 0;
    float spd;
    int i;

    sprintf(g_alertMsg, "D E M O");
    g_msgColor = 2;
    g_msgTimer = g_time + 5000;
    if (KeyDown(K_VK_SPACE) == true || KeyDown(K_VK_ESCAPE) == true ||
        InputFire(0) || InputFire(1)) {
        ResetToTitle();
        g_inputCooldown = 100;
    } else {
        if (g_time > g_save.players[g_curPlayer].drunkModeTimer)
            g_save.players[g_curPlayer].drunkModeTimer = 0;
        if (RandRange(0, 500) < 10)
            g_demoSteer = RandRange(0, 100);
        if (g_save.players[g_curPlayer].x < 130.0)
            g_demoSteer = 90;
        if (g_save.players[g_curPlayer].x > g_screenW - 130)
            g_demoSteer = 10;

        // ---- horizontal movement (mirrors UpdatePlayer) ----
        if (g_demoSteer < 33) {
            spd = g_save.players[g_curPlayer].speed;
            if (g_save.players[g_curPlayer].drunkModeTimer > 0)
                spd = 0 - spd;
            moved = 1;
            g_save.players[g_curPlayer].bank = g_save.players[g_curPlayer].bank - g_tiltStep;
            if (g_save.players[g_curPlayer].bank < 0.0)
                g_save.players[g_curPlayer].bank = 0;
            if (g_save.players[g_curPlayer].mirrorTime) {
                g_save.players[g_curPlayer].mirrorX = g_save.players[g_curPlayer].mirrorX - spd * g_frameDt;
                if (g_save.players[g_curPlayer].mirrorX < 64.0)
                    g_save.players[g_curPlayer].mirrorX = 64;
                if (g_save.players[g_curPlayer].mirrorX > g_screenW - 104)
                    g_save.players[g_curPlayer].mirrorX = g_screenW - 104;
                g_save.players[g_curPlayer].x = g_save.players[g_curPlayer].mirrorX;
            } else {
                g_save.players[g_curPlayer].x = g_save.players[g_curPlayer].x - spd * g_frameDt;
                if (g_save.players[g_curPlayer].x < 64.0)
                    g_save.players[g_curPlayer].x = 64;
                if (g_save.players[g_curPlayer].x > g_screenW - 104)
                    g_save.players[g_curPlayer].x = g_screenW - 104;
            }
        }

        if (g_demoSteer > 66) {
            spd = g_save.players[g_curPlayer].speed;
            if (g_save.players[g_curPlayer].drunkModeTimer > 0)
                spd = 0 - spd;
            moved = 1;
            g_save.players[g_curPlayer].bank = g_save.players[g_curPlayer].bank + g_tiltStep;
            if (!(g_save.players[g_curPlayer].bank < 11.0))
                g_save.players[g_curPlayer].bank = 10;
            if (g_save.players[g_curPlayer].mirrorTime) {
                g_save.players[g_curPlayer].mirrorX = g_save.players[g_curPlayer].mirrorX + spd * g_frameDt;
                if (g_save.players[g_curPlayer].mirrorX > g_screenW - 104)
                    g_save.players[g_curPlayer].mirrorX = g_screenW - 104;
                if (g_save.players[g_curPlayer].x < 64.0)
                    g_save.players[g_curPlayer].x = 64;
                g_save.players[g_curPlayer].x = g_save.players[g_curPlayer].mirrorX;
            } else {
                g_save.players[g_curPlayer].x = g_save.players[g_curPlayer].x + spd * g_frameDt;
                if (g_save.players[g_curPlayer].x > g_screenW - 104)
                    g_save.players[g_curPlayer].x = g_screenW - 104;
                if (g_save.players[g_curPlayer].x < 64.0)
                    g_save.players[g_curPlayer].x = 64;
            }
        }

        // ---- fire: manual shot, mirrored double shot, or race-mode boost charge ----
        if (RandRange(0, 100) < 3) {
            if (g_state == STATE_BONUS_RACE && g_time > g_raceStartTime && g_boostCharge < g_chargeMax)
                g_boostCharge = g_boostCharge + g_chargeRate * g_frameDt;
            if (g_save.players[g_curPlayer].keyLatchFire) {
                if (g_save.players[g_curPlayer].dead == 0
                    && g_save.players[g_curPlayer].lives >
                        g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy
                    && g_state != STATE_BONUS_RACE) {
                    if (g_save.players[g_curPlayer].mirrorTime == 0)
                        FirePlayer(g_save.players[g_curPlayer].bullets, 1);
                    if (g_save.players[g_curPlayer].mirrorTime != 0) {
                        FirePlayer(100, 0);
                        g_save.players[g_curPlayer].x = (g_screenW - 40) - g_save.players[g_curPlayer].x;
                        FirePlayer(100, 0);
                        g_save.players[g_curPlayer].x = (g_screenW - 40) - g_save.players[g_curPlayer].x;
                    }
                }
                g_save.players[g_curPlayer].keyLatchFire = 0;
            }

            if (g_save.players[g_curPlayer].autofire
                && g_save.players[g_curPlayer].dead == 0
                && g_save.players[g_curPlayer].lives > g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy
                && g_state != STATE_BONUS_RACE
                && g_time > g_save.players[g_curPlayer].autofireTimer) {
                if (g_save.players[g_curPlayer].mirrorTime == 0)
                    FirePlayer(g_save.players[g_curPlayer].bullets, 1);
                if (g_save.players[g_curPlayer].mirrorTime != 0) {
                    FirePlayer(100, 0);
                    g_save.players[g_curPlayer].x = (g_screenW - 40) - g_save.players[g_curPlayer].x;
                    FirePlayer(100, 0);
                    g_save.players[g_curPlayer].x = (g_screenW - 40) - g_save.players[g_curPlayer].x;
                }
                g_save.players[g_curPlayer].autofireTimer =
                    g_time + g_save.players[g_curPlayer].autofireInterval;
            }
        } else {
            if (g_state == STATE_BONUS_RACE && g_time > g_raceStartTime) {
                if (g_boostCharge > 0.0)
                    g_boostCharge = g_boostCharge - g_dischargeRate * g_frameDt;
                if (g_boostCharge < 0.0)
                    g_boostCharge = 0;
            }
            g_save.players[g_curPlayer].keyLatchFire = 1;
        }

        if (moved == 0) {
            if (g_save.players[g_curPlayer].bank > 5.0)
                g_save.players[g_curPlayer].bank = g_save.players[g_curPlayer].bank - g_tiltStep;
            if (g_save.players[g_curPlayer].bank < 5.0)
                g_save.players[g_curPlayer].bank = g_save.players[g_curPlayer].bank + g_tiltStep;
        }

        // ---- scoop powerup timer ----
        if (g_save.players[g_curPlayer].scoopTimer) {
            if (g_time > g_save.players[g_curPlayer].scoopTimer) {
                g_save.players[g_curPlayer].scoopTimer = 0;
                g_chanScopeHum = SoundStop(g_chanScopeHum);
            }
            for (i = 0; i < MAX_SCOOP; i++) {
                if (g_scoop[i].spawnDelay == 0) {
                    g_scoop[i].pos = g_scoop[i].pos - g_scoop[i].vel;
                    if (g_scoop[i].pos <= 0.0) {
                        g_scoop[i].vel = RandFloat(0.2f, 1.3f);
                        g_scoop[i].pos = g_scoopRange;
                        g_scoop[i].unusedF0c = 0;
                        g_scoop[i].sx = 0;
                    }
                } else {
                    g_scoop[i].spawnDelay = g_scoop[i].spawnDelay - 1;
                }
            }
            if (g_scoopRange < 45.0)
                g_scoopRange = g_scoopRange + 1.0;
        }

        // ---- shield powerup timer ----
        if (g_save.players[g_curPlayer].shieldTimer) {
            if (g_time > g_save.players[g_curPlayer].shieldTimer) {
                g_save.players[g_curPlayer].shieldTimer = 0;
                g_chanShieldHum = SoundStop(g_chanShieldHum);
            }
            g_save.players[g_curPlayer].unusedInvulnBlinkTick =
                g_save.players[g_curPlayer].unusedInvulnBlinkTick - 1;
            if (g_save.players[g_curPlayer].unusedInvulnBlinkTick < 0) {
                g_save.players[g_curPlayer].unusedInvulnBlinkTick = 2;
                g_save.players[g_curPlayer].unusedInvulnBlinkFrame =
                    g_save.players[g_curPlayer].unusedInvulnBlinkFrame - 1;
                if (g_save.players[g_curPlayer].unusedInvulnBlinkFrame < 0)
                    g_save.players[g_curPlayer].unusedInvulnBlinkFrame = 7;
            }
        }

        // ---- death / respawn ----
        if (g_save.players[g_curPlayer].dead && g_time > g_save.players[g_curPlayer].respawnTime) {
            if (g_playerUpdateFn == StateDemo) {
                ResetToTitle();
            } else {
                switch (g_gameMode) {
                case MODE_SINGLE:
                    g_save.players[g_curPlayer].x = g_screenW / 2 - 20;
                    g_save.players[g_curPlayer].y = g_floorY;
                    g_save.players[g_curPlayer].dead = 0;
                    g_save.players[g_curPlayer].lives = g_save.players[g_curPlayer].lives -
                        g_shipDefs[g_save.players[g_curPlayer].ship]->cost;
                    PlayerHit(g_curPlayer);
                    break;

                case MODE_TWO_PLAYER:
                    g_save.players[g_curPlayer].dead = 0;
                    g_save.players[g_curPlayer].lives = g_save.players[g_curPlayer].lives -
                        g_shipDefs[g_save.players[g_curPlayer].ship]->cost;
                    PlayerHit(g_curPlayer);
                    if (IsGameOver() == 0)
                        SwitchPlayer();
                    g_save.players[g_curPlayer].x = g_screenW / 2 - 20;
                    g_save.players[g_curPlayer].y = g_floorY;
                    break;

                case MODE_DUAL:
                    if (g_curPlayer == 0)
                        g_save.players[g_curPlayer].x = g_screenW / 6 * 2 - 20;
                    else
                        g_save.players[g_curPlayer].x = g_screenW / 6 * 4 - 20;
                    g_save.players[g_curPlayer].y = g_floorY;
                    g_save.players[g_curPlayer].dead = 0;
                    g_save.players[g_curPlayer].lives = g_save.players[g_curPlayer].lives -
                        g_shipDefs[g_save.players[g_curPlayer].ship]->cost;
                    PlayerHit(g_curPlayer);
                    break;

                case MODE_TEAM:
                    break;
                case MODE_UNUSED_4:
                    break;
                case MODE_ACE_TOURNAMENT:
                    break;

                case MODE_TIME_TRIAL:
                    g_save.players[g_curPlayer].x = g_screenW / 2 - 20;
                    g_save.players[g_curPlayer].y = g_floorY;
                    g_save.players[g_curPlayer].dead = 0;
                    g_save.players[g_curPlayer].lives = g_save.players[g_curPlayer].lives -
                        g_shipDefs[g_save.players[g_curPlayer].ship]->cost;
                    PlayerHit(g_curPlayer);
                    break;
                }

                g_viewTransitionFlag = 2;
                g_stateFn = SetViewHud;
                EmptyViewChangeHook();
                g_drawBordersFn = DrawBorders;
                g_bonusWeight[27] = 10;
                g_bonusWeight[33] = 8;
                g_bonusWeight[26] = 25;
                g_bonusWeight[19] = 35;

                if (IsGameOver())
                    ShowHiscoreTable();

                if (g_save.players[g_curPlayer].lives >
                    g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy) {
                    switch (g_gameMode) {
                    case MODE_SINGLE:
                        g_save.players[g_curPlayer].shieldTimer = g_time + 3000;
                        break;

                    case MODE_TWO_PLAYER:
                        g_save.players[g_curPlayer].shieldTimer = g_time + 5000;
                        break;

                    case MODE_DUAL:
                        g_save.players[g_curPlayer].shieldTimer = g_time + 3000;
                        break;

                    case MODE_TEAM:
                        g_save.players[g_curPlayer].shieldTimer = g_time + 3000;
                        break;

                    case MODE_UNUSED_4:
                        g_save.players[g_curPlayer].shieldTimer = g_time + 5000;
                        break;

                    case MODE_ACE_TOURNAMENT:
                        g_save.players[g_curPlayer].shieldTimer = g_time + 5000;
                        break;
                    case MODE_TIME_TRIAL:
                        g_save.players[g_curPlayer].shieldTimer = g_time + 1500;
                        break;
                    }
                    g_chanShieldHum = SoundPlayChannel(g_chanShieldHum, g_sfxShieldHum, -1, 180, 0, 223);
                }
            }
        }
    }
}

// Draws one power-up's countdown icon + depleting bar in the HUD stack (moving g_hudY up
// for the next one), if `timer` is running. Used for the plain single-icon timers below;
// the scoop and shield bars are more involved and stay inlined.
#define DRAW_TIMER_BAR(timer, iconSX, iconW, iconH) \
    if (g_save.players[g_curPlayer].timer != 0) { \
        left = g_save.players[g_curPlayer].timer - g_time; \
        g_hudY = g_hudY - 7; \
        Blit(g_hudY - 2, barY + 2, 0, g_gfxLogos, iconSX, 0xb6, iconW, iconH); \
        len = (int)((float)left / (g_save.players[g_curPlayer].buffDuration * 1000) * barH); \
        Blit(g_hudY, barY - len, 0, g_gfxLogos, 0x40, 0xb5 - len, 2, len + 1); \
    }

// Draws and updates the current player's ship-related effects for this frame: engine
// flame (particles or sprite-sheet), the scoop trail, the ship sprite itself, the four
// thruster flare bursts, tractor beams (spawning/fading/moving them, plus the Meteorstorm
// floor sink and hyperspace lift), and the shield glow plus every active power-up timer bar.
// `alt` selects the second (mirrored-copy) set of flame/scoop timers and positions.
void DrawPlayerShipFx(bool alt)
{
    Rect16 src;
    int frame;
    int i;
    int xp[6] = {0x200, 0x200, 0x200, 0x220, 0x240, 0x260};
    int yp[6] = {0, 0x20, 0x40, 0x40, 0x40, 0x40};
    int sx;
    int len;
    int left;
    float x;
    float y;
    int *P1_f;
    int *P2_f;
    int *P1_s;
    int *P2_s;
    unsigned int _FIRETIMER1;
    unsigned int _FIRETIMER2;
    unsigned int _SCOOPTIMER1;
    unsigned int _SCOOPTIMER2;
    int barY;
    float barH;
    int n1;
    int n2;

    int a0;
    int a1;
    int a2;
    int a3;
    int k;
    int pl;
    int j;
    int alpha;
    int m;
    int fade;
    int shieldLeft;

    // ---- HUD timer-bar stack position and per-player flame/scoop trail setup ----
    g_hudY = (int)g_save.players[g_curPlayer].x + 3;
    if (g_save.players[g_curPlayer].shieldL != 0 && g_state != STATE_BONUS_RACE && g_state != STATE_SHOP)
        g_hudY = g_hudY - 0x20;
    barY = (int)(g_save.players[g_curPlayer].y + 25.0);
    barH = 22.0f;

    if (g_save.players[g_curPlayer].dead == 0 &&
        g_save.players[g_curPlayer].lives > g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy) {
        x = g_save.players[g_curPlayer].x;
        y = g_save.players[g_curPlayer].y;
        if (g_curPlayer == 0) {
            if (!alt) {
                g_flameXP0 = (int)x + RandRange(0, 2) + 0x13;
                g_scoopXP0 = (int)g_save.players[g_curPlayer].x + 0x13;
                P1_f = &g_flameXP0;
                P1_s = &g_scoopXP0;
                _FIRETIMER1 = g_flameTimerP0;
                _SCOOPTIMER1 = g_scoopTimerP0;
            } else {
                g_flameXP0Alt = (int)x + RandRange(0, 2) + 0x13;
                g_scoopXP0Alt = (int)g_save.players[g_curPlayer].x + 0x13;
                P1_f = &g_flameXP0Alt;
                P1_s = &g_scoopXP0Alt;
                _FIRETIMER1 = g_flameTimerP0Alt;
                _SCOOPTIMER1 = g_scoopTimerP0Alt;
            }
        }

        if (g_curPlayer == 1) {
            if (!alt) {
                g_flameXP1 = (int)x + RandRange(0, 2) + 0x13;
                g_scoopXP1 = (int)g_save.players[g_curPlayer].x + 0x13;
                P2_f = &g_flameXP1;
                P2_s = &g_scoopXP1;
                _FIRETIMER2 = g_flameTimerP1;
                _SCOOPTIMER2 = g_scoopTimerP1;
            } else {
                g_flameXP1Alt = (int)x + RandRange(0, 2) + 0x13;
                g_scoopXP1Alt = (int)g_save.players[g_curPlayer].x + 0x13;
                P2_f = &g_flameXP1Alt;
                P2_s = &g_scoopXP1Alt;
                _FIRETIMER2 = g_flameTimerP1Alt;
                _SCOOPTIMER2 = g_scoopTimerP1Alt;
            }
        }

        // engine flame
        if (g_cfg.particlesOn == 0) {
            if (g_state != STATE_PAUSED) {
                g_save.players[g_curPlayer].flame = g_save.players[g_curPlayer].flame + 1.0f;
                if (g_save.players[g_curPlayer].flame >= 10.0)
                    g_save.players[g_curPlayer].flame = 0;
            }
            src.x1 = (int)g_save.players[g_curPlayer].flame * 16;
            src.y1 = 0;
            src.x2 = src.x1 + 16;
            src.y2 = src.y1 + 0x19;

            // NOTE: while(1){...break;} instead of a plain block, kept as in the original.
            while (1) {
                QueueBlit(x + 12.0, y + 21.0, g_gfxFighterFire2, &src);
                break;
            }
        } else if (g_state != STATE_PAUSED) {
            if (g_curPlayer == 0) {
                if (g_time > _FIRETIMER1) {
                    if (!alt)
                        g_flameTimerP0 = g_time + 2;
                    else
                        g_flameTimerP0Alt = g_time + 2;
                    for (n1 = 0; n1 < 5; n1++) {
                        AddParticle(g_gfxFlare12, g_flameXP0, (int)y + RandRange(0, 8) + 0x1a,
                                           RandFloat(20.0f, 28.0f), 0 - RandFloat(1.2f, 3.6f),
                                           RandFloat(0, 359.0f), RandFloat(0, 30.0f) - 15.0,
                                           RandRange(0xaa, 0xbe), 0xff, RandRange(0, 0x96),
                                           RandRange(0, 0x37), 0xff,
                                           RandFloat(1.0f, 2.0f) * 3.0, RandFloat(7.2f, 15.0f),
                                           -1, -0.01f, 0, P1_f, 0, 1);
                    }
                }

            } else {
                if (g_time > _FIRETIMER2) {
                    if (!alt)
                        g_flameTimerP1 = g_time + 2;
                    else
                        g_flameTimerP1Alt = g_time + 2;
                    for (n2 = 0; n2 < 5; n2++) {
                        AddParticle(g_gfxFlare26, g_flameXP1, (int)y + RandRange(0, 8) + 0x1a,
                                           RandFloat(20.0f, 28.0f), 0 - RandFloat(0.8f, 2.6f),
                                           RandFloat(0, 359.0f), RandFloat(0, 30.0f) - 15.0,
                                           RandRange(0xaa, 0xbe), RandRange(0, 100),
                                           RandRange(0, 0x96), 0xff, 0xff,
                                           RandFloat(1.0f, 2.0f) * 3.0, RandFloat(7.2f, 15.0f),
                                           -1, -0.01f, 0, P2_f, 0, 1);
                    }
                }
            }
        }

        // scoop powerup timer bar
        if (g_save.players[g_curPlayer].scoopTimer != 0) {
            if (g_cfg.particlesOn == 0) {
                left = g_save.players[g_curPlayer].scoopTimer - g_time;
                g_hudY = g_hudY - 7;
                Blit(g_hudY - 2, barY + 2, 0, g_gfxLogos, 0x60, 0xb6, 5, 5);
                len = (int)((float)left / (g_save.players[g_curPlayer].buffDuration * 1000) * barH);
                Blit(g_hudY, barY - len, 0, g_gfxLogos, 0x40, 0xb5 - len, 2, len + 1);

                for (i = 0; i < MAX_SCOOP; i++) {
                    if (!g_scoop[i].timer--) {
                        g_scoop[i].timer = RandRange(0, 2) + 1;
                        g_scoopTrailScale = 1.0f;
                        sx = g_scoop[i].sx;
                        Blit((int)x + 20 - (int)(g_scoop[i].pos * g_scoopTrailScale),
                                          (int)y - (int)(g_scoop[i].pos * g_scoopYScale),
                                          0, g_gfxLogos, sx, 11,
                                          (int)(g_scoop[i].pos * g_scoopTrailScale), 10);
                        Blit((int)x + 19,
                                          (int)y - (int)(g_scoop[i].pos * g_scoopYScale),
                                          0, g_gfxLogos,
                                          sx + (64 - (int)(g_scoop[i].pos * g_scoopTrailScale)) + 64, 11,
                                          (int)(g_scoop[i].pos * g_scoopTrailScale), 10);
                    }
                }
            } else {
                left = g_save.players[g_curPlayer].scoopTimer - g_time;
                g_hudY = g_hudY - 7;
                Blit(g_hudY - 2, barY + 2, 0, g_gfxLogos, 0x60, 0xb6, 5, 5);
                len = (int)((float)left / (g_save.players[g_curPlayer].buffDuration * 1000) * barH);
                Blit(g_hudY, barY - len, 0, g_gfxLogos, 0x40, 0xb5 - len, 2, len + 1);

                if (g_state != STATE_PAUSED) {
                    if (g_curPlayer == 0) {
                        if (g_time > _SCOOPTIMER1) {
                            if (!alt)
                                g_scoopTimerP0 = g_time + 5;
                            else
                                g_scoopTimerP0Alt = g_time + 5;
                            AddParticle(g_gfxFlareScoop, (int)x + 0x13, (int)y + RandRange(0, 8),
                                               5.0f, RandFloat(6.0f, 6.5f), 0, 0, 0,
                                               RandRange(0, 0xff), RandRange(100, 0xff), 0, 1000,
                                               RandFloat(2.7f, 3.0f) * 7.0, RandFloat(4.5f, 5.0f),
                                               -1, 0, 0, P1_s, 0, 0);
                        }

                    } else {
                        if (g_time > _SCOOPTIMER2) {
                            if (!alt)
                                g_scoopTimerP1 = g_time + 5;
                            else
                                g_scoopTimerP1Alt = g_time + 5;
                            AddParticle(g_gfxFlareScoop, (int)x + 0x13, (int)y + RandRange(0, 8),
                                               5.0f, RandFloat(6.0f, 6.5f), 0, 0, 0,
                                               0, RandRange(100, 0xff), RandRange(0, 0xff), 1000,
                                               RandFloat(2.7f, 3.0f) * 7.0, RandFloat(4.5f, 5.0f),
                                               -1, 0, 0, P2_s, 0, 0);
                        }
                    }
                }
            }
        }

        // ship sprite
        frame = (int)g_save.players[g_curPlayer].bank % 11;
        src.x1 = frame * 40;
        src.y1 = 0;
        src.x2 = src.x1 + 40;
        src.y2 = src.y1 + 0x1b;
        g_save.players[g_curPlayer].box = src;
        QueueBlit(x, y, g_save.players[g_curPlayer].gfx, &src);
        // NOTE: empty if-body (dead check), kept as in the original.
        if (g_save.players[g_curPlayer].hyperspaceOutTimer > 0.0) ;
        g_shipX = (int)x;
        g_shipY = (int)y;

        // thruster flares
        if (!g_flareCnt0--) {
            g_flareCnt0 = 300;
            g_flare0 = 1000;
        }
        if (!g_flareCnt1--) {
            g_flareCnt1 = 200;
            g_flare1 = 800;
        }
        if (!g_flareCnt2--) {
            g_flareCnt2 = 200;
            g_flare2 = 1500;
        }
        if (!g_flareCnt3--) {
            g_flareCnt3 = 300;
            g_flare3 = 1300;
        }

        if (g_curPlayer == 0 && g_state != STATE_PAUSED) {
            if (g_flare0 > 0) {
                if ((g_flare0 -= 100) < 0)
                    g_flare0 = 0;
                a0 = g_flare0;
                if (a0 > 0xff)
                    a0 = 0xff;
                QueueStretchRot(g_gfxFlare10,
                                       g_shipX + g_flareOfs[0][frame] - g_flareSize,
                                       g_shipY + 16 - g_flareSize,
                                       g_shipX + g_flareOfs[0][frame] + g_flareSize,
                                       g_shipY + 16 + g_flareSize,
                                       0xff, 0, 0, a0, 0, 1.0f);
                QueueStretchRot(g_gfxFlare10,
                                       g_shipX + g_flareOfs[1][frame] - g_flareSize,
                                       g_shipY + 16 - g_flareSize,
                                       g_shipX + g_flareOfs[1][frame] + g_flareSize,
                                       g_shipY + 16 + g_flareSize,
                                       0xff, 0, 0, a0, 0, 1.0f);
            }

            if (g_flare1 > 0) {
                if ((g_flare1 -= 200) < 0)
                    g_flare1 = 0;
                a1 = g_flare1;
                if (a1 > 0xff)
                    a1 = 0xff;
                QueueStretchRot(g_gfxFlare10,
                                       g_shipX + g_flareOfs[2][frame] - g_flareSize * 2,
                                       g_shipY + 20 - g_flareSize * 2,
                                       g_shipX + g_flareOfs[2][frame] + g_flareSize * 2,
                                       g_shipY + 20 + g_flareSize * 2,
                                       0, 0xff, 0, a1 >> 1, 0, 1.0f);
            }
        }

        if (g_curPlayer == 1 && g_state != STATE_PAUSED) {
            if (g_flare2 > 0) {
                if ((g_flare2 -= 100) < 0)
                    g_flare2 = 0;
                a2 = g_flare2;
                if (a2 > 0xff)
                    a2 = 0xff;
                QueueStretchRot(g_gfxFlare10,
                                       g_shipX + g_flareOfs[3][frame] - g_flareSize,
                                       g_shipY + g_flareOfs[7][frame] - g_flareSize,
                                       g_shipX + g_flareOfs[3][frame] + g_flareSize,
                                       g_shipY + g_flareOfs[7][frame] + g_flareSize,
                                       0xff, 0, 0, a2, 0, 1.0f);
                QueueStretchRot(g_gfxFlare10,
                                       g_shipX + g_flareOfs[4][frame] - g_flareSize,
                                       g_shipY + g_flareOfs[8][frame] - g_flareSize,
                                       g_shipX + g_flareOfs[4][frame] + g_flareSize,
                                       g_shipY + g_flareOfs[8][frame] + g_flareSize,
                                       0xff, 0, 0, a2, 0, 1.0f);
            }

            if (g_flare3 > 0) {
                if ((g_flare3 -= 200) < 0)
                    g_flare3 = 0;
                a3 = g_flare3;
                if (a3 > 0xff)
                    a3 = 0xff;
                QueueStretchRot(g_gfxFlare10,
                                       g_shipX + g_flareOfs[5][frame] - g_flareSize * 2,
                                       g_shipY + g_flareOfs[9][frame] - g_flareSize * 2,
                                       g_shipX + g_flareOfs[5][frame] + g_flareSize * 2,
                                       g_shipY + g_flareOfs[9][frame] + g_flareSize * 2,
                                       0xff, 0, 0x80, a3, 0, 1.0f);
                QueueStretchRot(g_gfxFlare10,
                                       g_shipX + g_flareOfs[6][frame] - g_flareSize * 2,
                                       g_shipY + g_flareOfs[10][frame] - g_flareSize * 2,
                                       g_shipX + g_flareOfs[6][frame] + g_flareSize * 2,
                                       g_shipY + g_flareOfs[10][frame] + g_flareSize * 2,
                                       0xff, 0, 0x80, a3, 0, 1.0f);
            }
        }

        // tractor beams
        if (g_beamLevel > 0 && g_state != STATE_MALFUNCTION) {
            for (k = 0; k < MAX_BEAMS; k++) {
                if (g_beams[k].active != 0) {
                    if ((int)g_beams[k].yvel == 0) {
                        QueueStretchF(g_gfxFlareSpark,
                                             x - g_beams[k].xoff,
                                             y - 90.0,
                                             x + 40.0 + g_beams[k].xoff,
                                             y + 50.0,
                                             g_beams[k].r, g_beams[k].g, g_beams[k].b,
                                             g_beams[k].alpha, 0);
                        if (g_state != STATE_PAUSED)
                            g_beams[k].alpha = g_beams[k].alpha - g_beams[k].fade * g_frameDt;
                        if (g_beams[k].alpha < 0.0)
                            g_beams[k].active = 0;

                    } else {
                        QueueStretchF(g_gfxFlare24,
                                             x - g_beams[k].xoff,
                                             y - 70.0 + g_beams[k].yoff,
                                             x + 40.0 + g_beams[k].xoff,
                                             y + 27.0 + 30.0 + g_beams[k].yoff,
                                             g_beams[k].r, g_beams[k].g, g_beams[k].b,
                                             g_beams[k].alpha, 0);
                        if (g_state != STATE_PAUSED) {
                            g_beams[k].yoff = g_beams[k].yoff + g_beams[k].yvel * g_frameDt;
                            g_beams[k].alpha = g_beams[k].alpha - g_beams[k].fade * g_frameDt;
                            if (g_beams[k].alpha < 0.0)
                                g_beams[k].active = 0;
                            g_beams[k].xoff = g_beams[k].xoff + g_beams[k].xvel * g_frameDt;
                            if ((int)g_beams[k].yoff + y > g_screenH)
                                g_beams[k].active = 0;
                        }
                    }
                }
            }
        }

        // ---- occasionally spawn a new tractor beam toward a scoopable target ----
        pl = g_curPlayer;
        if (g_gameMode == MODE_DUAL)
            pl = 0;
        if (RandRange(0, 100) < g_beamLevel && g_state != STATE_MALFUNCTION && g_state != STATE_PAUSED) {
            for (j = 0; j < MAX_BEAMS; j++) {
                if (g_beams[j].active == 0) {
                    g_beams[j].active = 1;
                    if (Rand7f() < 15) {
                        g_beams[j].xoff = RandRange(0x50, 0x82);
                        g_beams[j].xvel = 0;
                        g_beams[j].r = Randff();
                        g_beams[j].g = Randff();
                        g_beams[j].b = Randff();
                        g_beams[j].yoff = 0;
                        g_beams[j].yvel = 0;
                        g_beams[j].alpha = 255.0f;
                        g_beams[j].fade = RandRange(5, 15);

                    } else {
                        if (g_save.players[pl].hyperspaceOutTimer > 0.0) {
                            g_beams[j].xoff = RandRange(15, 50);
                            g_beams[j].alpha = 60.0f;
                        } else {
                            g_beams[j].xoff = RandRange(1, 30);
                            g_beams[j].alpha = 50.0f;
                        }
                        g_beams[j].xvel = RandRange(2, 6);
                        g_beams[j].r = RandRange(100, 0xff);
                        g_beams[j].g = RandRange(100, 0xff);
                        g_beams[j].b = RandRange(100, 0xff);
                        g_beams[j].yoff = 0;
                        g_beams[j].yvel = RandRange(2, 5);
                        g_beams[j].fade = RandRange(1, 8);
                    }
                }
            }
        }

        // ---- warp-malfunction: sink every player toward the floor ----
        if (g_state == STATE_MALFUNCTION && g_state != STATE_PAUSED) {
            g_save.players[0].y = g_save.players[0].y + g_frameDt * 1.2f;
            g_save.players[1].y = g_save.players[1].y + g_frameDt * 1.2f;
            g_save.players[2].y = g_save.players[2].y + g_frameDt * 1.2f;
            g_save.players[3].y = g_save.players[3].y + g_frameDt * 1.2f;

            if (g_save.players[0].y > g_floorY)
                g_save.players[0].y = g_floorY;
            if (g_save.players[1].y > g_floorY)
                g_save.players[1].y = g_floorY;
            if (g_save.players[2].y > g_floorY)
                g_save.players[2].y = g_floorY;
            if (g_save.players[3].y > g_floorY)
                g_save.players[3].y = g_floorY;
        }

        // ---- hyperspace transition: push players toward/away from the floor ----
        g_beamLevel = 0;
        if (g_save.players[pl].hyperspaceOutTimer > 0.0 && g_state != STATE_PAUSED) {
            g_beamLevel = 0x3c;
            g_save.players[0].y = g_save.players[0].y - g_frameDt * 0.5f;
            g_save.players[1].y = g_save.players[1].y - g_frameDt * 0.5f;
            g_save.players[2].y = g_save.players[2].y - g_frameDt * 0.5f;
            g_save.players[3].y = g_save.players[3].y - g_frameDt * 0.5f;
            if (g_save.players[0].y < g_floorY - 40)
                g_save.players[0].y = g_floorY - 40;
            if (g_save.players[1].y < g_floorY - 40)
                g_save.players[1].y = g_floorY - 40;
            if (g_save.players[2].y < g_floorY - 40)
                g_save.players[2].y = g_floorY - 40;
            if (g_save.players[3].y < g_floorY - 40)
                g_save.players[3].y = g_floorY - 40;
        }

        if (g_save.players[pl].hyperspaceInTimer > 0.0 && g_state != STATE_PAUSED) {
            g_beamLevel = (int)(g_save.players[pl].hyperspaceMidTimer / 4.0);
            g_save.players[0].y = g_save.players[0].y + g_frameDt * 0.85f;
            g_save.players[1].y = g_save.players[1].y + g_frameDt * 0.85f;
            g_save.players[2].y = g_save.players[2].y + g_frameDt * 0.85f;
            g_save.players[3].y = g_save.players[3].y + g_frameDt * 0.85f;
            if (g_save.players[0].y > g_floorY)
                g_save.players[0].y = g_floorY;
            if (g_save.players[1].y > g_floorY)
                g_save.players[1].y = g_floorY;
            if (g_save.players[2].y > g_floorY)
                g_save.players[2].y = g_floorY;
            if (g_save.players[3].y > g_floorY)
                g_save.players[3].y = g_floorY;
        }

        if (g_save.players[pl].hyperspaceMidTimer > 0.0 && g_state != STATE_PAUSED)
            g_beamLevel = (int)(g_save.players[pl].hyperspaceMidTimer / 3.0);

        if (g_beamLevel > 10 && g_state != STATE_MALFUNCTION && g_state != STATE_PAUSED) {
            alpha = 0xfa;
            for (m = 0; m < 15; m++) {
                if (g_curPlayer == 0) {
                    QueueStretchF(g_beamSparkGfxP0,
                                         x - (RandRange(0, 8) - m),
                                         y - (RandRange(0, 8) + m * 8),
                                         x + (RandRange(0, 8) + m + 40),
                                         y + (RandRange(0, 8) + m * 8 + 27),
                                         Randff(), 0, 0, alpha, 0);
                }
                if (g_curPlayer == 1) {
                    QueueStretchF(g_beamSparkGfxP1,
                                         x - (RandRange(0, 8) - m),
                                         y - (RandRange(0, 8) + m * 8),
                                         x + (RandRange(0, 8) + m + 40),
                                         y + (RandRange(0, 8) + m * 8 + 27),
                                         0, Rand7f(), Randff(), alpha, 0);
                }
                if ((alpha -= 15) < 0)
                    alpha = 0;
            }
        }

        // shield
        if (g_save.players[g_curPlayer].shieldTimer != 0) {
            x = g_save.players[g_curPlayer].x;
            y = g_save.players[g_curPlayer].y;
            fade = 0;
            shieldLeft = g_save.players[g_curPlayer].shieldTimer - g_time;
            g_hudY = g_hudY - 7;
            Blit(g_hudY - 2, barY + 2, 0, g_gfxLogos, 0x50, 0xb6, 6, 6);
            len = (int)((float)shieldLeft / (g_save.players[g_curPlayer].buffDuration * 1000) * barH);
            Blit(g_hudY, barY - len, 0, g_gfxLogos, 0x40, 0xb5 - len, 2, len + 1);
            if (shieldLeft < 5000)
                fade = 0xff - (int)(shieldLeft / 19.7);
            QueueStretchRot(g_gfxShieldNew, x - 6.0, y - 10.0, x - 6.0 + 50.0, y - 10.0 + 50.0, 0xff,
                                   g_save.players[g_curPlayer].shieldGlowG - fade < 0 ?
                                       0 : g_save.players[g_curPlayer].shieldGlowG - fade,
                                   g_save.players[g_curPlayer].shieldGlowB - fade < 0 ?
                                       0 : g_save.players[g_curPlayer].shieldGlowB - fade,
                                   0xff, 0, g_shieldAngle);

            if (g_state != STATE_PAUSED) {
                g_save.players[g_curPlayer].shieldGlowG =
                    g_save.players[g_curPlayer].shieldGlowG + g_save.players[g_curPlayer].shieldGlowGStep;
                g_shieldAngle = g_shieldAngle + 2.5;
                if (g_shieldAngle >= 360.0)
                    g_shieldAngle = g_shieldAngle - 360.0;
                if (g_save.players[g_curPlayer].shieldGlowG > 0xff) {
                    g_save.players[g_curPlayer].shieldGlowG = 0xff;
                    g_save.players[g_curPlayer].shieldGlowGStep =
                        -g_save.players[g_curPlayer].shieldGlowGStep;
                }
                if (g_save.players[g_curPlayer].shieldGlowG < 0) {
                    g_save.players[g_curPlayer].shieldGlowG = 0;
                    g_save.players[g_curPlayer].shieldGlowGStep =
                        -g_save.players[g_curPlayer].shieldGlowGStep;
                }

                g_save.players[g_curPlayer].shieldGlowB =
                    g_save.players[g_curPlayer].shieldGlowB + g_save.players[g_curPlayer].shieldGlowBStep;
                if (g_save.players[g_curPlayer].shieldGlowB > 0xff) {
                    g_save.players[g_curPlayer].shieldGlowB = 0xff;
                    g_save.players[g_curPlayer].shieldGlowBStep =
                        -g_save.players[g_curPlayer].shieldGlowBStep;
                }
                if (g_save.players[g_curPlayer].shieldGlowB < 0) {
                    g_save.players[g_curPlayer].shieldGlowB = 0;
                    g_save.players[g_curPlayer].shieldGlowBStep =
                        -g_save.players[g_curPlayer].shieldGlowBStep;
                }
            }
        }

        // other powerup timer bars
        DRAW_TIMER_BAR(scoreMult2Timer, 0x30, 6, 5)
        DRAW_TIMER_BAR(scoreMult5Timer, 0x40, 6, 5)
        DRAW_TIMER_BAR(drunkModeTimer, 0x20, 5, 8)
        DRAW_TIMER_BAR(mirrorTime, 0x70, 6, 5)
    }
    if (g_playerUpdateFn == StateDemo) {
        DrawMenuPrompt();
    }
}
#undef DRAW_TIMER_BAR

#define SHIP_HUD_CUR() \
    if (g_save.players[g_curPlayer].mirrorTime != 0) { \
        if (g_time > g_save.players[g_curPlayer].mirrorTime) { \
            g_save.players[g_curPlayer].x = g_save.players[g_curPlayer].mirrorX; \
            g_save.players[g_curPlayer].mirrorTime = 0; \
            g_save.players[g_curPlayer].writeOnlyMirrorFlag = 0; \
        } else { \
            DrawPlayerShipFx(false); \
            g_save.players[g_curPlayer].x = (float)(g_screenW - 40) - g_save.players[g_curPlayer].x; \
            DrawPlayerShipFx(true); \
            g_save.players[g_curPlayer].x = (float)(g_screenW - 40) - g_save.players[g_curPlayer].x; \
        } \
    } else { \
        DrawPlayerShipFx(false); \
    }

// Draws the current player's ship HUD/FX, drawing both halves of a mirrored ship.
void ShipHud()
{
    SHIP_HUD_CUR()
}

// Draws ship HUD/FX for whichever player(s) are active this frame (turn-based in versus
// races, both players otherwise).
void ShipHudAll()
{
    if (g_state == STATE_BONUS_RACE) {
        if (g_vsTurnPlayer == 0) {
            g_curPlayer = 0;
            DrawPlayerShipFx(false);
        } else {
            g_curPlayer = 1;
            SHIP_HUD_CUR()
        }
    } else {
        g_curPlayer = 1;
        SHIP_HUD_CUR()
        g_curPlayer = 0;
        SHIP_HUD_CUR()
    }
}

#undef SHIP_HUD_CUR

// Sets player p's start position and default ship graphics/hitmask for its slot (0 or 1); in 2-player mode
// the two ships are placed at 1/3 and 2/3 of the screen width.
void PlacePlayer(int p)
{
    if (g_gameMode == MODE_DUAL) {
        if (p == 0)
            g_save.players[p].x = (float)(g_screenW / 6 * 2 - 20);
        else
            g_save.players[p].x = (float)(g_screenW / 6 * 4 - 20);
    } else {
        g_save.players[p].x = (float)(g_screenW / 2 - 20);
    }

    g_save.players[p].y = (float)g_floorY;
    g_save.players[p].bank = 5.0f;
    g_save.players[p].writeOnlyF1c = 1.0f;
    g_save.players[p].speed = g_speedBase;

    if (p == 0) {
        g_save.players[p].gfx = g_gfxFighter1;
        g_save.players[p].hitMask = g_ship1Hma;
        g_save.players[p].hitMaskParamA = g_ship1GfxParamA;
        g_save.players[p].hitMaskParamB = g_ship1GfxParamB;
    } else {
        g_save.players[p].gfx = g_gfxFighter2;
        g_save.players[p].hitMask = g_ship2Hma;
        g_save.players[p].hitMaskParamA = g_ship2GfxParamA;
        g_save.players[p].hitMaskParamB = g_ship2GfxParamB;
    }
}

// Starting values for a fresh player save-state (formerly a shared table of scrambled constants).
enum {
    STARTING_BULLETS            = 8,
    STARTING_MEMORY_BONUS       = 25000,
    STARTING_MONEY_MAX          = 99990,
    STARTING_BONUS_ROUND_POINTS = 10000
};

// Resets player p's save-state to the start-of-game defaults: position, score/lives/weapon, timers,
// secrets, hyperspace state, and every per-run counter. Called at the start of a new game or life.
void InitPlayer(int p)
{
    int i;
    int j;

    g_scoreMul[p] = 1;
    g_save.players[p].inputDevice = (&g_cfg.device0)[p];
    PlacePlayer(p);
    if (g_playerUpdateFn == StateDemo) {
        g_save.players[p].level = RandRange(1, 8);
    } else {
        g_save.players[p].level = 1;
    }

    // Score, lives, money.
    g_save.players[p].score = 0;
    g_save.players[p].energy = 0;
    g_save.players[p].bullets = STARTING_BULLETS;
    g_save.players[p].lives =
        g_shipDefs[g_save.players[p].ship]->minEnergy + g_shipDefs[g_save.players[p].ship]->extraLives;
    g_save.players[p].deaths = 0;
    g_save.players[p].unusedScoreThreshold = 0;
    g_save.players[p].extraLetterE = 0;
    g_save.players[p].extraLetterX = 0;
    g_save.players[p].extraLetterT = 0;
    g_save.players[p].extraLetterR = 0;
    g_save.players[p].extraLetterA = 0;
    g_save.players[p].money = 0;
    g_save.players[p].rank = RANK_ENSIGN;
    g_save.players[p].bestRank = RANK_ENSIGN;
    g_save.players[p].bonusRoundCount = 0.0f;
    g_save.players[p].hits = 0;
    g_save.players[p].shots = 0;
    g_save.players[p].armour = g_shipDefs[g_save.players[p].ship]->baseArmour;

    // Weapon/autofire state.
    g_save.players[p].autofire = 0;
    g_save.players[p].autofireUnlocked = 0;
    g_save.players[p].turretTrackingReduction = 0;
    g_save.players[p].superAuto = 0;
    g_save.players[p].autofireInterval = 100;
    g_save.players[p].alienLock = 0;
    g_save.players[p].rockets = 0;
    g_save.players[p].marks = 0.0f;
    g_save.players[p].gemSeqB = -1;
    g_save.players[p].gemSeqA = -1;
    g_save.players[p].extraProgress = ' ';
    g_save.players[p].artxeProgress = ' ';
    g_save.players[p].bonusRoundScore = -1;
    g_save.players[p].bonusHighScore = -1;

    // Buff/effect timers.
    g_save.players[p].freezeTimer = 0;
    g_save.players[p].scoreMult2Timer = 0;
    g_save.players[p].scoreMult5Timer = 0;
    g_save.players[p].mirrorTime = 0;
    g_save.players[p].drunkModeTimer = 0;
    g_save.players[p].scoopTimer = 0;
    g_save.players[p].shieldTimer = 0;
    g_save.players[p].blueMoneyActive = 0;
    g_save.players[p].gemCounterCollected = 0;
    g_save.players[p].msMultiplierActive = 0;
    g_save.players[p].deaths = 0;
    if (g_playerUpdateFn == StateDemo) {
        g_save.players[p].weapon = RandRange(0, 3);
    } else {
        g_save.players[p].weapon = 0;
    }
    if (g_gameMode == MODE_TIME_TRIAL) {
        g_save.players[p].weapon = 9;
        g_save.players[p].bullets = 10;
    }

    // Shield/misc cosmetic state.
    g_save.players[p].buffDuration = 20;
    g_save.players[p].shieldL = 0;
    g_save.players[p].shieldR = 0;
    g_save.players[p].dead = 0;
    g_save.players[p].shieldHitFlashSpeed = 3.0f;
    g_save.players[p].enemyHpBonusRoll = 8;
    g_save.players[p].hurryupComboCount = 0;
    g_save.players[p].gems = g_shipDefs[g_save.players[p].ship]->gemBase;
    g_save.players[p].rows = 4;
    g_save.players[p].cols = 4;
    g_save.players[p].memoryGridUpgradeStreak = 0;
    g_save.players[p].memoryBonus = STARTING_MEMORY_BONUS;
    g_save.players[p].unused19c = 200;
    g_save.players[p].shieldGlowG = 100;
    g_save.players[p].shieldGlowB = 150;
    g_save.players[p].unused1a8 = -3;
    g_save.players[p].shieldGlowGStep = 27;
    g_save.players[p].shieldGlowBStep = 9;
    g_save.players[p].unusedF1b4_5 = 5;
    g_save.players[p].unusedF1b8_1 = 1;
    g_save.players[p].placeSlot = 0;
    g_save.players[p].flame = 0.0f;

    // Per-level run state.
    g_save.players[p].shipDestroyedThisLevel = 0;
    g_save.players[p].trackKillsFlag = 0;
    g_save.players[p].bonusRoundEnded = 0;
    g_save.players[p].levelEnemyDataCount = 0;
    g_save.players[p].unusedF1d4 = 0;
    g_save.players[p].unusedF1dc = 0;
    g_save.players[p].moneyMaxShown = 100;
    g_save.players[p].unusedF220 = 0;
    g_save.players[p].unusedF224 = 100;
    g_save.players[p].primaryEnemyCount = 0;
    g_save.players[p].secondaryEnemyCount = 0;
    g_save.players[p].totalEnemies = 0;
    g_save.players[p].spawnReserve = 0;
    g_save.players[p].killed = 0;
    g_save.players[p].escaped = 0;
    g_save.players[p].done = 0;
    g_save.players[p].doneTime = 0;
    g_save.players[p].chainBonusValue = 0;
    g_save.players[p].started = 0;

    // Starfield/hyperspace scroll state.
    g_save.players[p].starVelX = 0.0f;
    g_save.players[p].starSpeed = 5.0f;
    g_save.players[p].starVelZ = 0.0f;
    g_save.players[p].savedRaceStarSpeed = 0.0f;
    g_save.players[p].hyperspaceOutTimer = 0.0f;
    g_save.players[p].hyperspaceMidTimer = 0.0f;
    g_save.players[p].hyperspaceInTimer = 0.0f;
    g_save.players[p].hyperspaceInDuration = 100.0f;
    g_save.players[p].hyperspaceFade = 0.0f;
    g_save.players[p].savedHyperspaceFade = 0.0f;
    g_save.players[p].savedHyperspaceInDuration = 100.0f;
    g_save.players[p].savedHyperspaceInTimer = 0.0f;
    g_save.players[p].savedHyperspaceOutTimer = 0.0f;
    g_save.players[p].savedStarSpeed = 0.0f;
    g_save.players[p].savedStarVelZ = 0.0f;
    g_save.players[p].savedScrollSpeedY = 0.0f;
    g_save.players[p].savedHyperspaceMidTimer = 0.0f;
    g_save.players[p].scrollSpeedY = 0.0f;
    g_save.players[p].moneyMax = STARTING_MONEY_MAX;

    // Level transition / bonus-tally / "perfect" text state.
    g_save.players[p].levelFinished = 0;
    g_save.players[p].levelTransitioning = 0;
    g_save.players[p].levelWarpPending = 0;
    g_save.players[p].bonusTally = 0;
    g_save.players[p].bonusTallyTick = 4;
    g_save.players[p].bonusTallyDelay = 1.0f;
    g_save.players[p].perfectTextSpacing = 20.0f;
    g_save.players[p].perfectTextSpacingVel = 0.3f;
    g_save.players[p].perfectDone = 0;
    g_save.players[p].bonusResultsInitDone = 0;
    g_save.players[p].bonusKilled = 0;
    g_save.players[p].gemCounterPicks = 0;
    g_save.players[p].gemCounterUnlocked = 0;
    g_save.players[p].highScoreMilestone = 0;
    g_save.players[p].blueMoneyPicks = 0;
    g_save.players[p].blueMoneyUnlocked = 0;
    g_save.players[p].multiplierPicks = 0;
    g_save.players[p].multiplierUnlocked = 0;
    g_save.players[p].weaponFloorAtOne = 0;

    for (i = 0; i < 50; i++) {
        g_save.players[p].secretFlags[i] = 0;
    }
    for (j = 0; j < 50; j++) {
        g_save.players[p].secretSeen[j] = 0;
    }
    g_save.players[p].secretCount = 0;

    // Streaks, secret bird mini-game, and remaining per-run counters.
    g_save.players[p].bonusRoundPoints = STARTING_BONUS_ROUND_POINTS;
    g_save.players[p].drunkStreak = 0;
    g_save.players[p].perfectStreak = 0;
    g_save.players[p].color = 0;
    g_save.players[p].secretBirdHits = 0;
    g_save.players[p].levelMilestoneHandled = 0;
    g_save.players[p].secretBirdCounter = 20;
    g_save.players[p].secretBirdTick = 3;
    g_save.players[p].raceDistance = g_bonusThresholdBase;
    g_save.players[p].gameSpeedSetting = g_cfg.fps;
    g_save.players[p].maxRankReached = RANK_ENSIGN;
    g_save.players[p].bulletSpeedMult = 1.0f;
    g_save.players[p].bulkLevelsCooldown = 0;
    g_save.players[p].nextShotSnd = 0;
    if (g_profileIndex != -1) {
        g_save.players[p].sessionPlayTime = GetAccountTime(g_profileIndex);
    } else {
        g_save.players[p].sessionPlayTime = 0;
    }
}
