// particles.c: Particle systems: star-field burst slots, sprite particles, sparks and fireworks.
#include "globals.h"
#include "game.h"


// Deactivates all 1000 star-field slots and resets the active count.
void ClearSlots()
{
    int i;
    for (i = 0; i < MAX_SLOTS; i++)
        g_slots[i].active = 0;
    g_slotCount = 0;
}

// Activates up to `n` free star-field slots at (x, y) with color (r, g, b), all fading
// towards black at one random rate (dr/dg/db) picked for this whole call; each slot
// gets its own random outward position/velocity/acceleration and angle. `cnt` is
// computed but unused. NOTE: `n--` in the loop condition means it spawns n+1 slots.
void SpawnSlots(float x, float y, int n, int r, int g, int b)
{
    int i;
    int cnt;
    float dr;
    float dg;
    float db;
    dr = RandFloat(1.5f, 6.0f);
    dg = RandFloat(1.5f, 6.0f);
    db = RandFloat(1.5f, 6.0f);
    cnt = n;
    if (cnt < 0)
        cnt = 1;

    for (i = 0; i < MAX_SLOTS; i++) {
        if (g_slots[i].active == 0) {
            g_slots[i].x = x;
            g_slots[i].y = y;
            g_slots[i].active = 1;
            g_slots[i].pos = RandFloat(1.0f, 40.0f);
            g_slots[i].vel = RandFloat(2.0f, 4.0f);
            g_slots[i].accel = RandFloat(0.01f, 0.2f);

            g_slots[i].r = (float)r;
            g_slots[i].g = (float)g;
            g_slots[i].b = (float)b;
            g_slots[i].dr = dr;
            g_slots[i].dg = dg;
            g_slots[i].db = db;
            g_slots[i].angle = RandRange(0, 3600);
            g_slotCount++;
            if (!n--)
                break;
        }
    }
}

// Advances every active star-field slot: outward position/velocity by `dt`, color
// fading towards black; deactivates a slot once its velocity goes negative or its
// color has fully faded.
void UpdateSlots(float dt)
{
    int i;

    if (g_slotCount > 0) {
        for (i = 0; i < MAX_SLOTS; i++) {
            if (g_slots[i].active != 0) {
                g_slots[i].pos += g_slots[i].vel * dt;
                g_slots[i].vel -= g_slots[i].accel * dt;
                g_slots[i].r -= g_slots[i].dr * dt;
                if (g_slots[i].r < 0.0)
                    g_slots[i].r = 0;
                g_slots[i].g -= g_slots[i].dg * dt;
                if (g_slots[i].g < 0.0)
                    g_slots[i].g = 0;
                g_slots[i].b -= g_slots[i].db * dt;
                if (g_slots[i].b < 0.0)
                    g_slots[i].b = 0;
                if (g_slots[i].vel < 0.0 ||
                    (g_slots[i].r == 0.0 && g_slots[i].g == 0.0 && g_slots[i].b == 0.0)) {
                    g_slots[i].active = 0;
                    g_slotCount--;
                }
            }
        }
    }
}

// Draws every active star-field slot as a single pixel at its (x, y) plus a polar
// offset (angle/pos on the trig tables), clipped to the given screen rectangle.
void DrawSlots(int minX, int maxX, int minY, int maxY)
{
    int i;
    int px;
    int py;
    if (g_slotCount > 0) {
        for (i = 0; i < MAX_SLOTS; i++) {
            if (g_slots[i].active != 0) {
                px = g_slots[i].x + g_cosTable[g_slots[i].angle] * g_slots[i].pos;
                py = g_slots[i].y + g_sinTable[g_slots[i].angle] * g_slots[i].pos;
                if (px > minX && px < maxX && py < maxY && py > minY)
                    PlotPixel((float)px, (float)py, g_slots[i].r, g_slots[i].g,
                              g_slots[i].b, 1.0f);
            }
        }
    }
}

// Deactivates every particle slot.
void ClearParticles()
{
    for (int i = 0; i < MAX_PARTICLES - 1; i++) {
        g_particles[i].active = 0;
    }
}

// Counts active particle slots.
int CountParticles()
{
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES - 1; i++) {
        if (g_particles[i].active != 0) {
            count++;
        }
    }
    return count;
}

// Activates the first free particle slot with the given graphic, position, size/growth,
// rotation/spin, initial travel `dir` (degrees) and `speed`, color+alpha (fading out
// over `life`), movement `mode` (see UpdateParticles()), `gravity`, and:
//  - `spawn`: if not -1, average frames between this particle spawning a child particle
//    (see UpdateParticles()); -1 for a non-spawning (leaf) particle.
//  - `xref`: if set, overrides the particle's drawn x with *xref every frame (e.g. to
//    follow a moving object) without changing its own x.
//  - `kill`: if set and becomes nonzero, clears ALL particles (used as a global "cancel
//    effects" signal, not just this one).
// No-ops if particles are disabled or the pool is full.
void AddParticle(Image *graphic, int x, int y, float size, float sizeVel,
                        float angle, float angleVel, int dir, int r, int g, int b, int alpha,
                        float life, float speed, int spawn, float gravity, int mode,
                        int *xref, int *kill, unsigned char flag)
{
    if (g_cfg.particlesOn == 0)
        return;
    for (int i = 0; i < MAX_PARTICLES - 1; i++) {
        if (g_particles[i].active == 0) {
            g_particles[i].active = 1;
            g_particles[i].graphic = graphic;
            g_particles[i].x = x;
            g_particles[i].y = y;
            g_particles[i].alpha = alpha;
            g_particles[i].alphaStep = alpha / life;
            g_particles[i].angle = angle;
            g_particles[i].angleVel = angleVel;
            g_particles[i].r = r;
            g_particles[i].g = g;
            g_particles[i].b = b;
            g_particles[i].life = life;
            g_particles[i].spawn = spawn;
            g_particles[i].gravity = gravity;
            g_particles[i].speed = speed;
            g_particles[i].size = size;
            g_particles[i].sizeVel = sizeVel;
            g_particles[i].mode = mode;
            g_particles[i].dir = dir;
            g_particles[i].vx = g_cosDeg[dir] * speed;
            g_particles[i].vy = 0.0f - g_sinDeg[dir] * speed;
            g_particles[i].xref = xref;
            g_particles[i].kill = kill;
            g_particles[i].flag = flag;
            return;
        }
    }
}

// Advances every active particle one frame (fixed g_dt = 1.0): ages out life/alpha,
// grows/shrinks size, spins, and moves per its `mode` (0 = free-flying with gravity,
// 1 = vertical only, 2 = horizontal only). Particles with a spawn interval randomly
// spawn a smaller, shorter-lived child particle. If any particle's `kill` flag has been
// set from elsewhere, clears all particles and returns immediately.
void UpdateParticles()
{
    int i;
    float angle;

    if (g_cfg.particlesOn == 0)
        return;
    for (i = 0; i < MAX_PARTICLES - 1; i++) {
        if (g_particles[i].active != 0) {
            if (g_particles[i].kill && *g_particles[i].kill) {
                *g_particles[i].kill = 0;
                ClearParticles();
                return;
            }

            // NOTE: g_dt is always set to 1.0f here regardless of `flag` — both
            // branches assign the same value, so this if/else has no effect.
            g_dt = 1.0f;
            if (g_particles[i].flag)
                g_dt = 1.0f;

            g_particles[i].life -= 1.0f * g_dt;
            if (g_particles[i].life < 0.0)
                g_particles[i].active = 0;
            g_particles[i].alpha -= g_particles[i].alphaStep * g_dt;
            if (g_particles[i].alpha < 0.0)
                g_particles[i].alpha = 0;
            g_particles[i].size += g_particles[i].sizeVel * g_dt;
            if (g_particles[i].size < 1.0)
                g_particles[i].size = 1.0f;
            g_particles[i].angle += g_particles[i].angleVel * g_dt;
            if (g_particles[i].angle < 0.0)
                g_particles[i].angle += 360.0;
            if (g_particles[i].angle > 360.0)
                g_particles[i].angle -= 360.0;

            if (g_particles[i].mode == 0) {
                g_particles[i].x += g_particles[i].vx * g_dt;
                g_particles[i].y += g_particles[i].vy * g_dt;
                g_particles[i].vy += g_particles[i].gravity * g_dt;
            }
            if (g_particles[i].mode == 1) {
                g_particles[i].y += g_particles[i].vy * g_dt;
                g_particles[i].vy += g_particles[i].gravity * g_dt;
            }
            if (g_particles[i].mode == 2) {
                g_particles[i].x += g_particles[i].vx * g_dt;
            }

            if (g_particles[i].spawn != -1) {
                // ~1-in-(spawn+1) chance per frame of spawning a child particle: smaller,
                // quarter the life, 1-10% of the speed, random travel direction (or this
                // particle's own angle, for the non-free-flying modes).
                if (RandRange(0, (int)((g_particles[i].spawn + 1) * (1.0f / g_dt))) == 0) {
                    angle = RandFloat(0, 359.0f);
                    if (g_particles[i].mode != 0)
                        angle = g_particles[i].angle;

                    AddParticle(g_particles[i].graphic,
                                       (int)g_particles[i].x,
                                       (int)g_particles[i].y,
                                       g_particles[i].size * (RandFloat(50.0f, 95.0f) / 100.0f),
                                       g_particles[i].sizeVel,
                                       angle,
                                       g_particles[i].angleVel,
                                       RandRange(0, 360),
                                       g_particles[i].r,
                                       g_particles[i].g,
                                       g_particles[i].b,
                                       (int)g_particles[i].alpha,
                                       g_particles[i].life / 4.0,
                                       g_particles[i].speed * (RandFloat(1.0f, 10.0f) / 100.0f),
                                       -1,
                                       g_particles[i].gravity,
                                       g_particles[i].mode,
                                       g_particles[i].xref,
                                       g_particles[i].kill,
                                       g_particles[i].flag);
                }
            }
        }
    }
}

// Queues every active particle for drawing via QueueStretchRot(), sized/positioned per
// its `mode`: mode 0 draws a normal square sprite centered on (x, y) (using `xref`'s
// current value instead of x if set); mode 1 stretches it into a tall vertical streak
// centered horizontally on screen; mode 2 stretches it into a full-height vertical
// column at its own x. Alpha is clamped to 255.
void DrawParticles()
{
    float s;
    int a;
    float x;
    int i;

    if (g_cfg.particlesOn == 0)
        return;
    for (i = 0; i < MAX_PARTICLES - 1; i++) {
        if (g_particles[i].active != 0) {
            a = (int)g_particles[i].alpha > 255 ? 255 : (int)g_particles[i].alpha;
            x = g_particles[i].x;
            if (g_particles[i].xref)
                x = *g_particles[i].xref;

            if (g_particles[i].mode == 0) {
                s = g_particles[i].size * 0.6f;
                QueueStretchRot(g_particles[i].graphic,
                                       x - s, g_particles[i].y - s,
                                       x + s, g_particles[i].y + s,
                                       g_particles[i].r, g_particles[i].g,
                                       g_particles[i].b, a, 0,
                                       g_particles[i].angle);
            }

            if (g_particles[i].mode == 1) {
                s = g_particles[i].size;
                QueueStretchRot(g_particles[i].graphic,
                                       (g_screenW >> 1) - s,
                                       g_particles[i].y - 100.0 - 350.0,
                                       (g_screenW >> 1) + s,
                                       g_particles[i].y + g_screenW - 350.0,
                                       g_particles[i].r, g_particles[i].g,
                                       g_particles[i].b, a, 0,
                                       g_particles[i].angle);
            }

            if (g_particles[i].mode == 2) {
                s = g_particles[i].size;
                QueueStretchRot(g_particles[i].graphic,
                                       g_particles[i].x - s, 0,
                                       g_particles[i].x + s, g_screenH,
                                       g_particles[i].r, g_particles[i].g,
                                       g_particles[i].b, a, 0,
                                       g_particles[i].angle);
            }
        }
    }
}

// Allocates a spark in the first free slot of g_sparks (2000 max) and initialises its position, velocity
// (from angle/speed via the deg lookup tables), colour, size and fade/trail timing.
void SpawnSpark(int x, int y, int r, int g, int b, int type, float speed, int angle,
                       int delay, int fade, int trailInterval, int size)
{
    for (int i = 0; i < MAX_SPARKS; i++) {
        if (g_sparks[i].active == 0) {
            g_sparks[i].type = type;
            g_sparks[i].active = 1;
            g_sparks[i].moving = 1;
            g_sparks[i].x = (float)x;
            g_sparks[i].y = (float)y;
            g_sparks[i].vx = g_cosDeg[angle] * speed;
            g_sparks[i].vy = g_sinDeg[angle] * speed;
            g_sparks[i].delay = delay;
            g_sparks[i].alpha = 255.0f;
            g_sparks[i].fade = (float)fade;
            g_sparks[i].trailInterval = trailInterval;
            g_sparks[i].trailTimer = g_sparks[i].trailInterval;
            g_sparks[i].size = size;
            g_sparks[i].r = r;
            g_sparks[i].g = g;
            g_sparks[i].b = b;
            g_flag = 1;
            break;
        }
    }
}

// Per-frame update and draw for every active spark: picks its flare graphic by type, draws it as a sprite
// (particles on) or a single plotted pixel (particles off), spawns a smaller trailing spark at intervals
// while moving, fades its alpha, deactivating at zero, and integrates its velocity with light drag.
void UpdateSparks()
{
    Image *graphic = 0;
    g_flag = 0;
    for (int i = 0; i < MAX_SPARKS; i++) {
        if (g_sparks[i].active != 0) {
            g_flag = 1;

            if (g_cfg.particlesOn != 0) {
                // Flare graphic per spark type; several types intentionally alias the same graphic.
                switch (g_sparks[i].type) {
                case 0: graphic = g_gfxFlare1; break;
                case 1: graphic = g_gfxFlare2; break;
                case 2: graphic = g_gfxFlare3; break;
                case 3: graphic = g_gfxFlare1; break;
                case 4: graphic = g_gfxFlare5; break;
                case 5: graphic = g_gfxFlare6; break;
                case 6: graphic = g_gfxFlare7; break;
                case 7: graphic = g_gfxFlare1; break;
                case 8: graphic = g_gfxFlare1; break;
                case 9: graphic = g_gfxFlare10; break;
                case 10: graphic = g_gfxFlare11; break;
                case 11: graphic = g_gfxFlare19; break;
                case 12: graphic = g_gfxFlare18; break;
                case 13: graphic = g_gfxFlare14; break;
                case 14: graphic = g_gfxFlare15; break;
                case 15: graphic = g_gfxFlare16; break;
                case 16: graphic = g_gfxFlare17; break;
                default: graphic = g_gfxFlare1; break;
                }
            }

            // Draw: a stretched sprite when particles are enabled, otherwise a single lit pixel.
            if (g_cfg.particlesOn != 0) {
                QueueStretchF(graphic,
                                     g_sparks[i].x - (g_sparks[i].size >> 1),
                                     g_sparks[i].y - (g_sparks[i].size >> 1),
                                     g_sparks[i].x + (g_sparks[i].size >> 1),
                                     g_sparks[i].y + (g_sparks[i].size >> 1),
                                     g_sparks[i].r, g_sparks[i].g, g_sparks[i].b,
                                     (unsigned char)g_sparks[i].alpha, 0);
            } else {
                PlotPixel(g_sparks[i].x, g_sparks[i].y,
                          g_sparks[i].r / 255.0,
                          g_sparks[i].g / 255.0,
                          g_sparks[i].b / 255.0,
                          g_sparks[i].alpha / 255.0);
            }

            // Update: only while the game is running (g_state != STATE_PAUSED), not while paused/menus.
            if (g_state != STATE_PAUSED) {
                if (g_sparks[i].moving == 1) {
                    if (!g_sparks[i].trailTimer--) {
                        g_sparks[i].trailTimer = g_sparks[i].trailInterval;
                        for (int j = 0; j < MAX_SPARKS; j++) {
                            if (g_sparks[j].active == 0) {
                                g_sparks[j].type = g_sparks[i].type;
                                g_sparks[j].active = 1;
                                g_sparks[j].moving = 0;
                                g_sparks[j].delay = 0;
                                g_sparks[j].x = g_sparks[i].x;
                                g_sparks[j].y = g_sparks[i].y;
                                g_sparks[j].alpha = g_sparks[i].alpha;
                                g_sparks[j].fade = g_sparks[i].fade;
                                g_sparks[j].size = g_sparks[i].size >> 1;
                                g_sparks[j].r = g_sparks[i].r;
                                g_sparks[j].g = g_sparks[i].g;
                                g_sparks[j].b = g_sparks[i].b;
                                break;
                            }
                        }
                    }
                }

                if (g_sparks[i].delay > 0)
                    g_sparks[i].delay = g_sparks[i].delay - 1;
                if (g_sparks[i].delay < 1) {
                    g_sparks[i].alpha = g_sparks[i].alpha - g_sparks[i].fade * g_frameDt;
                    if (g_sparks[i].alpha < 0.0)
                        g_sparks[i].active = 0;
                }

                if (g_sparks[i].moving == 1) {
                    g_sparks[i].x += g_sparks[i].vx * g_frameDt * 0.98f;
                    g_sparks[i].y += g_sparks[i].vy * g_frameDt;
                    g_sparks[i].vy += 0.04f;
                }
            }
        }
    }
}

// Spawns a random firework burst of 10-50 sparks at a random point, plus (20% chance) a second ring-shaped
// burst of evenly-spaced sparks at the same point.
void SpawnFirework()
{
    int x = RandRange(100, g_screenW - 100);
    int y = RandRange(100, g_screenH - 100);
    int r = RandRange(0, 255);
    int g = RandRange(0, 255);
    int b = RandRange(0, 255);
    int count = RandRange(10, 50);
    int type = RandRange(0, 15);

    for (int i = 0; i < count; i++) {
        float speed = RandFloat(0.5f, 5.0f);
        int angle = RandRange(0, 360);
        int delay = RandRange(10, 100);
        int fade = RandRange(5, 15);
        int trail = RandRange(3, 7);
        int size = RandRange(10, 50);
        SpawnSpark(x, y, r, g, b, type, speed, angle, delay, fade, trail, size);
    }

    // 20% chance of a second, ring-shaped burst at the same point.
    if (RandRange(0, 10) < 2) {
        int r2 = RandRange(0, 255);
        int g2 = RandRange(0, 255);
        int b2 = RandRange(0, 255);
        int count2 = RandRange(5, 25);
        int type2 = RandRange(0, 15);
        float angle2 = 0;
        float step = 360.0 / count2;
        float speed2 = RandFloat(0.5f, 5.0f);
        int delay2 = RandRange(10, 100);
        int fade2 = RandRange(5, 15);
        int trail2 = RandRange(3, 7);
        int size2 = RandRange(10, 50);

        for (int j = 0; j < count2; j++) {
            SpawnSpark(x, y, r2, g2, b2, type2, speed2, (int)angle2, delay2, fade2, trail2, size2);
            angle2 = angle2 + step;
        }
    }
}

// Spawns up to `count` explosion-particle sparks at (x, y) with random speed in
// [spdLo, spdHi] and random (deceleration) magnitude in [accLo, accHi], in a random
// direction each, using a randomised blue-ish color.
void SpawnSparks(int count, float x, float y, float spdLo, float spdHi, float accLo, float accHi)
{
    int i;
    int ang;
    float speed;
    float acc;
    float life;

    for (i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
        if (g_explosionParticles[i].active == 0) {
            g_explosionParticles[i].active = 1;
            g_explosionParticles[i].x = x;
            g_explosionParticles[i].y = y;

            // random direction, speed and drag
            ang = RandRange(0, 359);
            speed = RandFloat(spdLo, spdHi);
            g_explosionParticles[i].vx = g_cosDeg[ang] * speed;
            g_explosionParticles[i].vy = g_sinDeg[ang] * speed;
            acc = RandFloat(accLo, accHi) / 20.0;
            g_explosionParticles[i].ax = g_cosDeg[ang] * acc * g_frameDt;
            g_explosionParticles[i].ay = g_sinDeg[ang] * acc * g_frameDt;
            if (acc == 0.0)
                acc = 1.0f;
            life = speed / acc;

            // animation frame and fade timing
            g_explosionParticles[i].frame = RandRange(0, 5);
            g_explosionParticles[i].frameBase = RandRange(0, 2) * 13;
            if (life == 0.0)
                life = 1.0f;
            g_explosionParticles[i].frameSpeed = 10 / life;
            g_explosionParticles[i].frameF = 0;
            g_explosionParticles[i].ageFrame = 0;
            g_explosionParticles[i].unusedFlickerTimer = (int)life / 15;
            g_explosionParticles[i].unusedFlickerReset = g_explosionParticles[i].unusedFlickerTimer;
            g_explosionParticles[i].alpha = 350.0f;
            g_explosionParticles[i].unusedFadeMul = 0.95f;

            // size and randomised blue-ish color
            g_explosionParticles[i].size = RandFloat(5.0f, 45.0f);
            g_explosionParticles[i].gfx = 4;
            g_explosionParticles[i].r = RandRange(0, 100);
            g_explosionParticles[i].g = RandRange(150, 255);
            g_explosionParticles[i].b = 255;
            count--;
            if (count < 1)
                break;
        }
    }
}

// Same as SpawnSparks, but with an explicit (r, g, b) color instead of the randomised one.
void SpawnSparksRGB(int count, float x, float y, float spdLo, float spdHi, float accLo, float accHi,
                           int r, int g, int b)
{
    int i;
    int ang;
    float speed;
    float acc;
    float life;

    for (i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
        if (g_explosionParticles[i].active == 0) {
            g_explosionParticles[i].active = 1;
            g_explosionParticles[i].x = x;
            g_explosionParticles[i].y = y;
            ang = RandRange(0, 359);
            speed = RandFloat(spdLo, spdHi);
            g_explosionParticles[i].vx = g_cosDeg[ang] * speed;
            g_explosionParticles[i].vy = g_sinDeg[ang] * speed;
            acc = RandFloat(accLo, accHi) / 20.0;
            g_explosionParticles[i].ax = g_cosDeg[ang] * acc * g_frameDt;
            g_explosionParticles[i].ay = g_sinDeg[ang] * acc * g_frameDt;
            if (acc == 0.0)
                acc = 1.0f;
            life = speed / acc;
            g_explosionParticles[i].frame = RandRange(0, 5);
            g_explosionParticles[i].frameBase = RandRange(0, 2) * 13;
            if (life == 0.0)
                life = 1.0f;
            g_explosionParticles[i].frameSpeed = 10 / life;
            g_explosionParticles[i].frameF = 0;
            g_explosionParticles[i].ageFrame = 0;
            g_explosionParticles[i].unusedFlickerTimer = (int)life / 15;
            g_explosionParticles[i].unusedFlickerReset = g_explosionParticles[i].unusedFlickerTimer;
            g_explosionParticles[i].alpha = 350.0f;
            g_explosionParticles[i].unusedFadeMul = 0.95f;
            g_explosionParticles[i].size = RandFloat(5.0f, 45.0f);
            g_explosionParticles[i].gfx = 4;
            g_explosionParticles[i].r = r;
            g_explosionParticles[i].g = g;
            g_explosionParticles[i].b = b;
            count--;
            if (count < 1)
                break;
        }
    }
}
