// stars.c: Backgrounds: the scrolling background, star fields (menu, flight, sprite, glow),
// borders, the dot-matrix signature.
#include "globals.h"
#include "game.h"


// Projects and draws `count` 3D star points (xs/ys/zs) to the screen rect, shaded by
// depth. Below `speed` 8.0 stars are tinted blue (or, if `bright`, white/cyan); at or
// above it they streak: color shifts with `speed` and each star trails `step`-px fading
// copies upward, proportional to `speed`.
void DrawStarField(int count, float *xs, float *ys, float *zs, int minY, int maxY, int minX, int maxX,
                      float zNear, float zFar, float speed, bool bright)
{
    int j;
    int i;
    int r;
    int g;
    int b;
    int shade;
    int sx;
    int sy;
    float z;
    float zscale;
    int step;
    int unused;   // NOTE: assigned but never read; dead code kept for the byte match.
    step = 2;
    unused = 200;
    zscale = 200.0 / (zFar - zNear);
    count = count * (1.0 - speed / 300.0);
    for (i = 0; i < count; i++) {
        z = zs[i];
        if (z == 0.0)
            z = 0.1f;
        sx = xs[i] * 75.0 / z + 400.0;
        sy = ys[i] * 75.0 / z + 300.0;
        shade = 255 - (int)((z - zNear) * zscale);
        if (speed < 8.0) {
            if (!bright) {
                r = shade >> 1;
                g = shade >> 1;
                b = shade;
                if (sx > minX && sx < maxX && sy < maxY && sy > minY)
                    PlotPixel((float)sx, (float)sy, (float)(r / 255.0), (float)(g / 255.0),
                              (float)(b / 255.0), 1.0f);
            } else {
                r = shade;
                g = shade;
                b = shade * 2 > 255 ? 255 : shade * 2;
                if (sx > minX && sx < maxX && sy < maxY && sy > minY)
                    PlotPixel((float)sx, (float)sy, (float)(r / 255.0), (float)(g / 255.0),
                              (float)(b / 255.0), 1.0f);
            }
        } else {
            r = shade / 2 + speed;
            if (r > 255)
                r = 255;
            if (r < 0)
                r = 1;
            g = shade / 2 + (int)(speed / 6.0);
            if (g > 255)
                g = 255;
            if (g < 0)
                g = 1;
            b = shade - (int)speed;
            if (b > 255)
                b = 255;
            if (b < 0)
                b = 1;
            if (sx > minX && sx < maxX && sy < maxY && sy > minY)
                PlotPixel((float)sx, (float)sy, (float)(r / 255.0), (float)(g / 255.0),
                          (float)(b / 255.0), 1.0f);
            if (speed > 0.0) {
                shade = speed / 5.0;
                for (j = 0; j < shade; j++) {
                    r -= step;
                    g -= step;
                    b -= step;
                    if (r < 0)
                        r = 0;
                    if (g < 0)
                        g = 0;
                    if (b < 0)
                        b = 0;
                    if (r > 0 || g > 0 || b > 0) {
                        if (sy - (j * 2 + 2) > 0 && sx > minX && sx < maxX &&
                            sy - (j * 2 + 2) < maxY && sy - (j * 2 + 2) > minY)
                            PlotPixel((float)sx, (float)(sy - (j * 2 + 2)), (float)(r / 255.0),
                                      (float)(g / 255.0), (float)(b / 255.0), 1.0f);
                    }
                }
            }
        }
    }
}

#define MIN_(a, b) (((a) < (b)) ? (a) : (b))

#define MAX_(a, b) (((a) > (b)) ? (a) : (b))

// Draws the vertically-scrolling starfield background (one of g_bg1..g_bg5, picked by
// g_bgIndex) as two copies stacked 600px apart so it tiles seamlessly, tinted by
// g_cfg.bgTint and reddened as the current player's ship speeds up (or a fixed dark red
// in state 16). Some states use a narrower strip inset 64px from each edge; others use
// the full screen width. Advances the scroll offset by player speed while gameplay is
// running (g_state != 0), wrapping it into [0, 600).
void DrawBackground()
{
    int r;
    int g;
    int b;
    int p;
    float x1;
    float y1;
    float x2;
    float y2;

    if (g_cfg.bgEnabled == 1) {
        // Whose starSpeed drives the tint/scroll: usually the current player, but
        // player 0 in 2-player mode (outside state 5) and always in state 5.
        p = g_curPlayer;
        if (g_gameMode == MODE_DUAL && g_state != STATE_TITLE)
            p = 0;
        if (g_state == STATE_TITLE)
            p = 0;

        x1 = 64;
        y1 = g_bgScroll + -600.0;
        x2 = g_screenW - 64.0;
        y2 = 0.0f + g_bgScroll;

        // NOTE: STATE_END_SEQUENCE is listed twice.
        if (g_state == STATE_TITLE || g_state == STATE_END_SEQUENCE || g_state == STATE_ENTER_HISCORE ||
            g_state == STATE_END_SEQUENCE || g_state == STATE_HISCORE_TABLE || g_state == STATE_POST_ROUND_IDLE ||
            g_state == STATE_INPUT_CONFIG || g_state == STATE_UNUSED_1) {
            x1 = 0;
            x2 = g_screenW;
        }

        if (g_state == STATE_MALFUNCTION) {
            r = 60;
            g = 0;
            b = 50;
        } else {
            r = MAX_(0, MIN_(255, (int)MIN_(255.0, MAX_(g_save.players[p].starSpeed - 30.0, 0) / 3.0)
                                   + g_cfg.bgTint));
            g = MAX_(0, MIN_(255, g_cfg.bgTint
                                   - (int)(MAX_(g_save.players[p].starSpeed - 30.0, 0) / 3.0)));
            b = MAX_(0, MIN_(255, g_cfg.bgTint
                                   - (int)(MAX_(g_save.players[p].starSpeed - 30.0, 0) / 3.0)));
        }

        // Draws background layer `n` (g_bg##n) as two copies 600px apart so it tiles.
        #define DRAW_BG_LAYER(n)                                                                \
            ImgSetBlitColor(g_bg##n, r / 255.0, g / 255.0, b / 255.0, 1);                       \
            ImgStretchAlphaRect(g_bg##n, 0, 0, ImgWidth(g_bg##n), ImgHeight(g_bg##n),           \
                                x1, y1, x2, y2, 1, 0, false, false, 0, 0);                       \
            ImgStretchAlphaRect(g_bg##n, 0, 0, ImgWidth(g_bg##n), ImgHeight(g_bg##n),           \
                                x1, y1 + 600.0, x2, y2 + 600.0, 1, 0, false, false, 0, 0);

        if (g_bgIndex == 1) {
            DRAW_BG_LAYER(1);
        }

        if (g_bgIndex == 2) {
            DRAW_BG_LAYER(2);
        }

        if (g_bgIndex == 3) {
            DRAW_BG_LAYER(3);
        }

        if (g_bgIndex == 4) {
            DRAW_BG_LAYER(4);
        }

        if (g_bgIndex == 5) {
            DRAW_BG_LAYER(5);
        }

        #undef DRAW_BG_LAYER

        if (g_state != 0) {
            g_bgScroll += g_save.players[p].starSpeed / 20.0;
            if (g_bgScroll >= 600.0)
                g_bgScroll -= 600.0;
            if (g_bgScroll < 0.0)
                g_bgScroll += 600.0;
        }
    }
}

#undef MIN_
#undef MAX_

// Randomizes the background star-field's rotation axis/speed and picks the still or glowing star
// renderer based on config; called when (re)entering a screen that shows the rotating star background.
void InitStarRotation()
{
    EmptyViewChangeHook();
    // NOTE: these three RandFloat values are immediately overwritten below by g_cosDeg[angle]; dead
    // writes kept from the original.
    g_save.players[0].starVelX = RandFloat(1.0f, 8.0f);
    g_save.players[0].starSpeed = RandFloat(1.0f, 8.0f);
    g_save.players[0].starVelZ = RandFloat(1.0f, 8.0f);
    g_angX = RandFloat(0.0f, 360.0f);
    g_angVelX = RandFloat(-5.0f, 5.0f) / 25.0;
    g_starSpeedX = 3.0f;
    g_save.players[0].starVelX = g_cosDeg[(int)g_angX];
    g_angY = RandFloat(0.0f, 360.0f);
    g_angVelY = RandFloat(-5.0f, 5.0f) / 25.0;
    g_starSpeedY = 3.0f;
    g_save.players[0].starSpeed = g_cosDeg[(int)g_angY];
    g_angZ = RandFloat(0.0f, 360.0f);
    g_angVelZ = RandFloat(-5.0f, 5.0f) / 25.0;
    g_starSpeedZ = 3.0f;
    g_save.players[0].starVelZ = g_cosDeg[(int)g_angZ];
    if (g_cfg.bgStars != 0)
        g_fnPtr = DrawStarsStill;
    else
        g_fnPtr = DrawStarsGlow;
}

// Scrolls the starfield for the menu/title background and slowly drifts the scroll
// direction (g_angX/Y/Z) in a random walk, picking a new random turn rate each time
// an angle wraps past 360 degrees.
void UpdateMenuStars()
{
    int i;
    for (i = 0; i < (int)g_cfg.numStars; i++) {
        g_starZ[i] += g_save.players[0].starVelZ;
        g_starX[i] += g_save.players[0].starVelX;
        g_starY[i] += g_save.players[0].starSpeed;
        if (g_starZ[i] > g_starZFar)
            g_starZ[i] = g_starZNear;
        if (g_starZ[i] < g_starZNear)
            g_starZ[i] = g_starZFar;
        if (g_starX[i] > g_starXMax)
            g_starX[i] -= g_starXMax - g_starXMin;
        if (g_starX[i] < g_starXMin)
            g_starX[i] += g_starXMax - g_starXMin;
        if (g_starY[i] > g_starYMax)
            g_starY[i] -= g_starYMax - g_starYMin;
        if (g_starY[i] < g_starYMin)
            g_starY[i] += g_starYMax - g_starYMin;
    }

    g_angX += g_angVelX;
    if (g_angX < 0.0)
        g_angX += 360.0;
    if (!(g_angX < 360.0)) {
        g_angX -= 360.0;
        g_angVelX = RandFloat(-5.0f, 5.0f) / 25.0;
    }
    g_save.players[0].starVelX = g_cosDeg[(int)g_angX];

    g_angY += g_angVelY;
    if (g_angY < 0.0)
        g_angY += 360.0;
    if (!(g_angY < 360.0)) {
        g_angY -= 360.0;
        g_angVelY = RandFloat(-5.0f, 5.0f) / 25.0;
    }
    g_save.players[0].starSpeed = g_cosDeg[(int)g_angY];

    g_angZ += g_angVelZ;
    if (g_angZ < 0.0)
        g_angZ += 360.0;
    if (!(g_angZ < 360.0)) {
        g_angZ -= 360.0;
        g_angVelZ = RandFloat(-5.0f, 5.0f) / 25.0;
    }
    g_save.players[0].starVelZ = g_cosDeg[(int)g_angZ];
}

// Draws the starfield using the current player's star speed.
void DrawStarsPlayer()
{
    DrawStarField((int)g_cfg.numStars, g_starX, g_starY, g_starZ,
                     g_clipTop, g_clipBottom, g_clipLeft, g_clipRight,
                     g_starZNear, g_starZFar, g_save.players[g_curPlayer].starSpeed,
                     *(bool *)&g_windowed);
}

// Draws the starfield with zero scroll speed (stationary background).
void DrawStarsStill()
{
    DrawStarField((int)g_cfg.numStars, g_starX, g_starY, g_starZ,
                     g_clipTop, g_clipBottom, g_clipLeft, g_clipRight,
                     g_starZNear, g_starZFar, 0, *(bool *)&g_windowed);
}

// Plots count dim, flickering dots at (x,y) offset by (xs[i],ys[i])*scale; used to
// draw one letter of the dot-matrix signature.
void DrawDotGlyph(int x, int y, int scale, int count, int *xs, int *ys)
{
    int px;
    int py;
    int base;
    int i;
    base = RandRange(50, 90);
    for (i = 0; i < count; i++) {
        px = xs[i] * scale + x;
        py = ys[i] * scale + y;
        PlotPixel(px, py, 0.1f, 0.1f, (RandRange(0, 40) + base) / 255.0, 1.0f);
    }
}

// Draws the "PAYdegra" dot-matrix signature at a random screen position, one glyph
// per DrawDotGlyph call, using per-letter point tables.
void DrawDotSignature()
{
    int xP[13] = {1, 3, 5, 1, 7, 1, 1, 1, 1, 3, 5, 7, 8};
    int yP[13] = {1, 1, 1, 3, 2, 5, 7, 9, 11, 7, 7, 6, 4};
    int xA[13] = {1, 2, 3, 4, 5, 6, 5, 7, 7, 8, 9, 10, 11};
    int yA[13] = {11, 9, 7, 5, 3, 1, 7, 7, 3, 5, 7, 9, 11};
    int xY[9] = {1, 2, 3, 4, 4, 4, 5, 6, 7};
    int yY[9] = {1, 3, 5, 7, 9, 11, 5, 3, 1};
    int xE[12] = {0, 2, 4, 6, 0, 0, 2, 0, 0, 2, 4, 6};
    int yE[12] = {0, 0, 0, 0, 2, 4, 4, 6, 8, 8, 8, 8};
    int xd[10] = {5, 5, 5, 3, 1, 0, 0, 2, 4, 5};
    int yd[10] = {0, 2, 4, 4, 4, 6, 8, 8, 8, 6};
    int xg[12] = {0, 0, 1, 2, 3, 4, 5, 5, 5, 5, 3, 1};
    int yg[12] = {4, 6, 8, 4, 8, 4, 6, 8, 10, 12, 13, 13};
    int xa[8] = {0, 1, 1, 3, 3, 5, 5, 6};
    int ya[8] = {6, 4, 8, 4, 8, 5, 7, 8};
    int xr[6] = {0, 0, 0, 1, 3, 5};
    int yr[6] = {4, 6, 8, 5, 4, 4};
    int x;
    int y;
    int s;

    x = RandRange(64, g_screenW - 200);
    y = RandRange(0, g_screenH - 200);
    s = RandRange(3, 7);

    DrawDotGlyph(x, y, s, 13, xP, yP);
    DrawDotGlyph(x + s * 9, y, s, 13, xA, yA);
    DrawDotGlyph(x + s * 20, y, s, 9, xY, yY);
    DrawDotGlyph(x + s * 40, y, s, 12, xE, yE);
    DrawDotGlyph(x + s * 52, y, s, 10, xd, yd);
    DrawDotGlyph(x + s * 62, y, s, 12, xg, yg);
    DrawDotGlyph(x + s * 72, y, s, 8, xa, ya);
    DrawDotGlyph(x + s * 82, y, s, 6, xr, yr);
}

// Draws the sprite-based starfield (used for the hyperspace/warp visual, g_state == STATE_MALFUNCTION):
// each star fades/zooms by depth, optionally gets a motion-streak flash sprite when
// moving fast, and is recycled with new depth/position once it scrolls off screen.
void DrawSpriteStars()
{
    float w1;
    float h1;
    float w2;
    float h2;
    float angle;
    bool on;
    int i;
    float alpha;
    float zoom;
    float sp;
    float vel;
    float col;

    w1 = ImgWidth(g_starSprite);
    h1 = ImgHeight(g_starSprite);
    w2 = ImgWidth(g_flashGfx);
    h2 = ImgHeight(g_flashGfx);
    angle = 0;
    on = true;

    for (i = 0; i < g_cfg.numStars; i++) {
        if (g_state == STATE_MALFUNCTION) {
            if (RandRange(0, 100) < 95)
                on = true;
            else
                on = false;
        }
        if (on) {
            alpha = 1.0 / g_stars[i].z + 0.177f > 1.0 ? 1.0 : 1.0 / g_stars[i].z + 0.177f;
            zoom = 0.09f / (g_stars[i].z / 1.2f) + 0.057f;
            sp = g_save.players[g_curPlayer].starSpeed / 4.0;
            vel = 1.4f / (g_stars[i].z * 1.0) * (g_save.players[g_curPlayer].starSpeed / 8.3f);
            col = 1.0 / (g_save.players[g_curPlayer].starSpeed / 30.0) > 1.0 ? 1.0
                : 1.0 / (g_save.players[g_curPlayer].starSpeed / 30.0);
            if (g_state == STATE_MALFUNCTION)
                angle = RandFloat(0, 360.0f);

            if (vel > 1.0 && zoom > 0.084f) {
                ImgSetBlitColor(g_flashGfx, 1.0f, col, col, 1.0f);
                ImgStretchAlphaRect(g_flashGfx, 0, 0, w2, h2,
                    g_stars[i].x - sp * 0.5f + w2 / 2.0f,
                    g_stars[i].y - sp * 8.0f + h2 / 2.0f,
                    g_stars[i].x + sp * 0.5f + w2 / 2.0f,
                    g_stars[i].y + sp * 3.0f + h2 / 2.0f,
                    1.0f, angle + 180.0, false, false, 0, 0.5f);
            }
            if (g_state == STATE_MALFUNCTION) {
                ImgSetBlitColor(g_starSprite, RandFloat(0, 0.5f) + 0.5, 0,
                                RandFloat(0, 0.4f) + 0.2f,
                                1.0 - g_stars[i].z / 16.0);
            } else {
                ImgSetBlitColor(g_starSprite, 1.0f, col + 0.5 > 1.0 ? 1.0 : col + 0.5,
                                col + 0.5 > 1.0 ? 1.0 : col + 0.5, 1.0f);
            }
            ImgBlitAlphaRectFx(g_starSprite, 0, 0, w1, h1, g_stars[i].x, g_stars[i].y,
                               angle, zoom, alpha, false, false, 0, 0);

            if (g_state == STATE_MALFUNCTION && g_state != STATE_PAUSED) {
                g_stars[i].z += 0.058f;
                if (g_stars[i].z > 16.0) {
                    g_stars[i].y = RandFloat(-150.0f, g_screenH);
                    g_stars[i].x = RandFloat(0, g_screenW);
                    g_stars[i].z = 0.1f;
                }
                vel = 1.4f / (g_stars[i].z * 1.0) * 0.83f;
                g_stars[i].y += vel;
            }
            if (g_state != STATE_PAUSED)
                g_stars[i].y += vel;
            if (g_stars[i].y > g_screenH + 40.0f) {
                g_stars[i].y = -50.0f;
                g_stars[i].x = RandFloat(0, g_screenW);
                g_stars[i].z = RandFloat(1.0f, 16.0f);
            }
        }
    }
}

// Draws the flat-shaded (non-sprite) starfield used during normal flight and hyperspace
// (g_state == STATE_MALFUNCTION): color and star count scale with the player's speed and, outside
// hyperspace, with the current background theme (g_bgIndex).
void DrawFlightFrame()
{
    int i;
    int bright;
    int size;
    int r;
    int g;
    int b;
    float fade;
    float shrink;
    float z;
    float sx;
    float sy;
    float div;
    int warp;
    int boost;
    float speed;
    float count;
    float scale;

    if (g_state == STATE_MALFUNCTION) {
        div = g_starZFar - g_starZNear;
        if (div == 0.0)
            div = 1;
        fade = 256.0 / div;
        shrink = 15.0 / div;
    } else {
        div = g_starZFar;
        if (div == 0.0)
            div = 1;
        fade = 250.0 / div;
        shrink = 16.0 / div;
    }

    warp = (int)((0.0 + (g_save.players[g_curPlayer].starSpeed - 5.0) / 2.0 > 255.0)
                     ? 255.0
                     : 0.0 + (g_save.players[g_curPlayer].starSpeed - 5.0) / 2.0);
    boost = (int)(((g_save.players[g_curPlayer].starSpeed - 5.0) / 2.0 < 0.0)
                      ? 0.0
                      : (g_save.players[g_curPlayer].starSpeed - 5.0) / 2.0);

    if (g_state == STATE_MALFUNCTION) {
        r = RandRange(0x32, 0xff);
        g = 0;
        b = 0xff - r;
    } else {
        r = warp;
        g = 0x3c;
        b = (int)(255.0 < ((255.0 - (g_save.players[g_curPlayer].starSpeed - 5.0) / 1.5 < 0.0)
                       ? 0.0
                       : 255.0 - (g_save.players[g_curPlayer].starSpeed - 5.0) / 1.5)
                      ? 255.0
                      : ((255.0 - (g_save.players[g_curPlayer].starSpeed - 5.0) / 1.5 < 0.0)
                             ? 0.0
                             : 255.0 - (g_save.players[g_curPlayer].starSpeed - 5.0) / 1.5));

        if (g_bgIndex == 1) {
            r = 0x6e;
            g = 0x6e;
            b = 0x78;
        }
        if (g_bgIndex == 2) {
            r = 0x78;
            g = 0x50;
            b = 0x8c;
        }
        if (g_bgIndex == 3) {
            r = 0x82;
            g = 0x78;
            b = 0x50;
        }

        if (g_bgIndex == 4) {
            r = 0x5a;
            g = 0x82;
            b = 0x3c;
        }
        if (g_bgIndex == 5) {
            r = 0x1e;
            g = 0x96;
            b = 0x5a;
        }
    }

    speed = g_save.players[g_curPlayer].starSpeed;
    count = (100.0 - (speed - 5.0) * 0.3846154f) / 100.0 * g_cfg.numStars;
    if (count > g_cfg.numStars)
        count = g_cfg.numStars;

    for (i = 0; i < (int)count; i++) {
        z = g_starZ[i];
        if (z == 0.0)
            z = 1;
        sx = g_starX[i] * 75.0 / z + 400.0;
        sy = g_starY[i] * 75.0 / z + 300.0;
        if (g_state == STATE_MALFUNCTION) {
            bright = 0x100 - (int)(z * fade);
            size = 0x10 - (int)(z * shrink);
        } else {
            bright = 0xfa - (int)(z * fade) + boost;
            size = 0x10 - (int)(z * shrink);
        }
        scale = (size > (int)((g_save.players[g_curPlayer].starSpeed - 5.0) / 5.0 + 1.0) * size)
                    ? size
                    : (int)((g_save.players[g_curPlayer].starSpeed - 5.0) / 5.0 + 1.0) * size;
        ImgSetBlitColor(g_starGfx[i], r / 255.0, g / 255.0, b / 255.0, 1.0f);
        ImgSetAlphaMode(g_starGfx[i], 0);
        ImgStretchAlphaRect(g_starGfx[i], 0, 0, 128.0f, 128.0f, sx - 4.0, sy - 4.0, sx + 4.0, sy + 4.0,
                            250.0f, 250.0f, false, false, 0, 0);
    }
}

// Draws a soft glow sprite over each star, sized/faded by depth and by the star's
// remaining "life" (g_starA), used alongside DrawFlightFrame for the glowing-star look.
void DrawStarsGlow()
{
    int i;
    int alpha;
    int life;
    float fade;
    float step;
    float z;
    float sx;
    float sy;
    float div;
    float zoom;
    float w;
    float h;

    div = g_starZFar;
    if (div == 0.0)
        div = 1;
    fade = 155.0 / div;

    for (i = 0; i < (int)g_cfg.numStars; i++) {
        div = g_starZFar;
        if (div == 0.0)
            div = 1;
        step = g_starA[i] / div;
        z = g_starZ[i];
        if (z == 0.0)
            z = 1;
        sx = g_starX[i] * 75.0 / z + 400.0;
        sy = g_starY[i] * 75.0 / z + 300.0;
        alpha = 155 - (int)(z * fade);
        life = (int)(g_starA[i] - z * step);
        zoom = life / 25.0;
        if (zoom > 0.05f) {
            ImgSetBlitColor(g_starGfx[i], 0.5f, 0.75f, 1.0f, alpha / 255.0);
            w = ImgWidth(g_starGfx[i]);
            h = ImgHeight(g_starGfx[i]);
            ImgBlitAlphaRectFx(g_starGfx[i], 0, 0, w, h, (short)sx, (short)sy, 0, zoom, 1.0f,
                               false, false, 0, 0);
        }
    }
}

// Does nothing; used as a placeholder background-draw callback.
void NoOpStars()
{
}

// Fixed tile size of the side-border graphics (g_gfxBorder<difficulty>): the main strip
// is BORDER_TILE_W x BORDER_TILE_H, with a shorter BORDER_BOTTOM_H tile below it in
// static mode.
enum { BORDER_TILE_W = 0x40, BORDER_TILE_H = 0x1e0, BORDER_BOTTOM_H = 0x78 };

// Draws one difficulty's border strip scrolled by `off` (top/mid/bottom copies), left
// and right edge, wrapping every BORDER_TILE_H px.
#define DRAW_BORDER_SCROLL(gfx)                                                             \
    BlitLocal2(0, off - BORDER_TILE_H, 0, gfx, 0, 0, BORDER_TILE_W, BORDER_TILE_H);          \
    BlitLocal2(0, off, 0, gfx, 0, 0, BORDER_TILE_W, BORDER_TILE_H);                          \
    BlitLocal2(0, off + BORDER_TILE_H, 0, gfx, 0, 0, BORDER_TILE_W, BORDER_TILE_H - off);    \
    BlitLocal2(g_screenW - BORDER_TILE_W, off - BORDER_TILE_H, 0, gfx, BORDER_TILE_W, 0,     \
               BORDER_TILE_W, BORDER_TILE_H);                                               \
    BlitLocal2(g_screenW - BORDER_TILE_W, off, 0, gfx, BORDER_TILE_W, 0,                     \
               BORDER_TILE_W, BORDER_TILE_H);                                               \
    BlitLocal2(g_screenW - BORDER_TILE_W, off + BORDER_TILE_H, 0, gfx, BORDER_TILE_W, 0,     \
               BORDER_TILE_W, BORDER_TILE_H - off);

// Draws one difficulty's border strip static (top + bottom tile), left and right edge.
#define DRAW_BORDER_STATIC(gfx)                                                             \
    BlitLocal2(0, 0, 0, gfx, 0, 0, BORDER_TILE_W, BORDER_TILE_H);                            \
    BlitLocal2(0, BORDER_TILE_H, 0, gfx, 0, 0, BORDER_TILE_W, BORDER_BOTTOM_H);              \
    BlitLocal2(g_screenW - BORDER_TILE_W, 0, 0, gfx, BORDER_TILE_W, 0,                       \
               BORDER_TILE_W, BORDER_TILE_H);                                               \
    BlitLocal2(g_screenW - BORDER_TILE_W, BORDER_TILE_H, 0, gfx, BORDER_TILE_W, 0,           \
               BORDER_TILE_W, BORDER_BOTTOM_H);

// Draws the side border graphics for the current difficulty. In scrolling mode
// (borderMode 0) they scroll with the player's speed and wrap every 480px; otherwise
// (borderMode 1/2) they're drawn static, split into a top and bottom tile.
void DrawBorders()
{
    int off;

    if (g_cfg.borderMode == BORDER_ON) {
        if (g_state != STATE_PAUSED)
            g_borderScroll = g_save.players[g_curPlayer].starSpeed / 8.0 + g_borderScroll;
        if (g_borderScroll >= 480.0)
            g_borderScroll = g_borderScroll - 480.0;
        off = (int)g_borderScroll;
        if (g_cfg.difficulty == DIFF_EASY) {
            DRAW_BORDER_SCROLL(g_gfxBorderEasy);
        }

        if (g_cfg.difficulty == DIFF_NORMAL) {
            DRAW_BORDER_SCROLL(g_gfxBorderNormal);
        }

        if (g_cfg.difficulty == DIFF_HARD) {
            DRAW_BORDER_SCROLL(g_gfxBorderHard);
        }

        if (g_cfg.difficulty == DIFF_ACE) {
            DRAW_BORDER_SCROLL(g_gfxBorderAce);
        }
    }

    // ---- static borders (top + bottom tile, one per difficulty) ----
    if (g_cfg.borderMode == BORDER_OFF || g_cfg.borderMode == BORDER_BLACK) {
        if (g_cfg.difficulty == DIFF_EASY) {
            DRAW_BORDER_STATIC(g_gfxBorderEasy);
        }

        if (g_cfg.difficulty == DIFF_NORMAL) {
            DRAW_BORDER_STATIC(g_gfxBorderNormal);
        }

        if (g_cfg.difficulty == DIFF_HARD) {
            DRAW_BORDER_STATIC(g_gfxBorderHard);
        }

        if (g_cfg.difficulty == DIFF_ACE) {
            DRAW_BORDER_STATIC(g_gfxBorderAce);
        }
    }
}

#undef DRAW_BORDER_SCROLL
#undef DRAW_BORDER_STATIC
