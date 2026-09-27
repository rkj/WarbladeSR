// frames.cpp: Per-frame render functions for gameplay and transitions (the stages have their own),
// the flash overlay.
#include "globals.h"
#include "game.h"


// Draws the fading colored flash overlay used for hit/warning feedback, when active and
// background effects are enabled.
void DrawFlash()
{
    if (g_cfg.bgEnabled != 1) return;
    if (g_flashOverlayActive == 0) return;
    else {
        float alpha = 1.0 - g_fadeStep / 30.0;   // 30 frames of fade-out
        g_gfxLogos->setAlphaMode(4);
        g_gfxLogos->setBlitColor(g_colR[g_fadeColorSet].step0, g_colG[g_fadeColorSet].step0,
                                  g_colB[g_fadeColorSet].step0, alpha);
        g_gfxLogos->blitAlphaRectFx(128.0f, 158.0f, 160.0f, 190.0f,
                                           (float)(g_screenW / 2), (float)(g_screenH / 2),
                                           0.0f, 25.0f, 1.0f, false, false, 0.0f, 0.0f);
        g_fadeStep++;
        if (g_fadeStep == 30) g_flashOverlayActive = 0;
        g_gfxLogos->setBlitColor(1.0f, 1.0f, 1.0f, 1.0f);
        g_gfxLogos->setAlphaMode(1);
    }
}

// Per-frame draw: game state, background, level slots, per-state extra draw, ship HUD, flash
// overlay, borders and HUD, via the current state's function pointers.
void Frame()
{
    g_stateFn();
    DrawBackground();
    DrawSlots(g_clipLeft, g_clipRight, g_clipTop, g_clipBottom);
    g_fnPtr();
    g_shipHudFn();
    DrawFlash();
    g_drawBordersFn();
    g_drawHudFn();
}

// Advances the three slow color-cycle counters (g_colorPhase1/2/3, used for
// palette-cycling effects), each on its own tick-down timer (g_animTickA/B/C), while
// gameplay isn't paused.
#define COLOR_CYCLE_TICK()                     \
    if (g_state != STATE_PAUSED) {             \
        if (!g_animTickA--) {                  \
            g_animTickA = 1;                   \
            g_colorPhase1++;                   \
            if (g_colorPhase1 > 256) g_colorPhase1 = 0; \
        }                                       \
        if (!g_animTickB--) {                  \
            g_animTickB = 2;                   \
            g_colorPhase2++;                   \
            if (g_colorPhase2 > 256) g_colorPhase2 = 0; \
        }                                       \
        if (!g_animTickC--) {                  \
            g_animTickC = 3;                   \
            g_colorPhase3++;                   \
            if (g_colorPhase3 > 256) g_colorPhase3 = 0; \
        }                                       \
    }

// Renders one frame of gameplay: advances the three slow color-cycle counters (used for
// palette-cycling effects), then draws background, slots, sprites/enemies/debris, map
// objects, per-mode HUD, explosions, popups, particles, flash overlay, boss bar (on boss
// levels), borders, and finally the warp ring / HUD via the mode's function pointers.
void RenderGameplayFrame()
{
    COLOR_CYCLE_TICK();

    g_stateFn();
    DrawBackground();
    DrawSlots(g_clipLeft, g_clipRight, g_clipTop, g_clipBottom);
    g_fnPtr();
    if (g_flag) UpdateSparks();
    DrawSprites();
    DrawEnemies();
    DrawExplosionDebris();
    g_drawLevelObjectsFn();
    DrawMapObjects();
    g_shipHudFn();

    DrawExplosions();
    DrawScorePopups();
    UpdateBonusResultsHud();
    DrawParticles();
    DrawFlash();
    if (g_curLevelData.type == 4) DrawBossBar();
    g_drawBordersFn();
    if (g_state == STATE_PAUSED) DrawWarpRing();
    g_drawHudFn();
}

// Alternate per-frame gameplay renderer (used by some screens in place of RenderGameplayFrame):
// same color-cycle bookkeeping, but draws map objects/sprites/debris/enemies before the level
// objects and HUD, and skips particles, flash overlay and the boss bar.
void RenderFrame2()
{
    COLOR_CYCLE_TICK();

    g_stateFn();
    DrawBackground();
    DrawSlots(g_clipLeft, g_clipRight, g_clipTop, g_clipBottom);
    g_fnPtr();
    if (g_flag) UpdateSparks();
    DrawMapObjects();
    DrawSprites();
    DrawExplosionDebris();
    DrawEnemies();
    g_drawLevelObjectsFn();
    g_shipHudFn();

    DrawExplosions();
    DrawScorePopups();
    UpdateBonusResultsHud();
    g_drawBordersFn();
    g_drawHudFn();
}

#undef COLOR_CYCLE_TICK

// Per-frame render for the shop-gate transition: background, sparks, explosion debris
// and explosions, ship HUD, bonus HUD, particles, flash, the rank-promotion HUD (once the
// transition has moved past state 0), warp ring, borders and HUD.
void RenderShopGateFrame()
{
    g_stateFn();
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
    DrawExplosionDebris();
    DrawExplosions();
    g_shipHudFn();
    UpdateBonusResultsHud();
    DrawParticles();
    DrawFlash();
    if (g_state != STATE_PAUSED)
        DrawRankPromoHud();
    if (g_state == STATE_PAUSED)
        DrawWarpRing();
    g_drawBordersFn();
    g_drawHudFn();
}
