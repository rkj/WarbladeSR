// promotion.cpp: Rank promotion: the in-game banner, the promotion screen and its fanfare.
#include <stdio.h>
#include "globals.h"
#include "game.h"


// Draws the "CONGRATULATIONS / YOU ARE HEREBY PROMOTED TO <rank>" banner shown when
// the current player is promoted to a new rank.
void DrawRankPromoBanner()
{
    int y;
    y = 450;
    DrawMenuText("C O N G R A T U L A T I O N S", POS_CENTERED, y - 30, RandRange(0, 4));
    DrawMenuText("YOU ARE HEREBY PROMOTED TO", POS_CENTERED, y - 15, RandRange(0, 4));
    sprintf(g_logBuf, "%s", g_rankNames[g_save.players[g_curPlayer].rank]);
    DrawMenuText(g_logBuf, POS_CENTERED, y + 10, RandRange(0, 4));
    Blit((g_screenW >> 1) - 32, y + 30, g_screen, g_gfxRanks, 0,
                      g_rankSprY[g_save.players[g_curPlayer].rank], 64, 13);
}

// Draws the rank-promotion screen for g_promoPlayer: for special ranks (>20) it shows a
// rotating sword/bird/ring effect matching the rank; otherwise it shows the "CONGRATULATIONS
// / PROMOTED TO <rank>" text with the rank badge and any row decorations, plus background
// sparks/scanlines. Gated on g_hiscoreSkipGateActive so it only renders once that screen
// is actually showing.
void DrawRankPromoHud()
{
    int n = 60;
    int y = 240;
    int w = 500;
    int r;
    int g;
    int b;
    int h;

    g_promoSpecialRank = 0;
    g_promoRingActive = 0;
    if (g_hiscoreSkipGateActive != 0) {
    if (g_rankMsgActive != 0) {
        if (g_time > g_promoBlinkTimer) {
            g_promoBlinkTimer = g_time + 400;
            g_promoContinueBlink = !g_promoContinueBlink;
        }
        if (g_promoContinueBlink) {
            DrawMenuText("PRESS FIRE TO CONTINUE", POS_CENTERED, g_screenH - 70, 1);
        }
    }

    if (g_save.players[g_promoPlayer].rank > RANK_GRANDMASTER_3) {
        g_promoSpecialRank = 1;
        if (g_save.players[g_promoPlayer].rank == RANK_CHAMPION) {
            h = 400;
            r = 255;
            g = 255;
            b = 255;
            QueueStretchI(g_gfxSword, 400.0 - (w >> 1), (float)y - (h >> 1),
                400.0 + (w >> 1), (float)y + (h >> 1),
                255, 255, 255, 255);
        }

        if (g_save.players[g_promoPlayer].rank == RANK_GOD) {
            r = 255;
            g = 0;
            b = 220;
            QueueStretchI(g_gfxBird, 400.0 - (w >> 1), (float)y - (w >> 1),
                400.0 + (w >> 1), (float)y + (w >> 1),
                255, 255, 255, 255);
        }

        if (g_save.players[g_promoPlayer].rank > RANK_GOD) {
            g_promoRingIndex = 9 - (g_save.players[g_promoPlayer].rank - RANK_GOD_PLUTO);
            int ry = 240;
            int rx = 500;
            int rr = g_ringR[g_promoRingIndex];
            int rg = g_ringG[g_promoRingIndex];
            int rb = g_ringB[g_promoRingIndex];
            int rs = g_ringSize[g_promoRingIndex];
            int cy2 = ry;
            QueueStretchRot(g_gfxFlarePlanet, 400.0 - (rs >> 1), (float)(cy2 - (rs >> 1)),
                (rs >> 1) + 400.0, (float)((rs >> 1) + cy2),
                rr, rg, rb, 255, 0, g_angle0);
            QueueStretchRot(g_gfxFlarePlanet, 400.0 - (rs >> 1), (float)(cy2 - (rs >> 1)),
                (rs >> 1) + 400.0, (float)((rs >> 1) + cy2),
                rr, rg, rb, 255, 0, g_angle1);
            QueueStretchRot(g_gfxFlarePlanet, 400.0 - (rs >> 1), (float)(cy2 - (rs >> 1)),
                (rs >> 1) + 400.0, (float)((rs >> 1) + cy2),
                rr, rg, rb, 255, 0, g_angle2);

            g_angle0 += 0.15f;
            if (g_angle0 > 360.0)
                g_angle0 -= 360.0;
            g_angle1 += 0.3f;
            if (g_angle1 > 360.0)
                g_angle1 -= 360.0;
            g_angle2 += 0.6f;
            if (g_angle2 > 360.0)
                g_angle2 -= 360.0;
            g_promoRingActive = 1;
        }

        if (!g_promoRingActive) {
            int s = 500;
            int cy3 = y;
            QueueStretchRot(g_gfxFlare16, 400.0 - (s >> 1), (float)cy3 - (s >> 1),
                400.0 + (s >> 1), (float)cy3 + (s >> 1),
                r, g, b, 128, 0, g_angle0);
            s = 700;
            QueueStretchRot(g_gfxFlare16, 400.0 - (s >> 1), (float)cy3 - (s >> 1),
                400.0 + (s >> 1), (float)cy3 + (s >> 1),
                r, g, b, 64, 0, g_angle1);
            s = 900;
            QueueStretchRot(g_gfxFlare16, 400.0 - (s >> 1), (float)cy3 - (s >> 1),
                400.0 + (s >> 1), (float)cy3 + (s >> 1),
                r, g, b, 32, 0, g_angle2);

            g_angle0 += 0.1f;
            if (g_angle0 > 360.0)
                g_angle0 -= 360.0;
            g_angle1 -= 0.3f;
            if (g_angle1 < 0.0)
                g_angle1 += 360.0;
            g_angle2 += 0.8f;
            if (g_angle2 > 360.0)
                g_angle2 -= 360.0;
        }
    } else {
        int cy = (int)g_screenH / 2;
        if (g_cfg.particlesOn == 0) {
            for (int i = 0; i < n; i++) {
                g_screen->drawLine(100.0f, (float)(cy - n * 3 / 2 + i * 3), g_screenW - 100.0,
                    (float)(cy - n * 3 / 2 + i * 3), (i * 3 + 50) / 255.0, 0, 0.5f, 1.0f);
            }
        } else if (g_time > g_nextSpark) {
            g_nextSpark = g_time + 8;
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) - 100, 3.0f, 0, 90.0f, 0, 0, 0, 0, 255, 70,
                               150.0f, 0, -1, RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) + 100, 3.0f, 0, 90.0f, 0, 0, 0, 0, 255, 70,
                               150.0f, 0, -1, 0 - RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);

            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) - 101, 2.0f, 0, 90.0f, 0, 0, 128, 0, 200, 50,
                               80.0f, 0, -1, 0 - RandFloat(0.01f, 0.02f), 1, 0, &g_introGateScratch, 0);
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) + 101, 2.0f, 0, 90.0f, 0, 0, 128, 0, 200, 80,
                               40.0f, 0, -1, RandFloat(0.01f, 0.02f), 1, 0, &g_introGateScratch, 0);
        }

        DrawMenuText("C O N G R A T U L A T I O N S", POS_CENTERED, cy - 55, 1);
        DrawMenuText("YOU ARE HEREBY PROMOTED TO", POS_CENTERED, cy - 28, 1);
        sprintf(g_logBuf, "%s", g_rankNames[g_save.players[g_promoPlayer].rank]);
        DrawMenuText(g_logBuf, POS_CENTERED, cy + 5, 2);
        Blit((g_screenW >> 1) - 32, cy + 40, g_screen, g_gfxRanks, 0,
                          g_rankSprY[g_save.players[g_promoPlayer].rank], 64, 13);
        // Rank-tier pip row (g_gfxLogos column `col`, `count` copies), one line per rank.
#define PROMO_RANK_ROW(rankVal, col, count) \
    if (g_save.players[g_promoPlayer].rank == (rankVal)) \
        DrawRow((g_screenW >> 1) + 40, cy + 38, col, count);

        PROMO_RANK_ROW(RANK_ADMIRAL_1_1, 0, 1)
        PROMO_RANK_ROW(RANK_ADMIRAL_1_2, 0, 2)
        PROMO_RANK_ROW(RANK_ADMIRAL_1_3, 0, 3)
        PROMO_RANK_ROW(RANK_ADMIRAL_2_1, 1, 1)
        PROMO_RANK_ROW(RANK_ADMIRAL_2_2, 1, 2)
        PROMO_RANK_ROW(RANK_ADMIRAL_2_3, 1, 3)

        PROMO_RANK_ROW(RANK_ADMIRAL_3_1, 2, 1)
        PROMO_RANK_ROW(RANK_ADMIRAL_3_2, 2, 2)
        PROMO_RANK_ROW(RANK_ADMIRAL_3_3, 2, 3)

        PROMO_RANK_ROW(RANK_GRANDMASTER_1, 2, 1)
        PROMO_RANK_ROW(RANK_GRANDMASTER_2, 2, 2)
        PROMO_RANK_ROW(RANK_GRANDMASTER_3, 2, 3)
#undef PROMO_RANK_ROW
    }
    }
}

// Queues the promotion music (unless an external playlist is active) and the congratulations +
// rank voice-over sound sequence for the player named by g_promoPlayer, escalating to more
// elaborate sound stacks (voice, medal tier, planet name, "Warblade" sting) at higher ranks.
void PlayRankPromotionSounds()
{
    if (g_cfg.musicFormat == MUSIC_FMT_PLAYLIST && g_playlistCount != 0) {
    } else {
        UpdateMusicPos();
        g_songName = "promoted";
        g_musicMode = MUSIC_PROMOTED;
        StartMusic();
    }

    SoundQueueAdd(g_sfxCongratulations, 100, 1);

    // ---- plain ranks: one rank-specific stinger plus the generic "rank up" chime ----
#define PROMO_SIMPLE(rankVal, sfx)                           \
    if (g_save.players[g_promoPlayer].rank == (rankVal)) {   \
        SoundQueueAdd(sfx, 50, 1);                           \
        SoundQueueAdd(g_sfxRank, 50, 1);                     \
    }

    PROMO_SIMPLE(RANK_LIEUTENANT, g_sfxRankLieutenant)
    PROMO_SIMPLE(RANK_COMMANDER, g_sfxRankCommander)
    PROMO_SIMPLE(RANK_CAPTAIN, g_sfxRankCaptain)
    PROMO_SIMPLE(RANK_ADMIRAL, g_sfxRankAdmiral)

    // ---- admiral tiers: rank sfx + voice-over + medal + star/stars flourish ----
#define PROMO_ADMIRAL_TIER(rankVal, voice, medal, starSfx)      \
    if (g_save.players[g_promoPlayer].rank == (rankVal)) {      \
        SoundQueueAdd(g_sfxRankAdmiral, 100, 1);                \
        SoundQueueAdd(g_sfxRank, 50, 1);                        \
        SoundQueueAdd(voice, 5, 1);                             \
        SoundQueueAdd(medal, 5, 1);                             \
        SoundQueueAdd(starSfx, 0, 1);                           \
    }

    PROMO_ADMIRAL_TIER(RANK_ADMIRAL_1_1, g_sfxVoiceOne, g_sfxRankBronze, g_sfxStar)
    PROMO_ADMIRAL_TIER(RANK_ADMIRAL_1_2, g_sfxVoiceTwo, g_sfxRankBronze, g_sfxStars)
    PROMO_ADMIRAL_TIER(RANK_ADMIRAL_1_3, g_sfxVoiceThree, g_sfxRankBronze, g_sfxStars)
    PROMO_ADMIRAL_TIER(RANK_ADMIRAL_2_1, g_sfxVoiceOne, g_sfxRankSilver, g_sfxStar)
    PROMO_ADMIRAL_TIER(RANK_ADMIRAL_2_2, g_sfxVoiceTwo, g_sfxRankSilver, g_sfxStars)
    PROMO_ADMIRAL_TIER(RANK_ADMIRAL_2_3, g_sfxVoiceThree, g_sfxRankSilver, g_sfxStars)
    PROMO_ADMIRAL_TIER(RANK_ADMIRAL_3_1, g_sfxVoiceOne, g_sfxRankGold, g_sfxStar)
    PROMO_ADMIRAL_TIER(RANK_ADMIRAL_3_2, g_sfxVoiceTwo, g_sfxRankGold, g_sfxStars)
    PROMO_ADMIRAL_TIER(RANK_ADMIRAL_3_3, g_sfxVoiceThree, g_sfxRankGold, g_sfxStars)
#undef PROMO_ADMIRAL_TIER

    // ---- "Warblade" ranks: the sting plus a rank-specific stinger and the generic chime ----
#define PROMO_WARBLADE(rankVal, sfx, priority)                  \
    if (g_save.players[g_promoPlayer].rank == (rankVal)) {      \
        SoundQueueAdd(g_sfxWarblade, priority, 1);              \
        SoundQueueAdd(sfx, 50, 1);                              \
        SoundQueueAdd(g_sfxRank, 50, 1);                        \
    }

    PROMO_WARBLADE(RANK_KNIGHT, g_sfxRankKnight, 20)
    PROMO_WARBLADE(RANK_LORD, g_sfxRankLord, 20)
    PROMO_WARBLADE(RANK_OVERLORD, g_sfxRankOverlord, 20)
    PROMO_WARBLADE(RANK_GRANDMASTER, g_sfxRankGrandmaster, 20)
#undef PROMO_SIMPLE

    // ---- grandmaster tiers: like the admiral tiers, but with the Warblade sting up front ----
#define PROMO_GM_TIER(rankVal, voice, starSfx)                  \
    if (g_save.players[g_promoPlayer].rank == (rankVal)) {      \
        SoundQueueAdd(g_sfxWarblade, 20, 1);                    \
        SoundQueueAdd(g_sfxRankGrandmaster, 20, 1);             \
        SoundQueueAdd(g_sfxRank, 50, 1);                        \
        SoundQueueAdd(voice, 5, 1);                             \
        SoundQueueAdd(g_sfxRankGold, 5, 1);                     \
        SoundQueueAdd(starSfx, 0, 1);                           \
    }

    PROMO_GM_TIER(RANK_GRANDMASTER_1, g_sfxVoiceOne, g_sfxStar)
    PROMO_GM_TIER(RANK_GRANDMASTER_2, g_sfxVoiceTwo, g_sfxStars)
    PROMO_GM_TIER(RANK_GRANDMASTER_3, g_sfxVoiceThree, g_sfxStars)
#undef PROMO_GM_TIER

    PROMO_WARBLADE(RANK_CHAMPION, g_sfxRankChampion, 40)
    PROMO_WARBLADE(RANK_GOD, g_sfxRankGod, 40)
#undef PROMO_WARBLADE

    // ---- god-planet ranks: Warblade sting, the "god" rank sfx, the planet's name, the chime ----
#define PROMO_PLANET(rankVal, planetSfx)                        \
    if (g_save.players[g_promoPlayer].rank == (rankVal)) {      \
        SoundQueueAdd(g_sfxWarblade, 40, 1);                    \
        SoundQueueAdd(g_sfxRankGod, 300, 1);                    \
        SoundQueueAdd(planetSfx, 40, 1);                        \
        SoundQueueAdd(g_sfxRank, 50, 1);                        \
    }

    PROMO_PLANET(RANK_GOD_PLUTO, g_sfxPlanetPluto)
    PROMO_PLANET(RANK_GOD_NEPTUNE, g_sfxPlanetNeptune)
    PROMO_PLANET(RANK_GOD_URANUS, g_sfxPlanetUranus)
    PROMO_PLANET(RANK_GOD_SATURN, g_sfxPlanetSaturn)
    PROMO_PLANET(RANK_GOD_JUPITER, g_sfxPlanetJupiter)
    PROMO_PLANET(RANK_GOD_MARS, g_sfxPlanetMars)
    PROMO_PLANET(RANK_GOD_TELLUS, g_sfxPlanetTellus)
    PROMO_PLANET(RANK_GOD_VENUS, g_sfxPlanetVenus)
    PROMO_PLANET(RANK_GOD_MERCURY, g_sfxPlanetMercury)
#undef PROMO_PLANET

    // The final, "ultimate" rank: a one-off 5-call stack, not shaped like PROMO_PLANET
    // (extra g_sfxUltimateRank call, and different priorities throughout).
    if (g_save.players[g_promoPlayer].rank == RANK_GOD_SOL) {
        SoundQueueAdd(g_sfxUltimateRank, 150, 1);
        SoundQueueAdd(g_sfxWarblade, 100, 1);
        SoundQueueAdd(g_sfxRankGod, 100, 1);
        SoundQueueAdd(g_sfxPlanetSol, 40, 1);
        SoundQueueAdd(g_sfxRank, 50, 1);
    }
}
