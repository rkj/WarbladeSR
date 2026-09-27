// render.cpp: Drawing primitives: the batched blit/stretch/quad queues and their flushes, clipped
// blits, immediate draws, screen clear and flip, draw counters.
#include "globals.h"
#include "game.h"

enum {
    // KGraphic::_field04 value once the object has been freed; also MSVC's debug-heap
    // freed-memory fill byte (0xfeeefeee repeated), so a freed graphic reads as this by
    // accident as much as by design. Queued draws check for it and skip the entry.
    FREED_GFX_SENTINEL = 0xfeeefeee,
    // Sanity bound on a queued blit's source rect right edge (QueueBlit/QueueBlit2).
    BLIT_SRC_MAX       = 2000
};


// Blits the whole of `g` stretched into (x1,y1)-(x2,y2), tinted `r,gr,b` (0-255) at alpha
// `a` (0-255) and rotated by `angle`. No-op if `g` is NULL.
void DrawImage(KGraphic* g, int x1, int y1, int x2, int y2, unsigned char r, unsigned char gr,
               unsigned char b, unsigned char a, int unused, float angle)
{
    if (!g)
        return;
    float w = g->getWidth();
    float h = g->getHeight();
    g->setBlitColor(r / 255.0, gr / 255.0, b / 255.0, 1.0f);
    g->stretchAlphaRect(0, 0, w, h, x1, y1, x2, y2, a / 255.0, angle, false, false, 0, 0);
}

// Queues a whole-graphic stretch to an integer destination rect, tinted (r,g,b,a).
void QueueStretchI(KGraphic *graphic, float x1, float y1, float x2, float y2,
                          unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    if (g_stretchICount < MAX_STRETCH_I - 1) {
        StretchItemI *p = &g_stretchI[g_stretchICount];
        p->graphic = graphic;
        p->x1 = (int)x1;
        p->y1 = (int)y1;
        p->x2 = (int)x2;
        p->y2 = (int)y2;
        p->r = r;
        p->g = g;
        p->b = b;
        p->a = a;
        g_stretchICount++;
    }
}

// Queues a whole-graphic stretch to a float destination rect, tinted (r,g,b,a), with
// an extra `flag` (blend/mirror mode passed through to stretchAlphaRect on flush).
void QueueStretchF(KGraphic *graphic, float x1, float y1, float x2, float y2,
                          unsigned char r, unsigned char g, unsigned char b, unsigned char a,
                          unsigned char flag)
{
    if (g_stretchFCount < MAX_STRETCH_F - 1) {
        StretchItemF *p = &g_stretchF[g_stretchFCount];
        p->graphic = graphic;
        p->x1 = x1;
        p->y1 = y1;
        p->x2 = x2;
        p->y2 = y2;
        p->r = r;
        p->g = g;
        p->b = b;
        p->a = a;
        p->flag = flag;
        g_stretchFCount++;
    }
}

// Queues a whole-graphic stretch, tinted and rotated by `angle` degrees (main queue,
// 999 entries: particles, flame overlays, most rotated sprites).
void QueueStretchRot(KGraphic *graphic, float x1, float y1, float x2, float y2,
                            unsigned char r, unsigned char g, unsigned char b, unsigned char a,
                            unsigned char flag, float angle)
{
    if (g_stretchRotCount < MAX_STRETCH_ROT - 1) {
        StretchItemRot *p = &g_stretchRot[g_stretchRotCount];
        p->graphic = graphic;
        p->x1 = x1;
        p->y1 = y1;
        p->x2 = x2;
        p->y2 = y2;
        p->r = r;
        p->g = g;
        p->b = b;
        p->a = a;
        p->flag = flag;
        p->angle = angle;
        g_stretchRotCount++;
    }
}

// Same as QueueStretchRot() but into a separate, much smaller queue (19 entries),
// flushed independently by FlushStretchRot2().
void QueueStretchRot2(KGraphic *graphic, float x1, float y1, float x2, float y2,
                             unsigned char r, unsigned char g, unsigned char b, unsigned char a,
                             unsigned char flag, float angle)
{
    if (g_stretchRot2Count < MAX_STRETCH_ROT2 - 1) {
        StretchItemRot *p = &g_stretchRot2[g_stretchRot2Count];
        p->graphic = graphic;
        p->x1 = x1;
        p->y1 = y1;
        p->x2 = x2;
        p->y2 = y2;
        p->r = r;
        p->g = g;
        p->b = b;
        p->a = a;
        p->flag = flag;
        p->angle = angle;
        g_stretchRot2Count++;
    }
}

// Queues an unscaled, untinted blit of `src` (a source rect on `graphic`) to
// (destX, destY). Rejects (counting g_blitErrors) an empty or out-of-range source rect
// instead of queuing it — a sanity check against bad tile/sprite rects.
void QueueBlit(float destX, float destY, KGraphic *graphic, Rect16 *src)
{
    if (g_blitCount < MAX_BLITS - 1) {
        if (src->x1 == src->x2) {
            g_blitErrors++;
        } else if (src->x1 < 0) {
            g_blitErrors++;
        } else if (src->x2 > BLIT_SRC_MAX) {
            g_blitErrors++;
        } else {
            BlitItem *p = &g_blit[g_blitCount];
            memcpy(p, src, 16);
            p->destX = destX;
            p->destY = destY;
            p->graphic = graphic;
            g_blitCount++;
        }
    }
}

// Same as QueueBlit() but into a second, separately-flushed queue.
void QueueBlit2(float destX, float destY, KGraphic *graphic, Rect16 *src)
{
    if (g_blit2Count < MAX_BLITS - 1) {
        if (src->x1 == src->x2) {
            g_blitErrors++;
        } else if (src->x1 < 0) {
            g_blitErrors++;
        } else if (src->x2 > BLIT_SRC_MAX) {
            g_blitErrors++;
        } else {
            BlitItem *p = &g_blit2[g_blit2Count];
            memcpy(p, src, 16);
            p->destX = destX;
            p->destY = destY;
            p->graphic = graphic;
            g_blit2Count++;
        }
    }
}

// Draws and empties the QueueStretchI() queue. Each entry is skipped if its graphic
// is null or has been freed (_field04 == FREED_GFX_SENTINEL is the freed-object sentinel).
void FlushStretchI()
{
    if (g_stretchICount > 0) {
        StretchItemI *p = g_stretchI;
        for (int i = 0; i < g_stretchICount; i++) {
            if (p->graphic && p->graphic->_field04 != FREED_GFX_SENTINEL) {
                p->graphic->setBlitColor(p->r / 255.0, p->g / 255.0, p->b / 255.0, 1.0f);
                float w = p->graphic->getWidth();
                float h = p->graphic->getHeight();
                p->graphic->setTextureQuality(true);
                p->graphic->stretchAlphaRect(0, 0, w, h, p->x1, p->y1, p->x2, p->y2,
                                             p->a / 255.0, 0, false, false, 0, 0);
            }
            p++;
        }
        g_stretchICount = 0;
    }
}

// Draws and empties the QueueStretchF() queue.
void FlushStretchF()
{
    if (g_stretchFCount > 0) {
        StretchItemF *p = g_stretchF;
        for (int i = 0; i < g_stretchFCount; i++) {
            if (p->graphic && p->graphic->_field04 != FREED_GFX_SENTINEL) {
                p->graphic->setBlitColor(p->r / 255.0, p->g / 255.0, p->b / 255.0, 1.0f);
                float w = p->graphic->getWidth();
                float h = p->graphic->getHeight();
                p->graphic->stretchAlphaRect(0, 0, w, h, p->x1, p->y1, p->x2, p->y2,
                                             p->a / 255.0, 0, false, false, 0, 0);
            }
            p++;
        }
        g_stretchFCount = 0;
    }
}

// Draws and empties the QueueStretchRot() queue.
void FlushStretchRot()
{
    if (g_stretchRotCount > 0) {
        StretchItemRot *p = g_stretchRot;
        for (int i = 0; i < g_stretchRotCount; i++) {
            if (p->graphic && p->graphic->_field04 != FREED_GFX_SENTINEL) {
                p->graphic->setBlitColor(p->r / 255.0, p->g / 255.0, p->b / 255.0, 1.0f);
                float w = p->graphic->getWidth();
                float h = p->graphic->getHeight();
                p->graphic->setTextureQuality(true);
                p->graphic->stretchAlphaRect(0, 0, w, h, p->x1, p->y1, p->x2, p->y2,
                                             p->a / 255.0, p->angle, false, false, 0, 0);
            }
            p++;
        }
        g_stretchRotCount = 0;
    }
}

// Draws and empties the QueueStretchRot2() queue.
void FlushStretchRot2()
{
    if (g_stretchRot2Count > 0) {
        StretchItemRot *p = g_stretchRot2;
        for (int i = 0; i < g_stretchRot2Count; i++) {
            if (p->graphic && p->graphic->_field04 != FREED_GFX_SENTINEL) {
                p->graphic->setBlitColor(p->r / 255.0, p->g / 255.0, p->b / 255.0, 1.0f);
                float w = p->graphic->getWidth();
                float h = p->graphic->getHeight();
                p->graphic->stretchAlphaRect(0, 0, w, h, p->x1, p->y1, p->x2, p->y2,
                                             p->a / 255.0, p->angle, false, false, 0, 0);
            }
            p++;
        }
        g_stretchRot2Count = 0;
    }
}

// Draws and empties the QueueBlit() queue. `dst` is unused. Skips entries with an
// empty source rect (redundant with the check already done in QueueBlit()).
void FlushBlit(void* dst)
{
    if (g_blitCount > 0) {
        BlitItem *p = g_blit;
        for (int i = 0; i < g_blitCount; i++) {
            if (p->graphic && p->src.y2 != p->src.y1 && p->src.x1 != p->src.x2 &&
                p->graphic->_field04 != FREED_GFX_SENTINEL) {
                p->graphic->setTextureQuality(true);
                p->graphic->blitRectF(p->src.x1, p->src.y1, p->src.x2, p->src.y2,
                                          p->destX, p->destY, false, false);
            }
            p++;
        }
        g_blitCount = 0;
    }
}

// Draws and empties the QueueBlit2() queue.
void FlushBlit2(void* dst)
{
    if (g_blit2Count > 0) {
        BlitItem *p = g_blit2;
        for (int i = 0; i < g_blit2Count; i++) {
            if (p->graphic && p->src.y2 != p->src.y1 && p->src.x1 != p->src.x2 &&
                p->graphic->_field04 != FREED_GFX_SENTINEL) {
                p->graphic->setTextureQuality(true);
                p->graphic->blitRectF(p->src.x1, p->src.y1, p->src.x2, p->src.y2,
                                          p->destX, p->destY, false, false);
            }
            p++;
        }
        g_blit2Count = 0;
    }
}

// Queues a source-rect-to-dest-rect stretch (dest x,y,w,h then source x,y,w,h) using
// `param` as the graphic; `unused` is unused. Also tracks the queue's high-water mark
// in g_quadCountMax.
void QueueQuad(KGraphic* unused, float dx, float dy, float dw, float dh,
                     KGraphic* graphic, float sx, float sy, float sw, float sh)
{
    // NOTE: g_quad[] is declared with only 100 entries but this cap allows up to 149;
    // the original game has this same overrun (kept as-is for the byte match).
    if (g_quadCount < 149) {
        QuadItem *p = &g_quad[g_quadCount];
        p->dx = dx;
        p->dy = dy;
        p->dw = dw;
        p->dh = dh;
        p->sx = sx;
        p->sy = sy;
        p->sw = sw;
        p->sh = sh;
        p->graphic = graphic;
        g_quadCount++;
        if (g_quadCount > g_quadCountMax) {
            g_quadCountMax = g_quadCount;
        }
    }
}

// Draws and empties the QueueQuad() queue. `dst` is unused.
void FlushQuads(void* dst)
{
    if (g_quadCount > 0) {
        QuadItem *p = g_quad;
        for (int i = 0; i < g_quadCount; i++) {
            if (p->graphic && p->graphic->_field04 != FREED_GFX_SENTINEL) {
                p->graphic->stretchAlphaRect(p->sx, p->sy, p->sx + p->sw, p->sy + p->sh,
                                             p->dx, p->dy, p->dx + p->dw, p->dy + p->dh,
                                             1.0f, 0, false, false, 0, 0);
            }
            p++;
        }
        g_quadCount = 0;
    }
}

// Draws a stretch immediately (not queued) from source rect (sx,sy,sw,sh) to
// destination rect (dx,dy,dw,dh), untinted.
void DrawStretch(KGraphic *graphic, int dx, int dy, int dw, int dh,
                        int sx, int sy, int sw, int sh)
{
    graphic->stretchAlphaRect(sx, sy, sx + sw, sy + sh, dx, dy, dx + dw, dy + dh,
                              1.0f, 0, false, false, 0, 0);
}

// Resets the per-frame blit/draw-call counters (debug/profiling stats).
void ResetF893()
{
    g_blitCount = 0;
    g_blit2Count = 0;
    g_quadCount = 0;
    g_stretchFCount = 0;
    g_stretchRotCount = 0;
}

// Resets the global clip rectangle to the full screen.
bool ResetClip()
{
    g_clipTop = 0;
    g_clipBottom = g_screenH;
    g_clipLeft = 0;
    g_clipRight = g_screenW;
    return true;
}

// Clears the per-frame blit/quad/stretch draw-call counters (profiling counters, reset
// once each frame after the counts are consumed).
void ResetBlitCounters()
{
    CLEAR_DRAW_COUNTERS();
}

// Blits a w x h region of `graphic` at (sx, sy) to (x, y), clipped to the current clip
// rect (g_clipLeft/Top/Right/Bottom), and queues it via QueueBlit(). No-op if fully
// outside the clip rect. `dst` is accepted but unused (QueueBlit always targets the
// active render target).
void Blit(int x, int y, KGraphic* dst, KGraphic* graphic, int sx, int sy, int w, int h)
{
    g_blitSrc.x1 = sx;
    g_blitSrc.y1 = sy;
    if (!CLIP_VISIBLE(x, y, w, h))
        return;
    CLIP_SRC_RECT(x, y, w, h, g_blitSrc);
    QueueBlit((float)x, (float)y, graphic, &g_blitSrc);
}

// Like Blit(), but queues the blit with QueueBlit2() (a separate draw list/layer).
void Blit2(int x, int y, void* dst, KGraphic* graphic, int sx, int sy, int w, int h)
{
    g_blitSrc.x1 = sx;
    g_blitSrc.y1 = sy;
    if (!CLIP_VISIBLE(x, y, w, h))
        return;
    CLIP_SRC_RECT(x, y, w, h, g_blitSrc);
    QueueBlit2((float)x, (float)y, graphic, &g_blitSrc);
}

// Same clipping/blit logic as Blit(), with the clip rect worked out in local variables
// (dx, dy) instead of reusing the caller's x/y; result goes through QueueBlit().
void BlitLocal(int x, int y, void* dst, KGraphic* graphic, int sx, int sy, int w, int h)
{
    Rect16 src;
    int dx;
    int dy;

    dx = x;
    dy = y;
    src.x1 = sx;
    src.y1 = sy;
    if (!CLIP_VISIBLE(dx, dy, w, h))
        return;
    CLIP_SRC_RECT(dx, dy, w, h, src);
    QueueBlit((float)dx, (float)dy, graphic, &src);
}

// Same as BlitLocal(), but queues the blit with QueueBlit2() (a separate draw list/layer).
void BlitLocal2(int x, int y, KGraphic* dst, KGraphic* graphic, int sx, int sy, int w, int h)
{
    Rect16 src;
    int dx;
    int dy;

    dx = x;
    dy = y;
    src.x1 = sx;
    src.y1 = sy;
    if (!CLIP_VISIBLE(dx, dy, w, h))
        return;
    CLIP_SRC_RECT(dx, dy, w, h, src);
    QueueBlit2((float)dx, (float)dy, graphic, &src);
}

// Presents the back buffer to the screen. `a` is unused.
void FlipBuffer(int a)
{
    g_window->flipBackBuffer(true, true);
}

// Clears the HUD play-field area (excluding the 64px side borders) to black, or to the
// current flash-fade color while a screen flash is in progress (advancing its fade step).
void SetViewHud()
{
    if (g_cfg.bgEnabled == 1) return;
    if (g_flashOverlayActive == 0) {
        g_screen->drawRect(64.0f, 0.0f, g_screenW - 64.0, (float)g_screenH, 0.0f, 0.0f, 0.0f, 1.0f);
    } else {
        g_screen->drawRect(64.0f, 0.0f, g_screenW - 64.0, (float)g_screenH,
            ((float *)&g_colR[g_fadeColorSet])[g_fadeStep],
            ((float *)&g_colG[g_fadeColorSet])[g_fadeStep],
            ((float *)&g_colB[g_fadeColorSet])[g_fadeStep], 1.0f);
        g_fadeStep++;
        if (g_fadeStep == 30) g_flashOverlayActive = 0;
    }
}

// Clears the whole screen to black, or to the current flash-fade color while a screen
// flash is in progress (advancing its fade step). Same as SetViewHud but without the
// HUD side borders excluded.
void SetView()
{
    if (g_flashOverlayActive == 0) {
        g_screen->drawRect(0.0f, 0.0f, (float)g_screenW, (float)g_screenH, 0.0f, 0.0f, 0.0f, 1.0f);
    } else {
        g_screen->drawRect(0.0f, 0.0f, (float)g_screenW, (float)g_screenH,
            ((float *)&g_colR[g_fadeColorSet])[g_fadeStep],
            ((float *)&g_colG[g_fadeColorSet])[g_fadeStep],
            ((float *)&g_colB[g_fadeColorSet])[g_fadeStep], 1.0f);
        g_fadeStep++;
        if (g_fadeStep == 30) g_flashOverlayActive = 0;
    }
}
