#pragma once
// Statement macros shared by several modules. The original evidently had macros like these:
// the same statement sequences recur verbatim across files. Each one expands to exactly the
// statements that were written out at its call sites, so using it doesn't change the code.
// Macros used by a single module stay local to that module's .c file.
//
// Included from game.h; the names they use (globals, functions) are resolved where they expand.


// Zeroes every render-queue counter (the inlined body of ResetBlitCounters()). Screens do
// this before switching to a new frame function so no stale queued blits get flushed.
#define CLEAR_DRAW_COUNTERS() \
    g_blitCount = 0;          \
    g_blit2Count = 0;         \
    g_quadCount = 0;          \
    g_stretchFCount = 0;      \
    g_stretchRotCount = 0;    \
    g_stretchRot2Count = 0;


// ---------------------------------------------------------------------------------------
// Score
// ---------------------------------------------------------------------------------------
// Adds `add` times player `pl`'s score multiplier to `score` (an lvalue).
#define ADD_PLAYER_SCORE(score, pl, add) ((score) += (__int64)(add) * g_scoreMul[pl])


// ---------------------------------------------------------------------------------------
// Clip a sprite to the clip rect before queueing it
// ---------------------------------------------------------------------------------------
// True when the rect (x,y,w,h) overlaps the current clip rect at all.
#define CLIP_VISIBLE(x, y, w, h) \
    ((x) < g_clipRight && (y) < g_clipBottom && (x) + (w) > g_clipLeft && (y) + (h) > g_clipTop)

// Clips (x,y,w,h) to the clip rect, moving the source rect `src`'s top-left by the amount cut
// off the left/top, then sets src's bottom-right from the clipped size.
#define CLIP_SRC_RECT(x, y, w, h, src)          \
    if ((x) < g_clipLeft) {                     \
        (src).x1 = g_clipLeft - (x) + (src).x1; \
        (w) = (w) - (g_clipLeft - (x));         \
        (x) = g_clipLeft;                       \
    } else if ((x) + (w) >= g_clipRight) {      \
        (w) = g_clipRight - (x);                \
    }                                           \
    if ((y) < g_clipTop) {                      \
        (src).y1 = g_clipTop - (y) + (src).y1;  \
        (h) = (h) - (g_clipTop - (y));          \
        (y) = g_clipTop;                        \
    } else if ((y) + (h) >= g_clipBottom) {     \
        (h) = g_clipBottom - (y);               \
    }                                           \
    (src).x2 = (src).x1 + (w);                  \
    (src).y2 = (src).y1 + (h);


// ---------------------------------------------------------------------------------------
// Turn a rotating sprite toward a target (level objects, map objects)
// ---------------------------------------------------------------------------------------
// Turns `frame` (a sprite frame 1..32) one step toward the quadrant of (tx,ty) relative to
// (ox,oy), using the caller's `quadtab[11]` (quadrant bits -> frame); a tie turns a random
// way. Needs the caller's int locals `dir`, `want`, `d1`, `d2`.
#define TURN_TOWARD_QUADRANT(frame, ox, oy, tx, ty) \
    dir = 0;                                        \
    if ((tx) >= (ox))                               \
        dir |= 2;                                   \
    if ((tx) < (ox))                                \
        dir |= 1;                                   \
    if ((ty) < (oy))                                \
        dir |= 8;                                   \
    if ((ty) >= (oy))                               \
        dir |= 4;                                   \
    want = quadtab[dir];                            \
                                                    \
    if ((frame) != want) {                          \
        if ((d1 = want - (frame)) < 0)              \
            d1 += 32;                               \
        if ((d2 = (frame) - want) < 0)              \
            d2 += 32;                               \
        if (d1 == d2) {                             \
            if (RandRange(0, 100) < 50)             \
                (frame)--;                          \
            else                                    \
                (frame)++;                          \
            if ((frame) < 1)                        \
                (frame) = 32;                       \
            if ((frame) > 32)                       \
                (frame) = 1;                        \
        } else if (d1 < d2) {                       \
            (frame)++;                              \
            if ((frame) > 32)                       \
                (frame) = 1;                        \
        } else {                                    \
            (frame)--;                              \
            if ((frame) < 1)                        \
                (frame) = 32;                       \
        }                                           \
    }


// ---------------------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------------------
// Leaves a full-screen view (bonus stage, shop, freeze...) back to the gameplay HUD view.
#define RETURN_TO_HUD_VIEW()        \
    g_moneyBlinkTimer = 0;          \
    g_viewTransitionFlag = 2;       \
    g_stateFn = SetViewHud;         \
    g_drawBordersFn = DrawBorders;

// Clears an account's medal award order (7 slots).
#define CLEAR_MEDAL_ORDER(acc) \
    (acc).medalOrder[0] = 0;   \
    (acc).medalOrder[1] = 0;   \
    (acc).medalOrder[2] = 0;   \
    (acc).medalOrder[3] = 0;   \
    (acc).medalOrder[4] = 0;   \
    (acc).medalOrder[5] = 0;   \
    (acc).medalOrder[6] = 0;

// Restores player `dst`'s hyperspace/scroll-speed state from the copy player `src` saved
// before entering a bonus stage (both are Player lvalues).
#define RESTORE_HYPERSPACE(dst, src)                                  \
    (dst).hyperspaceOutTimer = (src).savedHyperspaceOutTimer;         \
    (dst).hyperspaceMidTimer = (src).savedHyperspaceMidTimer;         \
    (dst).hyperspaceInTimer = (src).savedHyperspaceInTimer;           \
    (dst).hyperspaceInDuration = (src).savedHyperspaceInDuration;     \
    (dst).hyperspaceFade = (src).savedHyperspaceFade;                 \
    (dst).scrollSpeedY = (src).savedScrollSpeedY;                     \
    (dst).starSpeed = (src).savedStarSpeed;                           \
    (dst).starVelZ = (src).savedStarVelZ;
