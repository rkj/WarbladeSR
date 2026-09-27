// explosions.cpp: Explosions: spawning (small to huge), update and drawing, explosion debris.
#include "globals.h"
#include "game.h"

enum {
    // g_explosions[].type: SpawnSmall's plain 13-frame explosion (own 32x32 sheet, g_explGfx2)
    // and the alpha-fade-out variant (no frame animation); other type values are indices into
    // g_explGfx's shared sheet (EXPLOSION_TYPE_ROW_H-tall rows) and run a 14-frame animation.
    EXPLOSION_TYPE_SMALL   = 10,
    EXPLOSION_TYPE_FADE    = -1,
    EXPLOSION_TYPE_ROW_H   = 45,
    EXPLOSION_SMALL_FRAMES = 12,   // last frame index of the type-10 sheet (13 frames, 0-12)
    EXPLOSION_LAST_FRAME   = 13    // last frame index of the shared sheet (14 frames, 0-13)
};


// Spawns a small, plain (type 10) explosion at (x, y) with no color/graphics customisation.
void SpawnSmall(float x, float y)
{
    int i;
    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        if (g_explosions[i].active == 0) {
            g_explosions[i].active = 1;
            g_explosions[i].x = x;
            g_explosions[i].y = y;
            g_explosions[i].type = EXPLOSION_TYPE_SMALL;
            g_explosions[i].frame = 0;
            g_explosions[i].delay = (float)RandRange(0, 2);
            g_explosions[i].timer = g_explosions[i].delay;
            g_explosions[i].spin = 0;
            break;
        }
    }
}

// Spawns a standard explosion centered on the (w x h) box at (x, y), fading from `scale`
// 255 down, and a burst of (r2, g2, b2) colored sparks scaled by g_cfg.sparks.
void SpawnExplosion(float x, float y, int w, int h, int life, int type, int p20,
                           int r, int g, int b, int r2, int g2, int b2)
{
    int i;
    float hw = w / 2.0;
    float hh = h / 2.0;
    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        if (g_explosions[i].active == 0) {
            g_explosions[i].active = 1;
            g_explosions[i].x = x + hw;
            g_explosions[i].y = y + hh;
            g_explosions[i].type = type;
            g_explosions[i].frame = 0;
            g_explosions[i].delay = (float)(RandRange(0, 3) + 2);
            g_explosions[i].timer = g_explosions[i].delay;
            g_explosions[i].spin = g_cfg.particlesOn;
            g_explosions[i].writeOnlyColorR = r;
            g_explosions[i].writeOnlyColorG = g;
            g_explosions[i].writeOnlyColorB = b;
            g_explosions[i].writeOnlyGfxIndex = p20;
            g_explosions[i].scale = 255.0f;
            g_explosions[i].scaleMul = 0.85f;
            g_explosions[i].alpha = RandRange(0, 35) + life;
            g_explosions[i].writeOnlyCenterX = x + hw;
            g_explosions[i].writeOnlyCenterY = y + hh;
            g_explosions[i].angle = RandFloat(0, 360.0f);
            g_explosions[i].angleVel = RandFloat(-2.0f, 2.0f);
            SpawnSparksRGB(RandRange(g_maxSparks, (int)g_cfg.sparks), (w >> 1) + x, (h >> 1) + y,
                               1.5f, 5.0f, 1.0f, 4.0f, r2, g2, b2);
            break;
        }
    }
}

// Bigger variant of SpawnExplosion: larger starting scale (400) and alpha, and twice as
// many sparks.
void SpawnBigExplosion(float x, float y, int w, int h, int type, int p20, int r, int g, int b)
{
    int i;
    float cx = w / 2.0 + x;
    float cy = h / 2.0 + y;
    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        if (g_explosions[i].active == 0) {
            g_explosions[i].active = 1;
            g_explosions[i].x = cx;
            g_explosions[i].y = cy;
            g_explosions[i].type = type;
            g_explosions[i].frame = 0;
            g_explosions[i].delay = (float)(RandRange(0, 3) + 2);
            g_explosions[i].timer = g_explosions[i].delay;
            g_explosions[i].spin = g_cfg.particlesOn;
            g_explosions[i].writeOnlyColorR = r;
            g_explosions[i].writeOnlyColorG = g;
            g_explosions[i].writeOnlyColorB = b;
            g_explosions[i].writeOnlyGfxIndex = p20;

            // larger scale/alpha and rotation than SpawnExplosion
            g_explosions[i].scale = 400.0f;
            g_explosions[i].scaleMul = 0.85f;
            g_explosions[i].alpha = RandRange(0, 100) + 200;
            g_explosions[i].writeOnlyCenterX = cx;
            g_explosions[i].writeOnlyCenterY = cy;
            g_explosions[i].angle = 0;
            g_explosions[i].angleVel = 0;
            SpawnSparksRGB((int)g_cfg.sparks * 2, cx, cy, 2.0f, 8.0f, 2.0f, 5.0f, r, g, b);
            break;
        }
    }
}

// Biggest explosion variant: also triggers the full-screen flash overlay and, if
// `mode` > 0, a screen-filling flash ring in a free g_flash slot (mode 1/2 pick the
// bomb-flare graphic). Scale/alpha and spark count are larger again than SpawnBigExplosion.
void SpawnHugeExplosion(float x, float y, int w, int h, int type, int p20, int r, int g, int b, int mode)
{
    int i;
    float cx = w / 2.0 + x;
    float cy = h / 2.0 + y;
    g_flashOverlayActive = 1;
    g_fadeStep = 0;
    g_fadeColorSet = 0;

    // screen-filling flash ring in a free slot, mode 1/2 picking the bomb-flare graphic
    if (mode > 0) {
        bool found = false;
        for (int j = 0; j < MAX_FLASH_RINGS; j++) {
            if (g_flash[j].active == 0 && !found) {
                g_flash[j].active = 1;
                g_flash[j].x = (int)cx;
                g_flash[j].y = (int)cy;
                g_flash[j].radius = 16;
                g_flash[j].speed = 35.0f;
                g_flash[j].alpha = 500.0f;
                g_flash[j].r = r;
                g_flash[j].g = g;
                g_flash[j].b = b;
                if (mode == 1) {
                    g_flash[j].gfx = g_gfxFlareBomb2;
                }
                if (mode == 2) {
                    g_flash[j].gfx = g_gfxFlareBomb3;
                }
                g_ringCount = 1;
                found = true;
            }
        }
    }

    // the explosion sprite itself: largest scale/alpha of the three variants
    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        if (g_explosions[i].active == 0) {
            g_explosions[i].active = 1;
            g_explosions[i].x = cx;
            g_explosions[i].y = cy;
            g_explosions[i].type = type;
            g_explosions[i].frame = 0;
            g_explosions[i].delay = (float)(RandRange(0, 3) + 2);
            g_explosions[i].timer = g_explosions[i].delay;
            g_explosions[i].spin = g_cfg.particlesOn;
            g_explosions[i].writeOnlyColorR = r;
            g_explosions[i].writeOnlyColorG = g;
            g_explosions[i].writeOnlyColorB = b;
            g_explosions[i].writeOnlyGfxIndex = p20;

            // largest scale/alpha and spark count of the three explosion variants
            g_explosions[i].scale = 600.0f;
            g_explosions[i].scaleMul = 0.85f;
            g_explosions[i].alpha = RandRange(0, 150) + 400;
            g_explosions[i].writeOnlyCenterX = cx;
            g_explosions[i].writeOnlyCenterY = cy;
            g_explosions[i].angle = 0;
            g_explosions[i].angleVel = 0;
            SpawnSparksRGB((int)g_cfg.sparks * 3, cx, cy, 2.0f, 8.0f, 1.5f, 3.5f, r, g, b);
            break;
        }
    }
}

// Per-frame update for the 50 explosion-sprite slots: scrolls them during hyperspace, and on each
// animation tick either spins/shrinks a spin-type explosion or advances its frame, deactivating it once
// its animation (or fade, for type -1) finishes.
void UpdateExplosions()
{
    int i;

    if (g_gameMode == MODE_DUAL)
        g_curPlayer = 0;

    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        if (g_explosions[i].active != 0) {
            if (g_save.players[g_curPlayer].hyperspaceFade > 0.0f)
                g_explosions[i].y += (double)g_save.players[g_curPlayer].scrollSpeedY;
            g_explosions[i].timer = g_explosions[i].timer - 1.0f;

            if (g_explosions[i].timer < 0.0f) {
                g_explosions[i].timer = g_explosions[i].delay;

                if (g_explosions[i].spin != 0) {
                    g_explosions[i].angle = g_explosions[i].angle + g_explosions[i].angleVel;
                    if (g_explosions[i].angle >= 360.0f)
                        g_explosions[i].angle = g_explosions[i].angle - 360.0f;
                    if (g_explosions[i].angle < 0.0f)
                        g_explosions[i].angle = g_explosions[i].angle + 360.0f;
                    if (g_explosions[i].scale > 0.0f) {
                        g_explosions[i].scale = g_explosions[i].scale * g_explosions[i].scaleMul;
                        if (g_explosions[i].scale < 20.0f) {
                            g_explosions[i].scale = 0;
                            g_explosions[i].active = 0;
                        }
                    } else {
                        g_explosions[i].active = 0;
                    }
                }

                g_explosions[i].frame++;
                if (g_explosions[i].type == EXPLOSION_TYPE_SMALL) {
                    // 13-frame variant (its own sprite sheet), no fade.
                    if (g_explosions[i].frame > EXPLOSION_SMALL_FRAMES)
                        g_explosions[i].active = 0;
                } else if (g_explosions[i].type == EXPLOSION_TYPE_FADE) {
                    // alpha-fades out instead of running through frames.
                    if (g_explosions[i].alpha > 0) {
                        g_explosions[i].alpha = g_explosions[i].alpha - 5;
                        if (g_explosions[i].alpha < 0) {
                            g_explosions[i].alpha = 0;
                            g_explosions[i].active = 0;
                        }
                    } else {
                        g_explosions[i].active = 0;
                    }
                } else {
                    // other types: standard 14-frame explosion.
                    if (g_explosions[i].frame > EXPLOSION_LAST_FRAME)
                        g_explosions[i].active = 0;
                }
            }
        }
    }
}

// Draws the expanding shockwave rings (g_flash, additive-blended circles that grow and fade), then the
// 50 frame-based explosion sprites that aren't spinning particle-style ones (those are drawn elsewhere).
void DrawExplosions()
{
    Rect16 src;
    int i;
    int frame;
    int dx;
    int dy;
    int w;
    int h;
    int j;

    if (g_ringCount > 0) {
        g_ringCount = 0;
        for (j = 0; j < 4; j++) {
            if (g_flash[j].active != 0) {
                QueueStretchF(g_flash[j].gfx,
                                     (float)(g_flash[j].x - g_flash[j].radius),
                                     (float)(g_flash[j].y - g_flash[j].radius),
                                     (float)(g_flash[j].x + g_flash[j].radius),
                                     (float)(g_flash[j].y + g_flash[j].radius),
                                     g_flash[j].r, g_flash[j].g, g_flash[j].b,
                                     (int)g_flash[j].alpha > 255 ? 255 : (int)g_flash[j].alpha, 0);
                g_flash[j].radius += (int)g_flash[j].speed;
                if (g_flash[j].radius > 1000)
                    g_flash[j].active = 0;
                g_flash[j].speed = g_flash[j].speed * 0.98f;
                g_flash[j].alpha = g_flash[j].alpha * 0.95f;
                if (g_flash[j].alpha < 10.0f)
                    g_flash[j].active = 0;
                g_ringCount++;
            }
        }
    }

    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        if (g_explosions[i].active != 0 && g_explosions[i].spin == 0 &&
            g_explosions[i].type != EXPLOSION_TYPE_FADE) {
            while (1) {
                if (g_explosions[i].type == EXPLOSION_TYPE_SMALL) {
                    // type 10 uses its own 32x32 sprite sheet (g_explGfx2 below).
                    dx = (int)g_explosions[i].x - 16;
                    dy = (int)g_explosions[i].y - 16;
                    frame = g_explosions[i].frame;
                    w = 32;
                    h = 32;
                    src.x1 = g_expl2SrcX[frame];
                    src.y1 = g_expl2SrcY[frame];
                } else {
                    dx = (int)g_explosions[i].x - 22;
                    dy = (int)g_explosions[i].y - 22;
                    frame = g_explosions[i].frame;
                    w = g_explW[frame];
                    h = g_explH[frame];
                    src.x1 = g_explSrcX[frame];
                    src.y1 = g_explSrcY[frame];
                }

                if (!(dx < g_clipRight && dy < g_clipBottom && dx + w > g_clipLeft && dy + h > g_clipTop))
                    break;

                if (dx < g_clipLeft) {
                    src.x1 += g_clipLeft - dx;
                    w -= g_clipLeft - dx;
                    dx = g_clipLeft;
                } else if (dx + w >= g_clipRight) {
                    w = g_clipRight - dx;
                }
                if (dy < g_clipTop) {
                    src.y1 += g_clipTop - dy;
                    h -= g_clipTop - dy;
                    dy = g_clipTop;
                } else if (dy + h >= g_clipBottom) {
                    h = g_clipBottom - dy;
                }

                src.x2 = src.x1 + w;
                src.y2 = src.y1 + h;
                if (g_explosions[i].type != EXPLOSION_TYPE_SMALL) {
                    // Other types share one sheet, stacked in EXPLOSION_TYPE_ROW_H-tall rows
                    // by explosion type.
                    src.y1 += g_explosions[i].type * EXPLOSION_TYPE_ROW_H;
                    src.y2 += g_explosions[i].type * EXPLOSION_TYPE_ROW_H;
                }

                if (g_explosions[i].type != EXPLOSION_TYPE_SMALL)
                    QueueBlit((float)dx, (float)dy, g_explGfx, &src);
                else
                    QueueBlit((float)dx, (float)dy, g_explGfx2, &src);
                break;
            }
        }
    }
}

// Per-frame update for the 500 explosion-debris particle slots: drags them along in hyperspace scroll, or
// otherwise moves and drag-decelerates them (0.98 per frame) and fades them out, deactivating once their
// speed, frame age, size (particles-off mode) or alpha bottoms out.
void UpdateExplosionDebris()
{
    int i;
    float speed;

    for (i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
        if (g_explosionParticles[i].active != 0) {
            if (g_save.players[g_curPlayer].hyperspaceFade > 0.0f) {
                g_explosionParticles[i].y =
                    g_explosionParticles[i].y + g_save.players[g_curPlayer].scrollSpeedY;
            } else {
                g_explosionParticles[i].x += (double)g_explosionParticles[i].vx;
                g_explosionParticles[i].vx = g_explosionParticles[i].vx * 0.98f;
                g_explosionParticles[i].y += (double)g_explosionParticles[i].vy;
                g_explosionParticles[i].vy = g_explosionParticles[i].vy * 0.98f;

                // NOTE: multiplies vx*vy (not vx*vx + vy*vy) before sqrt/abs — not a true speed magnitude,
                // just the original's (odd) near-zero-velocity test.
                speed = Sqrt(FabsExplosion((double)g_explosionParticles[i].vx * g_explosionParticles[i].vy));
                if (speed < 0.02f)
                    g_explosionParticles[i].active = 0;
                g_explosionParticles[i].frameF += (double)g_explosionParticles[i].frameSpeed;
                g_explosionParticles[i].ageFrame = (int)g_explosionParticles[i].frameF;
                if (g_explosionParticles[i].ageFrame > 9)
                    g_explosionParticles[i].active = 0;
                if (g_cfg.particlesOn == 0)
                    g_explosionParticles[i].size = g_explosionParticles[i].size * 0.95f;
                g_explosionParticles[i].alpha = g_explosionParticles[i].alpha * 0.96f;
            }
        }
    }
}

#define PMAX(a, b) ((a) < (b) ? (b) : (a))

#define PMIN(a, b) ((a) < (b) ? (a) : (b))

// Draws the 500 explosion-debris particles: as sprites when particle graphics are enabled, or as small
// flat-colour rects when they're off (colour/alpha boosted by a `2 - 20/value` curve so faint particles
// stay visible as plain rects).
void DrawExplosionDebris()
{
    int i;
    int a;
    float half;
    float r;
    float g;
    float b;
    float al;

    if (g_cfg.particlesOn != 0) {
        for (i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
            if (g_explosionParticles[i].active != 0) {
                a = (int)g_explosionParticles[i].alpha;
                if (a > 255)
                    a = 255;
                QueueStretchF(g_gfxTable[g_explosionParticles[i].gfx],
                                     g_explosionParticles[i].x - g_explosionParticles[i].size / 2.0f,
                                     g_explosionParticles[i].y - g_explosionParticles[i].size / 2.0f,
                                     g_explosionParticles[i].x + g_explosionParticles[i].size / 2.0f,
                                     g_explosionParticles[i].y + g_explosionParticles[i].size / 2.0f,
                                     g_explosionParticles[i].r, g_explosionParticles[i].g,
                                     g_explosionParticles[i].b,
                                     a, 0);
            }
        }
    } else {
        for (i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
            if (g_explosionParticles[i].active != 0) {
                half = g_explosionParticles[i].size / 6.0;

                r = PMIN(g_explosionParticles[i].r * PMAX(2.0 - 20.0 / g_explosionParticles[i].r, 0), 255)
                    / 255.0;
                g = PMIN(g_explosionParticles[i].g * PMAX(2.0 - 20.0 / g_explosionParticles[i].g, 0), 255)
                    / 255.0;
                b = PMIN(g_explosionParticles[i].b * PMAX(2.0 - 20.0 / g_explosionParticles[i].b, 0), 255)
                    / 255.0;
                al = PMIN(g_explosionParticles[i].alpha
                          * PMAX(2.0 - 20.0 / g_explosionParticles[i].alpha, 0), 255)
                    / 255.0;

                g_screen->setAlphaMode(0);
                g_screen->drawRect(g_explosionParticles[i].x - half, g_explosionParticles[i].y - half,
                                          g_explosionParticles[i].x + half, g_explosionParticles[i].y + half,
                                          r, g, b, al);
            }
        }
    }
}

#undef PMAX
#undef PMIN
