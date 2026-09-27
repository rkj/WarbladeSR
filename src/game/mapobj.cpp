// mapobj.cpp: Map objects (player shots, missiles, debris): spawning, update and drawing.
#include "globals.h"
#include "game.h"


// Spawns a piece of debris (a "map object") of `type` at (x, y) for `player`: awards
// `countsAsShot` energy/money (1 for a shot that counts toward stats, 0 for a mirrored
// side shot that doesn't), sets its velocity from `speed`, `push` and the per-type factor
// tables, and optionally chains a second/third linked debris piece (g_debrisLinkType1/2).
// Returns the slot index used, or -1 if no free slot was found.
int SpawnDebris(float x, float y, int player, int type, float dmg, int countsAsShot, float speed, float push)
{
    int i;
    int result;
    float t;
    float spd;
    float drag;

    // spd = max(2.0 - min(push / 20 * 2, 1.0), 0.0): less push-back the more the shot recoils.
    spd = (2.0 - (0 + push / 20.0 * 2 < 1.0 ? 1.0 : 0 + push / 20.0 * 2) < 0.0 ? 0.0
           : 2.0 - (0 + push / 20.0 * 2 < 1.0 ? 1.0 : 0 + push / 20.0 * 2));
    drag = g_debrisVySpeedFactor[type];
    if (drag != 0.0)
        drag = (0 - drag - 7.5 < 0.0 ? 0.0 : 0 - drag - 7.5) / 5.0;  // max(-drag - 7.5, 0.0) / 5
    else
        drag = 1.0f;
    result = -1;
    for (i = 0; i < MAX_MAP_OBJS; i++) {
        if (g_mapObjs[i].active == 0) {
            g_isBossLevel = 1;
            g_debrisSpawnedCount++;
            result = i;

            // award score/money for this piece
            g_save.players[player].energy += countsAsShot;
            g_sessionScore += countsAsShot;
            g_save.players[player].shots += countsAsShot;

            // initialise the map-object slot
            g_mapObjs[i].player = player;
            g_mapObjs[i].cost = countsAsShot;
            g_mapObjs[i].vy = speed * spd * g_debrisVySpeedFactor[type] - push * drag;
            g_mapObjs[i].vx = speed * spd * g_debrisVxFactor[type];
            g_mapObjs[i].x = x + 20 - g_objW[type] / 2.0f - g_debrisXOffset[type];
            g_mapObjs[i].y = y - g_debrisYOffset[type];
            g_mapObjs[i].w = g_objW[type];
            g_mapObjs[i].stepY = g_objH[type];
            g_mapObjs[i].laser = g_debrisLaserFlag[type];
            g_mapObjs[i].dmg = dmg;
            g_mapObjs[i].state = type;
            g_mapObjs[i].writeOnlyType = type;
            g_mapObjs[i].unusedFlag14 = 0;
            g_mapObjs[i].type = type;
            g_mapObjs[i].active = 1;
            g_mapObjs[i].animT = 1.0f;

            // randomised horizontal spread for some debris types
            if (g_debrisVxFactor[type] > 160.0 && g_debrisVxFactor[type] < 170.0) {
                t = g_debrisVxFactor[type] - 160.0;
                g_mapObjs[i].vx = RandFloat(0, t) - t / 2;
            } else if (g_debrisVxFactor[type] > 170.0) {
                t = g_debrisVxFactor[type] - 170.0;
                g_mapObjs[i].x += (RandFloat(0, t) - t / 2) * g_frameDt;
                g_mapObjs[i].vx = 0;
            }

            // trailing spark and chained (linked) debris pieces
            AddParticle(g_debrisParticleGfx, (int)g_mapObjs[i].x + g_mapObjs[i].w / 2,
                               (int)g_mapObjs[i].y + g_debrisYOffset[type],
                               g_debrisParticleSize[type] * RandFloat(1.0f, 2.0f), 0, 0, 0, 0,
                               g_debrisParticleColorR[type], g_debrisParticleColorG[type],
                               g_debrisParticleColorB[type], g_debrisParticleAlpha[type],
                               RandFloat(2.0f, 4.0f), 0, -1, 0, 0, 0, 0, 1);
            g_mapObjs[i].link1 = -1;
            g_mapObjs[i].link2 = -1;
            if (g_debrisLinkType1[type] != 0)
                g_mapObjs[i].link1 = SpawnDebris(x, y, player, g_debrisLinkType1[type], dmg, countsAsShot,
                                                  speed, push);
            if (g_debrisLinkType2[type] != 0)
                g_mapObjs[i].link2 = SpawnDebris(x, y, player, g_debrisLinkType2[type], dmg, countsAsShot,
                                                  speed, push);
            break;
        }
    }
    return result;
}

// Advances every active map object one frame: homing missiles (state 200) track and
// steer towards a locked enemy, explode on timeout; everything else (debris/weapon
// pickups/lasers) drifts, cycles its debris sprite, and deactivates when off-screen,
// refunding its energy cost to its owning player.
void UpdateMapObjects()
{
    int i;
    float x;
    float y;
    unsigned int dir;
    int want;
    int d1;
    int d2;
    // Maps a bitmask of which diagonal quadrant the target is in (bit0=west, bit1=east,
    // bit2=north, bit3=south; exactly one of bit0/bit1 and one of bit2/bit3 is set,
    // giving indices 5, 6, 9 or 10) to one of the 32 missile sprite frames. Other
    // indices are unreachable and left 0.
    int quadtab[11] = {0, 0, 0, 0, 0, 21, 13, 0, 0, 29, 5};
    float ox;
    float oy;
    float ex;
    float ey;
    int e;
    int target;
    int j;

    for (i = 0; i < MAX_MAP_OBJS; i++) {
        if (g_mapObjs[i].active != 0) {
            if (g_mapObjs[i].state == MAPOBJ_STATE_HOMING) {
                // Homing missile.
                g_mapObjs[i].life -= 1.0f * g_frameDt;
                if (g_mapObjs[i].life < 0.0) {
                    // Lifetime expired: explode and unlock the enemy it was chasing.
                    e = g_mapObjs[i].enemy;
                    g_enemies[g_curPlayer][e].locked = 0;
                    x = g_mapObjs[i].x;
                    y = g_mapObjs[i].y;
                    g_mapObjs[i].active = 0;

                    SoundPlay(g_sfxExplo1, RandRange(35000, 44100),
                                   g_rampB[ClampY((int)y)],
                                   g_panTable[ClampX((int)x)], 127, g_sndFlags);
                    SpawnExplosion(x, y, 24, 24, 150, 0, 1, 50, 100, 255, 50, 100, 255);
                    AddParticle(g_gfxFlare4, (int)x, (int)y, 16.0f,
                                         (float)(RandFloat(0.0f, 5.0f) + 30.0),
                                         RandFloat(0.0f, 359.0f), 0.0f, 0,
                                         RandRange(0, 50) + 128, 255,
                                         RandRange(0, 50) + 128, 600, 10.0f, 0.0f, -1,
                                         0.0f, 0, 0, 0, 1);
                } else {
                    // Still homing. If the current target is gone, pick the first
                    // eligible enemy (active, not mid-attack-stagger, not type 5/8,
                    // not already locked by another missile).
                    target = g_mapObjs[i].enemy;

                    if (g_enemies[g_curPlayer][target].active == 0) {
                        for (j = 0; j < 150; j++) {
                            if (g_enemies[g_curPlayer][j].active != 0 &&
                                g_enemies[g_curPlayer][j].attackStaggerTimer < 1.0 &&
                                g_enemies[g_curPlayer][j].type != ENEMY_CAPTURED &&
                                g_enemies[g_curPlayer][j].locked == 0 &&
                                g_enemies[g_curPlayer][j].type != ENEMY_DEBRIS) {
                                target = j;
                                break;
                            }
                        }
                    }

                    if (g_state != STATE_PAUSED) {
                        // Cycle the exhaust-flame animation row (0-2).
                        if (!(g_mapObjs[i].animCnt--)) {
                            g_mapObjs[i].row++;
                            if (g_mapObjs[i].row > 2)
                                g_mapObjs[i].row = 0;
                            g_mapObjs[i].animCnt = g_mapObjs[i].animDelay;
                        }
                    }

                    g_mapObjs[i].turnCnt -= 1.0f * g_frameDt;
                    if (g_mapObjs[i].turnCnt < 0.0) {
                        // Time to re-steer: pick the desired sprite frame from the
                        // target's quadrant, then turn towards it by one frame step
                        // (picking a random direction when it's exactly opposite).
                        g_mapObjs[i].turnCnt = g_mapObjs[i].turnDelay;
                        ox = g_mapObjs[i].x;
                        oy = g_mapObjs[i].y;

                        if (g_enemies[g_curPlayer][target].active != 0 &&
                            g_enemies[g_curPlayer][target].attackStaggerTimer < 1.0 &&
                            g_enemies[g_curPlayer][target].type != ENEMY_CAPTURED &&
                            g_enemies[g_curPlayer][target].type != ENEMY_DEBRIS) {
                            ex = g_enemies[g_curPlayer][target].x;
                            ey = g_enemies[g_curPlayer][target].y;
                            TURN_TOWARD_QUADRANT(g_mapObjs[i].frame, ox, oy, ex, ey)
                        }
                    }

                    // Move in the direction the current frame represents.
                    g_mapObjs[i].x += g_dirVecX[g_mapObjs[i].frame] * g_mapObjs[i].speed * g_frameDt;
                    g_mapObjs[i].y += g_dirVecY[g_mapObjs[i].frame] * g_mapObjs[i].speed * g_frameDt;
                }
            } else {
                // Debris / weapon-effect object (tumbling wreckage, laser beam segment, etc).
                if (g_state != STATE_PAUSED && g_mapObjs[i].laser == 0) {
                    // Non-laser: cycle to the next debris type once per second of animT.
                    g_mapObjs[i].animT -= 1.0f * g_frameDt;
                    if (g_mapObjs[i].animT < 0.0) {
                        g_mapObjs[i].animT = 1.0f;
                        g_mapObjs[i].type = g_debrisNextType[g_mapObjs[i].type];
                    }
                }

                if (g_state != STATE_PAUSED && g_mapObjs[i].laser != 0)
                    // Laser segments advance their sprite every frame instead.
                    g_mapObjs[i].type = g_debrisNextType[g_mapObjs[i].type];
                g_mapObjs[i].y += g_mapObjs[i].vy * g_frameDt;
                g_mapObjs[i].x += g_mapObjs[i].vx * g_frameDt;

                // Deactivate once it drifts off any edge of the play field.
                if ((int)g_mapObjs[i].y < -50) {
                    g_mapObjs[i].x = 0;
                    g_mapObjs[i].y = 0;
                    g_mapObjs[i].active = 0;
                }
                if ((int)g_mapObjs[i].x < -50) {
                    g_mapObjs[i].x = 0;
                    g_mapObjs[i].y = 0;
                    g_mapObjs[i].active = 0;
                }
                if ((int)g_mapObjs[i].x > (int)g_screenW + 50) {
                    g_mapObjs[i].x = 0;
                    g_mapObjs[i].y = 0;
                    g_mapObjs[i].active = 0;
                }

                if (g_mapObjs[i].type == -1) {
                    // Debris chain reached its end type: deactivate, and if this was a
                    // laser beam, deactivate its linked segments too and refund their cost.
                    g_mapObjs[i].x = 0;
                    g_mapObjs[i].y = 0;
                    g_mapObjs[i].active = 0;
                    if (g_mapObjs[i].laser != 0) {
                        if (g_mapObjs[i].link1 != -1) {
                            g_mapObjs[g_mapObjs[i].link1].active = 0;
                            g_save.players[g_mapObjs[g_mapObjs[i].link1].player].energy -=
                                g_mapObjs[g_mapObjs[i].link1].cost;
                            if (g_save.players[g_mapObjs[g_mapObjs[i].link1].player].energy < 0)
                                g_save.players[g_mapObjs[g_mapObjs[i].link1].player].energy = 0;
                        }
                        if (g_mapObjs[i].link2 != -1) {
                            g_mapObjs[g_mapObjs[i].link2].active = 0;
                            g_save.players[g_mapObjs[g_mapObjs[i].link2].player].energy -=
                                g_mapObjs[g_mapObjs[i].link2].cost;
                            if (g_save.players[g_mapObjs[g_mapObjs[i].link2].player].energy < 0)
                                g_save.players[g_mapObjs[g_mapObjs[i].link2].player].energy = 0;
                        }
                    }
                }

                if (g_mapObjs[i].active == 0) {
                    // This object itself was deactivated (any of the reasons above):
                    // refund its own energy cost too.
                    g_save.players[g_mapObjs[i].player].energy -= g_mapObjs[i].cost;
                    if (g_save.players[g_mapObjs[i].player].energy < 0)
                        g_save.players[g_mapObjs[i].player].energy = 0;
                }
            }
        }
    }
}

// Queues every active map object's sprite (and, for weapon effects, its flame overlay)
// for this frame's blit, clipped to the screen rect. Homing missiles also spawn an
// occasional exhaust particle.
void DrawMapObjects()
{
    KGraphic *graphic;
    Rect16 src;
    int i;
    int type;
    int x;
    int y;
    int w;
    int h;
    int off;
    int alpha;
    float fx;
    float fy;
    int r;   // NOTE: computed but never used; dead code kept for the byte match.

    r = RandRange(0, 20) + 2;
    for (i = 0; i < MAX_MAP_OBJS; i++) {
        if (g_mapObjs[i].active != 0) {
            if (g_mapObjs[i].state == MAPOBJ_STATE_HOMING) {
                // Homing missile: chance-per-frame exhaust particle behind the tail.
                if (g_cfg.particlesOn != 0 && g_state != STATE_PAUSED) {
                    if (RandRange(0, (int)(1.0f * (1.0f / g_frameDt))) == 0) {
                        fx = g_mapObjs[i].x + 12.0;
                        fx += 0 - g_dirVecX[g_mapObjs[i].frame] * 6.0;
                        fy = g_mapObjs[i].y + 12.0;
                        fy += 0 - g_dirVecY[g_mapObjs[i].frame] * 6.0;
                        AddParticle(g_gfxFlare19, (int)fx, (int)fy, RandFloat(14.0f, 20.0f),
                                             0 - RandFloat(0.8f, 1.6f), RandFloat(0.0f, 359.0f),
                                             0.0f, 0, 0, 255, 255, 255,
                                             RandFloat(1.0f, 2.0f) * 10.0, 0.0f, -1, -0.01f,
                                             0, 0, 0, 1);
                    }
                }

                // Missile sprite: 24x24 cell at (frame-1, row) in g_gfxRocket, clipped
                // to the screen. The `while(1)` is just an early-exit block (`break`
                // instead of nested `if`), not a real loop.
                while (1) {
                    x = (int)g_mapObjs[i].x;
                    y = (int)g_mapObjs[i].y;
                    w = 24;
                    h = 24;
                    src.x1 = (g_mapObjs[i].frame - 1) * 24;
                    src.y1 = g_mapObjs[i].row * 24;
                    g_mapObjs[i].srcRect.x1 = src.x1;
                    g_mapObjs[i].srcRect.y1 = src.y1;
                    g_mapObjs[i].srcRect.x2 = src.x1 + w;
                    g_mapObjs[i].srcRect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h))
                        break;
                    CLIP_SRC_RECT(x, y, w, h, src)
                    QueueBlit((float)x, (float)y, g_gfxRocket, &src);
                    break;
                }
            } else {
                type = g_mapObjs[i].type;

                if (g_debrisLaserFlag[type] == 0) {
                    // Single-sprite weapon effect/debris: blit once, clipped, plus an
                    // optional flame overlay stretched across its sprite rect.
                    x = (int)g_mapObjs[i].x;
                    y = (int)g_mapObjs[i].y;
                    w = g_objW[type];
                    h = g_objH[type];
                    src.x1 = g_objSrcX[type];
                    src.y1 = g_objSrcY[type];
                    g_mapObjs[i].srcRect.x1 = src.x1;
                    g_mapObjs[i].srcRect.y1 = src.y1;
                    g_mapObjs[i].srcRect.x2 = src.x1 + w;
                    g_mapObjs[i].srcRect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h)) {
                    } else {
                        CLIP_SRC_RECT(x, y, w, h, src)
                        QueueBlit((float)x, (float)y, g_gfxWeaponsBig, &src);
                        g_mapObjs[i].drawnA = (int)g_hmaWeaponsBig;
                        g_mapObjs[i].drawnB = g_gfxWeaponsBigW;
                        g_mapObjs[i].drawnC = g_gfxWeaponsBigH;
                    }

                    if (g_cfg.particlesOn != 0 && g_objFlameOn[type] != 0) {
                        // Flame overlay: pick the graphic by this type's flame-gfx id,
                        // then stretch+rotate it across the sprite's flame region.
                        float ang;
                        graphic = g_flashGfx;
                        if (g_objFlameGfx[type] == 1)
                            graphic = g_gfxFlareStreakBig;
                        if (g_objFlameGfx[type] == 2)
                            graphic = g_gfxFlareLaser;
                        ang = 0.0f - g_objFlameAngle[type];
                        if (ang < 0.0)
                            ang += 360.0;

                        QueueStretchRot(graphic,
                            // flame rect: top-left / bottom-right corners
                            g_mapObjs[i].x + ((g_objW[type] >> 1) - g_objFlameHalfW[type]),
                            g_mapObjs[i].y + ((g_objH[type] >> 1) - g_objFlameHalfH[type]
                                                                   + g_objFlameOffY[type]),
                            g_mapObjs[i].x + ((g_objW[type] >> 1) + g_objFlameHalfW[type]),
                            g_mapObjs[i].y + ((g_objH[type] >> 1) + g_objFlameHalfH[type]
                                                                   + g_objFlameOffY[type]),
                            // colour + alpha, rotation angle
                            (unsigned char)g_objR[type], (unsigned char)g_objG[type],
                            (unsigned char)g_objB[type], 255, 0, ang);
                    }
                } else {
                    // Laser beam: this map object represents a chain of segments, each
                    // one `stepY` above the last, drawn until one falls off-screen. Each
                    // segment also gets its own randomized flame overlay.
                    y = (int)g_mapObjs[i].y;
                    while (1) {
                        x = (int)g_mapObjs[i].x;
                        w = g_objW[type];
                        h = g_objH[type];

                        src.x1 = g_objSrcX[type];
                        src.y1 = g_objSrcY[type];
                        g_mapObjs[i].srcRect.x1 = src.x1;
                        g_mapObjs[i].srcRect.y1 = src.y1;
                        g_mapObjs[i].srcRect.x2 = src.x1 + w;
                        g_mapObjs[i].srcRect.y2 = src.y1 + h;

                        if (!CLIP_VISIBLE(x, y, w, h)) {
                            break;
                        } else {
                            CLIP_SRC_RECT(x, y, w, h, src)
                            QueueBlit((float)x, (float)y, g_gfxWeaponsBig, &src);
                            g_mapObjs[i].drawnA = (int)g_hmaWeaponsBig;
                            g_mapObjs[i].drawnB = g_gfxWeaponsBigW;
                            g_mapObjs[i].drawnC = g_gfxWeaponsBigH;
                            y = y - g_mapObjs[i].stepY;

                            if (g_cfg.particlesOn != 0 && g_objFlameOn[type] != 0) {
                                float ang2;
                                off = RandRange(0, g_objFlameHalfW[type]) - (g_objFlameHalfW[type] >> 1);
                                alpha = RandRange(128, 255);
                                graphic = g_gfxFlareLaser;
                                ang2 = 0.0f - g_objFlameAngle[type];
                                if (ang2 < 0.0)
                                    ang2 += 360.0;

                                QueueStretchRot(graphic,
                                    // flame rect: top-left / bottom-right corners
                                    g_mapObjs[i].x + ((g_objW[type] >> 1) - off),
                                    g_mapObjs[i].y + ((g_objH[type] >> 1) - g_objFlameHalfH[type]
                                                                           + g_objFlameOffY[type]),
                                    g_mapObjs[i].x + ((g_objW[type] >> 1) + off),
                                    g_mapObjs[i].y + ((g_objH[type] >> 1) + g_objFlameHalfH[type]
                                                                           + g_objFlameOffY[type]),
                                    // colour + alpha, rotation angle
                                    (unsigned char)g_objR[type], (unsigned char)g_objG[type],
                                    (unsigned char)g_objB[type], (unsigned char)alpha, 0, ang2);
                            }
                        }
                    }
                }
            }
        }
    }
}
