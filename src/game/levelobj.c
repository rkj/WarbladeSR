// levelobj.c: Level objects (mines, turrets, hazards): update and drawing.
#include "globals.h"
#include "game.h"


// Per-frame update for the 100-slot level object pool (mines/turrets/hazards placed in a
// level). Behaviour branches on `type`: type 9 objects count down a fuse and explode, or
// (while armed) turn to track the current player and fly toward them; type 14/15 objects
// cycle an animation row and occasionally spawn debris particles; type 18 objects fade out
// over time; anything else just drifts by its velocity and deactivates off the bottom of
// the screen.
void UpdateLevelObjects()
{
    float ex;
    float ey;
    int i;
    int pl;
    unsigned int dir;
    int want;
    int d1;
    int d2;
    // Indexed by a 4-bit quadrant code (bit0 = target left, bit1 = target right,
    // bit2 = target below, bit3 = target above) built below; gives the facing frame
    // (of 32) that points roughly at the player for the valid diagonal combinations.
    int quadtab[11] = {0, 0, 0, 0, 0, 21, 13, 0, 0, 29, 5};
    float ox;
    float oy;
    float px;
    float py;
    float dx;
    float dy;
    float dist;

    for (i = 0; i < MAX_LEVEL_OBJS; i++) {
        if (g_levelObj[i].active != 0) {
            g_isWaveLevel = 1;
            if (g_save.players[g_curPlayer].hyperspaceFade > 0.0) {
                g_levelObj[i].y += g_save.players[g_curPlayer].scrollSpeedY * g_frameDt;
            } else if (g_levelObj[i].type == LOBJ_ROCKET) {
                g_levelObj[i].fuse -= 1.0f * g_frameDt;

                if (g_levelObj[i].fuse < 0.0) {
                    ex = g_levelObj[i].x;
                    ey = g_levelObj[i].y;
                    g_levelObj[i].active = 0;
                    SoundPlay(g_sfxExplo1, RandRange(38000, 44100), g_rampB[ClampY((int)ey)],
                                      g_panTable[ClampX((int)ex)], RandRange(216, 190), g_sndFlags);

                    SpawnExplosion(ex, ey, 24, 24, 150, 0, 1, 50, 100, 255, 50, 100, 255);
                    AddParticle(g_gfxFlare4, (int)ex, (int)ey, 16.0f, RandFloat(0.0f, 5.0f) + 30.0,
                                         RandFloat(0.0f, 359.0f), 0.0f, 0, 0, RandRange(0, 100) + 128,
                                         255, 600, 10.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);
                } else {
                    if (g_state != STATE_PAUSED) {
                        if (!g_levelObj[i].frameTimer--) {
                            g_levelObj[i].row++;
                            if (g_levelObj[i].row > 2)
                                g_levelObj[i].row = 0;
                            g_levelObj[i].frameTimer = g_levelObj[i].frameDelay;
                        }
                    }

                    // Turn to track the player: turret aims by random chance, mine faces the
                    // player's quadrant otherwise.
                    pl = g_levelObj[i].player;
                    g_levelObj[i].turnTimer -= 1.0f * g_frameDt;
                    if (g_levelObj[i].turnTimer < 0.0) {
                        g_levelObj[i].turnTimer = g_levelObj[i].turnDelay;
                        ox = g_levelObj[i].x;
                        oy = g_levelObj[i].y;
                        if (g_save.players[pl].lives > g_shipDefs[g_save.players[pl].ship]->minEnergy &&
                            g_save.players[pl].dead == 0) {
                            px = g_save.players[pl].x;
                            py = g_save.players[pl].y;
                            if (RandRange(0, 101 - g_save.players[pl].turretTrackingReduction + 1) <
                                (int)g_diffTurretTrackChance) {
                                if (RandRange(0, 100) < 33)
                                    g_levelObj[i].frame--;
                                if (RandRange(0, 100) > 66)
                                    g_levelObj[i].frame++;
                                if (g_levelObj[i].frame < 1)
                                    g_levelObj[i].frame = 32;
                                if (g_levelObj[i].frame > 32)
                                    g_levelObj[i].frame = 1;

                            } else {
                                TURN_TOWARD_QUADRANT(g_levelObj[i].frame, ox, oy, px, py)
                            }
                        }
                    }

                    // Play a proximity-based sound as it closes in on the player.
                    if (g_save.players[pl].lives > g_shipDefs[g_save.players[pl].ship]->minEnergy &&
                        g_save.players[pl].dead == 0) {
                        g_levelObj[i].soundTimer -= 1.0f * g_frameDt;
                        ox = g_levelObj[i].x;
                        oy = g_levelObj[i].y;
                        pl = g_levelObj[i].player;
                        px = g_save.players[pl].x;
                        py = g_save.players[pl].y;
                        dx = ox - px;
                        if (dx < 0.0)
                            dx = 0 - dx;
                        dx = dx * dx;
                        dy = oy - py;
                        if (dy < 0.0)
                            dy = 0 - dy;
                        dy = dy * dy;
                        dist = Sqrt(dx + dy) / 8.0 + 2.0;
                        if (g_levelObj[i].soundTimer < 0.0) {
                            g_levelObj[i].soundTimer = dist;
                            dist = ((500.0 - Sqrt(dx + dy) < 0.0) ? 0.0 : 500.0 - Sqrt(dx + dy)) / 3.0;
                            SoundPlay(g_sfxPing2, -1, (int)dist,
                                              g_panTable[ClampX((int)g_levelObj[i].x)], 127, g_sndFlags);
                        }
                    }

                    g_levelObj[i].x += g_dirVecX[g_levelObj[i].frame] * g_levelObj[i].speed * g_frameDt;
                    g_levelObj[i].y += g_dirVecY[g_levelObj[i].frame] * g_levelObj[i].speed * g_frameDt;
                }
            } else {

                if (g_levelObj[i].type == LOBJ_BOSS_BURST && g_state != STATE_PAUSED) {
                    g_levelObj[i].frameTimer -= 1.0f * g_frameDt;
                    if (g_levelObj[i].frameTimer < 0.0) {
                        g_levelObj[i].frameTimer = g_levelObj[i].frameDelay;
                        g_levelObj[i].row++;
                        if (g_levelObj[i].row > 5)
                            g_levelObj[i].row = 0;
                    }
                    if (Rand7f() < 64 && g_cfg.bulletIntensity == BULLETS_FLARE_FX && g_state != STATE_PAUSED) {
                        AddParticle(g_gfxObjExplode, (int)g_levelObj[i].x + 16, (int)g_levelObj[i].y + 16,
                                             RandFloat(40.0f, 96.0f), 0 - RandFloat(0.2f, 3.0f),
                                             RandFloat(0.0f, 359.0f), RandFloat(0.0f, 59.0f),
                                             RandRange(0, 359), Rand7f() + 128, Randff(), 0, 200,
                                             RandFloat(5.2f, 15.5f), 1.0f, 10, 0.0f, 0, 0, 0, 1);
                    }
                }

                if (g_levelObj[i].type == LOBJ_BOSS_SHOT && g_state != STATE_PAUSED) {
                    g_levelObj[i].frameTimer -= 1.0f * g_frameDt;
                    if (g_levelObj[i].frameTimer < 0.0) {
                        g_levelObj[i].frameTimer = g_levelObj[i].frameDelay;
                        g_levelObj[i].row++;
                        if (g_levelObj[i].row > 3)
                            g_levelObj[i].row = 0;
                    }
                    if (Rand7f() < 64 && g_cfg.bulletIntensity == BULLETS_FLARE_FX && g_state != STATE_PAUSED) {
                        AddParticle(g_gfxObjExplode, (int)g_levelObj[i].x + 16, (int)g_levelObj[i].y + 16,
                                             RandFloat(10.0f, 30.0f), 0 - RandFloat(0.2f, 3.0f),
                                             RandFloat(0.0f, 359.0f), RandFloat(0.0f, 59.0f),
                                             RandRange(0, 359), Rand7f() + 128, Randff(), 0, 300,
                                             RandFloat(5.2f, 15.5f), 1.0f, 10, 0.0f, 0, 0, 0, 1);
                    }
                }

                if (g_levelObj[i].type == LOBJ_BEAM && g_state != STATE_PAUSED) {
                    g_levelObj[i].f28 -= 1.0f * g_frameDt;
                    if (g_levelObj[i].f28 < 1.0)
                        g_levelObj[i].active = 0;
                } else {
                    if (g_state != STATE_PAUSED) {
                        g_levelObj[i].turnDelay -= 1.0f * g_frameDt;
                        if (g_levelObj[i].turnDelay < 0.0) {
                            g_levelObj[i].turnDelay = g_defaultObjAlpha;
                            g_levelObj[i].f28 -= 1.0;
                            if (g_levelObj[i].f28 < 0.0)
                                g_levelObj[i].f28 += 2.0;
                        }
                    }
                    g_levelObj[i].x += g_levelObj[i].vx;
                    g_levelObj[i].y += g_levelObj[i].vy;
                    if (g_levelObj[i].y > g_screenH)
                        g_levelObj[i].active = 0;
                }
            }
        }
    }
}

// Draws all active level objects (normal/unlit variant), one clipped QueueBlit per
// object with the source rect picked by `type` (and, for some types, the animation
// `row`/`frame`). Type 18 draws an additive streak instead of the sprite when particles
// are enabled.
void DrawLevelObjectsNormal()
{
    // Per-row (0..5) tile offsets within the object's 3x2 frame sheet, for type 14.
    int top_p[6] = {0, 32, 64, 0, 32, 64};
    int left_p[6] = {0, 0, 0, 32, 32, 32};
    Rect16 src;
    int i;
    int x;
    int y;
    int w;
    int h;
    int px;
    int py;
    int r;
    float fx;
    float fy;
    int off;

    r = RandRange(0, 32);
    for (i = 0; i < MAX_LEVEL_OBJS; i++) {
        if (g_levelObj[i].active == 1) {
            // ---- LOBJ_ROCKET / LOBJ_ROCKET_B: homing mine/rocket, exhaust trail ----
            if (g_levelObj[i].type == LOBJ_ROCKET || g_levelObj[i].type == LOBJ_ROCKET_B) {
                if (g_cfg.particlesOn != 0 && g_state != STATE_PAUSED) {
                    if (RandRange(0, (int)(1.0f * (1.0f / g_frameDt))) == 0) {
                        fx = g_levelObj[i].x + 12.0;
                        fx += 0 - g_dirVecX[g_levelObj[i].frame] * 6.0;
                        fy = g_levelObj[i].y + 12.0;
                        fy += 0 - g_dirVecY[g_levelObj[i].frame] * 6.0;
                        AddParticle(g_gfxFlare27, (int)fx, (int)fy, RandFloat(11.0f, 15.0f),
                                             0 - RandFloat(0.8f, 1.6f), RandFloat(0.0f, 359.0f),
                                             0.0f, 0, 0, 255, 0, 255,
                                             RandFloat(1.0f, 2.0f) * 25.0, 0.0f, -1, -0.01f,
                                             0, 0, 0, 1);
                    }
                }

                while (1) {
                    x = (int)g_levelObj[i].x;
                    y = (int)g_levelObj[i].y;
                    w = 24;
                    h = 24;
                    src.x1 = (g_levelObj[i].frame - 1) * 24;
                    src.y1 = g_levelObj[i].row * 24;
                    g_levelObj[i].rect.x1 = src.x1;
                    g_levelObj[i].rect.y1 = src.y1;
                    g_levelObj[i].rect.x2 = src.x1 + w;
                    g_levelObj[i].rect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h))
                        break;
                    CLIP_SRC_RECT(x, y, w, h, src)

                    QueueBlit((float)x, (float)y, g_gfxRocket, &src);
                    break;
                }

            } else if (g_levelObj[i].type == LOBJ_BOSS_BURST) {
                // boss cannon charge-up burst
                while (1) {
                    x = (int)g_levelObj[i].x;
                    y = (int)g_levelObj[i].y;
                    w = 32;
                    h = 32;
                    src.x1 = 512;
                    src.y1 = 0;
                    g_levelObj[i].rect.x1 = src.x1;
                    g_levelObj[i].rect.y1 = src.y1;
                    g_levelObj[i].rect.x2 = src.x1 + w;
                    g_levelObj[i].rect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h))
                        break;
                    CLIP_SRC_RECT(x, y, w, h, src)

                    src.y1 += top_p[g_levelObj[i].row];
                    src.x1 += left_p[g_levelObj[i].row];
                    src.y2 += top_p[g_levelObj[i].row];
                    src.x2 += left_p[g_levelObj[i].row];
                    g_levelObj[i].rect.x1 += left_p[g_levelObj[i].row];
                    g_levelObj[i].rect.y1 += top_p[g_levelObj[i].row];
                    g_levelObj[i].rect.x2 += left_p[g_levelObj[i].row];
                    g_levelObj[i].rect.y2 += top_p[g_levelObj[i].row];

                    QueueBlit((float)x, (float)y, g_levelObj[i].gfxA, &src);
                    break;
                }

            } else if (g_levelObj[i].type == LOBJ_BOSS_SHOT) {
                // boss homing shot
                while (1) {
                    x = (int)g_levelObj[i].x;
                    y = (int)g_levelObj[i].y;
                    w = 32;
                    h = 32;
                    src.x1 = 0;
                    src.y1 = 64;
                    g_levelObj[i].rect.x1 = src.x1;
                    g_levelObj[i].rect.y1 = src.y1;
                    g_levelObj[i].rect.x2 = src.x1 + w;
                    g_levelObj[i].rect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h))
                        break;
                    CLIP_SRC_RECT(x, y, w, h, src)

                    off = g_levelObj[i].row * 32;
                    src.x1 += off;
                    src.x2 += off;
                    g_levelObj[i].rect.x1 += off;
                    g_levelObj[i].rect.x2 += off;

                    QueueBlit((float)x, (float)y, g_levelObj[i].gfxA, &src);
                    break;
                }

            } else if (g_levelObj[i].type == LOBJ_BEAM) {
                // boss beam weapon
                while (1) {
                    x = (int)g_levelObj[i].x;
                    y = (int)g_levelObj[i].y;
                    w = 64;
                    h = 70;
                    src.x1 = 0;
                    src.y1 = 0;
                    g_levelObj[i].rect.x1 = src.x1;
                    g_levelObj[i].rect.y1 = src.y1;
                    g_levelObj[i].rect.x2 = src.x1 + w;
                    g_levelObj[i].rect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h))
                        break;
                    CLIP_SRC_RECT(x, y, w, h, src)

                    if (g_cfg.particlesOn != 0) {
                        QueueStretchRot(g_gfxFlareStreakGuard,
                                               g_levelObj[i].x - 48.0 - r,
                                               g_levelObj[i].y,
                                               g_levelObj[i].x + 64.0 + 48.0 + r,
                                               g_levelObj[i].y + 70.0,
                                               200, 255, 200, 255 - r, 0, 0.0f);
                    } else {
                        QueueBlit((float)x, (float)y, g_levelObj[i].gfxA, &src);
                    }
                    break;
                }

            } else {
                while (1) {
                    x = (int)g_levelObj[i].x;
                    y = (int)g_levelObj[i].y;
                    w = 32;
                    h = 32;

                    if (g_levelObj[i].type == LOBJ_SHOT) {
                    // mine (LOBJ_SHOT) vs turret sprite offset
                        if (Rand7f() < 100 && g_cfg.bulletIntensity == BULLETS_FLARE_FX && g_state != STATE_PAUSED) {
                            px = (int)g_levelObj[i].x + g_levelObj[i].hitOffsetX +
                                 (g_levelObj[i].w >> 1);
                            py = (int)g_levelObj[i].y + g_levelObj[i].hitOffsetY +
                                 (g_levelObj[i].h >> 1);
                            AddParticle(g_gfxObjExplode, px, py,
                                                 RandFloat((float)(g_levelObj[i].h >> 1),
                                                              (float)(g_levelObj[i].h << 1)),
                                                 0 - RandFloat(0.01f, 2.0f), RandFloat(0.0f, 359.0f),
                                                 RandFloat(0.0f, 59.0f), RandRange(0, 359),
                                                 g_levelEnemyColorR, g_levelEnemyColorG,
                                                 g_levelEnemyColorB, 250,
                                                 RandFloat(5.2f, 10.5f), 0.0f, 20, 0.0f,
                                                 0, 0, 0, 1);
                        }
                        src.x1 = 480;

                    } else {
                        if (Rand7f() < 100 && g_cfg.bulletIntensity == BULLETS_FLARE_FX && g_state != STATE_PAUSED) {
                            px = (int)g_levelObj[i].x + g_levelObj[i].hitOffsetX +
                                 (g_levelObj[i].w >> 1);
                            py = (int)g_levelObj[i].y + g_levelObj[i].hitOffsetY +
                                 (g_levelObj[i].h >> 1);
                            AddParticle(g_gfxObjExplode, px, py,
                                                 RandFloat((float)(g_levelObj[i].h >> 1),
                                                              (float)(g_levelObj[i].h << 1)),
                                                 0 - RandFloat(0.01f, 2.0f), RandFloat(0.0f, 359.0f),
                                                 RandFloat(0.0f, 59.0f), RandRange(0, 359),
                                                 g_levelEnemyColorB, g_levelEnemyColorG,
                                                 g_levelEnemyColorR, 400,
                                                 RandFloat(5.2f, 15.5f), 0.0f, 10, 0.0f,
                                                 0, 0, 0, 1);
                        }
                        src.x1 = 448;
                    }

                    src.y1 = 0;
                    g_levelObj[i].rect.x1 = src.x1;
                    g_levelObj[i].rect.y1 = src.y1;
                    g_levelObj[i].rect.x2 = src.x1 + w;
                    g_levelObj[i].rect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h))
                        break;
                    CLIP_SRC_RECT(x, y, w, h, src)

                    if ((int)g_levelObj[i].f28 == 1) {
                        src.y1 += 32;
                        src.y2 += 32;
                    }
                    QueueBlit((float)x, (float)y, g_levelObj[i].gfxA, &src);
                    break;
                }
            }
        }
    }
}

// Draws all active level objects (bright/lit variant, e.g. for a flash effect). Same
// per-type layout as DrawLevelObjectsNormal(), but the default-case object blinks
// between two graphics (gfxA/gfxB) every other frame instead of drawing every frame.
void DrawLevelObjectsBright()
{
    // Per-row (0..5) tile offsets within the object's 3x2 frame sheet, for type 14.
    int top_p[6] = {0, 32, 64, 0, 32, 64};
    int left_p[6] = {0, 0, 0, 32, 32, 32};
    Rect16 src;
    int i;
    int x;
    int y;
    int w;
    int h;
    int r;
    float fx;
    float fy;
    int off;

    r = RandRange(0, 32);
    for (i = 0; i < MAX_LEVEL_OBJS; i++) {
        if (g_levelObj[i].active == 1) {
            if (g_levelObj[i].type == LOBJ_ROCKET) {
                if (g_cfg.particlesOn != 0 && g_state != STATE_PAUSED) {
                    if (RandRange(0, (int)(1.0f * (1.0f / g_frameDt))) == 0) {
                        fx = g_levelObj[i].x + 12.0;
                        fx += 0 - g_dirVecX[g_levelObj[i].frame] * 6.0;
                        fy = g_levelObj[i].y + 12.0;
                        fy += 0 - g_dirVecY[g_levelObj[i].frame] * 6.0;
                        AddParticle(g_gfxFlare27, (int)fx, (int)fy, RandFloat(11.0f, 15.0f),
                                             0 - RandFloat(0.8f, 1.6f), RandFloat(0.0f, 359.0f),
                                             0.0f, 0, 0, 255, 0, 255,
                                             RandFloat(1.0f, 2.0f) * 25.0, 0.0f, -1, -0.01f,
                                             0, 0, 0, 1);
                    }
                }

                while (1) {
                    x = (int)g_levelObj[i].x;
                    y = (int)g_levelObj[i].y;
                    w = 24;
                    h = 24;
                    src.x1 = (g_levelObj[i].frame - 1) * 24;
                    src.y1 = g_levelObj[i].row * 24;
                    g_levelObj[i].rect.x1 = src.x1;
                    g_levelObj[i].rect.y1 = src.y1;
                    g_levelObj[i].rect.x2 = src.x1 + w;
                    g_levelObj[i].rect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h))
                        break;
                    CLIP_SRC_RECT(x, y, w, h, src)

                    QueueBlit((float)x, (float)y, g_gfxRocket, &src);
                    break;
                }

            } else if (g_levelObj[i].type == LOBJ_BOSS_BURST) {
                // boss cannon charge-up burst
                while (1) {
                    x = (int)g_levelObj[i].x;
                    y = (int)g_levelObj[i].y;
                    w = 32;
                    h = 32;
                    src.x1 = 512;
                    src.y1 = 0;
                    g_levelObj[i].rect.x1 = src.x1;
                    g_levelObj[i].rect.y1 = src.y1;
                    g_levelObj[i].rect.x2 = src.x1 + w;
                    g_levelObj[i].rect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h))
                        break;
                    CLIP_SRC_RECT(x, y, w, h, src)

                    src.y1 += top_p[g_levelObj[i].row];
                    src.x1 += left_p[g_levelObj[i].row];
                    src.y2 += top_p[g_levelObj[i].row];
                    src.x2 += left_p[g_levelObj[i].row];
                    QueueBlit((float)x, (float)y, g_levelObj[i].gfxA, &src);
                    break;
                }

            } else if (g_levelObj[i].type == LOBJ_BOSS_SHOT) {
                // boss homing shot
                while (1) {
                    x = (int)g_levelObj[i].x;
                    y = (int)g_levelObj[i].y;
                    w = 32;
                    h = 32;
                    src.x1 = 0;
                    src.y1 = 64;
                    g_levelObj[i].rect.x1 = src.x1;
                    g_levelObj[i].rect.y1 = src.y1;
                    g_levelObj[i].rect.x2 = src.x1 + w;
                    g_levelObj[i].rect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h))
                        break;
                    CLIP_SRC_RECT(x, y, w, h, src)

                    off = g_levelObj[i].row * 32;
                    src.x1 += off;
                    src.x2 += off;
                    g_levelObj[i].rect.x1 += off;
                    g_levelObj[i].rect.x2 += off;
                    QueueBlit((float)x, (float)y, g_levelObj[i].gfxA, &src);
                    break;
                }

            } else if (g_levelObj[i].type == LOBJ_BEAM) {
                // boss beam weapon
                while (1) {
                    x = (int)g_levelObj[i].x;
                    y = (int)g_levelObj[i].y;
                    w = 64;
                    h = 70;
                    src.x1 = 0;
                    src.y1 = 0;
                    g_levelObj[i].rect.x1 = src.x1;
                    g_levelObj[i].rect.y1 = src.y1;
                    g_levelObj[i].rect.x2 = src.x1 + w;
                    g_levelObj[i].rect.y2 = src.y1 + h;

                    if (!CLIP_VISIBLE(x, y, w, h))
                        break;
                    CLIP_SRC_RECT(x, y, w, h, src)

                    if (g_cfg.particlesOn != 0) {
                        QueueStretchRot(g_gfxFlareStreakGuard,
                                               g_levelObj[i].x - 48.0 - r,
                                               g_levelObj[i].y,
                                               g_levelObj[i].x + 64.0 + 48.0 + r,
                                               g_levelObj[i].y + 70.0,
                                               200, 255, 200, 255 - r, 0, 0.0f);
                    } else {
                        QueueBlit((float)x, (float)y, g_levelObj[i].gfxA, &src);
                    }
                    break;
                }

            } else {
                g_levelObj[i].blink = g_levelObj[i].blink == 0;
                if (g_levelObj[i].blink != 0) {
                    while (1) {
                        x = (int)g_levelObj[i].x;
                        y = (int)g_levelObj[i].y;
                        w = 32;
                        h = 32;
                        if (g_levelObj[i].type == LOBJ_SHOT) {
                        // mine (LOBJ_SHOT) vs turret sprite offset
                            src.x1 = 480;
                        } else {
                            src.x1 = 448;
                        }
                        src.y1 = 0;
                        g_levelObj[i].rect.x1 = src.x1;
                        g_levelObj[i].rect.y1 = src.y1;
                        g_levelObj[i].rect.x2 = src.x1 + w;
                        g_levelObj[i].rect.y2 = src.y1 + h;

                        if (!CLIP_VISIBLE(x, y, w, h))
                            break;
                        CLIP_SRC_RECT(x, y, w, h, src)

                        if (g_levelObj[i].f28 != 0.0) {
                            src.y1 += 32;
                            src.y2 += 32;
                        }
                        g_levelObj[i].flip = g_levelObj[i].flip == 0;
                        if (g_levelObj[i].flip != 0) {
                            QueueBlit((float)x, (float)y, g_levelObj[i].gfxA, &src);
                        } else {
                            QueueBlit((float)x, (float)y, g_levelObj[i].gfxB, &src);
                        }
                        break;
                    }
                }
            }
        }
    }
}
