// items.cpp: Bonus items: spawning, pickup effects (one huge switch), movement and drawing.
#include <stdio.h>
#include "globals.h"
#include "game.h"


// No-op hook, called after the player picks up a gem.
void AfterGemPickup()
{
}

// Drops a random weighted bonus pickup at (x, y), picked from g_itemTypePool via
// g_bonusWeight; level-gated types (pool index 12/13/14) get re-rolled more often as the
// player's level increases.
void SpawnBonus(float x, float y)
{
    int i;
    int r;
    int k;
    int total;
    int j;
    bool ok;

    ok = true;
    total = 0;
    for (j = 0; j < NUM_BONUS_WEIGHTS; j++)
        total += g_bonusWeight[j];
    for (i = 0; i < MAX_ITEMS; i++)
    {
        if (g_items[i].alive == 0)
        {
            g_items[i].active = 1;
            g_items[i].alive = 1;
            g_items[i].x = RandRange(0, 6) - 3 + x;
            g_items[i].y = y;
            g_items[i].vx = 0;
            g_items[i].vy = RandFloat(1.2f, 1.8f);
            g_items[i].frameDelay = RandFloat(3.0f, 7.0f);
            g_items[i].frameTimer = g_items[i].frameDelay;
            g_items[i].gfx = g_gfxBonus;
            g_items[i].hma = (int)g_hmaBonuses;
            g_items[i].hmaW = g_bonusItemField34;
            g_items[i].hmaH = g_bonusItemGfxH;

            do
            {
                ok = true;
                do
                {
                    r = RandRange(0, total - 1);
                    k = 0;
                    while (r > g_bonusWeight[k])
                    {
                        r -= g_bonusWeight[k];
                        k++;
                        if (k > NUM_BONUS_WEIGHTS - 1)
                        {
                            r = RandRange(0, total - 1);
                            k = 0;
                        }
                    }
                    if (k < 0)
                        k = 0;
                } while (g_bonusWeight[k] == 0);

                // reroll if the picked type is over its per-level rarity cap
                if (g_itemTypePool[k] == ITEM_WEAPON_SINGLE)
                {
                    if (RandRange(0, 50) < g_save.players[g_curPlayer].level)
                        ok = false;
                }
                if (g_itemTypePool[k] == ITEM_WEAPON_DOUBLE)
                {
                    if (RandRange(0, 150) < g_save.players[g_curPlayer].level)
                        ok = false;
                }
                if (g_itemTypePool[k] == ITEM_WEAPON_TRIPLE)
                {
                    if (RandRange(0, 300) < g_save.players[g_curPlayer].level)
                        ok = false;
                }
            } while (!ok);

            g_items[i].srcX = g_itemBonusSrcX[k];
            g_items[i].srcY = g_itemBonusSrcY[k];
            g_items[i].h = g_itemHeightTable[k];
            g_items[i].w = g_itemWidthTable[k];
            g_items[i].frameCount = g_itemFrameCountTable[k];
            g_items[i].frame = RandRange(0, 5) + 2.0f;
            g_items[i].type = g_itemTypePool[k];
            g_items[i].left = g_items[i].srcX;
            g_items[i].top = g_items[i].srcY;
            g_items[i].right = g_items[i].left + g_items[i].w;
            g_items[i].bottom = g_items[i].top + g_items[i].h;
            break;
        }
    }
}

// Drops a coin item at (x, y): picks one of the coin variants (falling speed/sprite/type
// from g_coin*Table[r]), occasionally a rarer one when `rare` is set.
void SpawnItem(float x, float y, unsigned char rare)
{
    int i;
    int r;
    int n;

    for (i = 0; i < MAX_ITEMS; i++)
    {
        if (g_items[i].alive == 0)
        {
            if (rare && RandRange(0, 5) < 1)
                n = 7;
            else
                n = 6;
            r = RandRange(0, n);
            g_items[i].active = 1;
            g_items[i].alive = 1;
            g_items[i].x = x + 22.0;
            g_items[i].y = y;
            g_items[i].vx = 0;
            g_items[i].vy = g_coinVySpeedTable[r];

            g_items[i].gfx = g_gfxCoins;
            g_items[i].hma = (int)g_hmaMarks;
            g_items[i].hmaW = g_coinGfxW;
            g_items[i].hmaH = g_coinGfxH;
            g_items[i].frameDelay = 6.0f;
            g_items[i].frameTimer = 6.0f;
            g_items[i].srcX = g_coinSrcX[r];
            g_items[i].srcY = g_coinSrcYTable[r];
            g_items[i].frame = 0;
            g_items[i].frameCount = 10;
            g_items[i].h = 20;
            g_items[i].w = 20;
            g_items[i].type = g_coinTypeTable[r];

            g_items[i].left = g_items[i].srcX;
            g_items[i].top = g_items[i].srcY;
            g_items[i].right = g_items[i].left + g_items[i].w;
            g_items[i].bottom = g_items[i].top + g_items[i].h;
            break;
        }
    }
}

// Drops a weapon-upgrade coin at (x, y): prefers a weapon mark the player doesn't have yet
// (E/X/T/R/A letter bits in `marks`), falling back to a random one once all are collected.
void SpawnWeaponItem(int x, int y)
{
    int i;
    int r;
    int nothave[6];
    int n;

    n = 0;
    if (((short)g_save.players[g_curPlayer].marks & MARK_1) == 0)
    {
        nothave[n] = 0;
        n++;
    }
    if (((short)g_save.players[g_curPlayer].marks & MARK_2) == 0)
    {
        nothave[n] = 1;
        n++;
    }
    if (((short)g_save.players[g_curPlayer].marks & MARK_3) == 0)
    {
        nothave[n] = 2;
        n++;
    }
    if (((short)g_save.players[g_curPlayer].marks & MARK_4) == 0)
    {
        nothave[n] = 3;
        n++;
    }

    if (((short)g_save.players[g_curPlayer].marks & MARK_5) == 0)
    {
        nothave[n] = 4;
        n++;
    }
    if (((short)g_save.players[g_curPlayer].marks & MARK_6) == 0)
    {
        nothave[n] = 5;
        n++;
    }
    if (n > 0)
        r = nothave[RandRange(0, n)];
    else
        r = RandRange(0, 6);
    for (i = 0; i < MAX_ITEMS; i++)
    {
        if (g_items[i].alive == 0)
        {
            g_items[i].active = 1;
            g_items[i].alive = 1;
            g_items[i].x = x + 22;
            g_items[i].y = y;
            g_items[i].vx = 0;
            g_items[i].vy = g_coinVySpeedTable[r];

            g_items[i].gfx = g_gfxCoins;
            g_items[i].hma = (int)g_hmaMarks;
            g_items[i].hmaW = g_coinGfxW;
            g_items[i].hmaH = g_coinGfxH;
            g_items[i].frameDelay = 6.0f;
            g_items[i].frameTimer = 6.0f;
            g_items[i].srcX = g_coinSrcX[r];
            g_items[i].srcY = g_coinSrcYTable[r];
            g_items[i].frame = RandRange(0, 9);
            g_items[i].frameCount = 10;
            g_items[i].h = 20;
            g_items[i].w = 20;
            g_items[i].type = g_coinTypeTable[r];

            g_items[i].left = g_items[i].srcX;
            g_items[i].top = g_items[i].srcY;
            g_items[i].right = g_items[i].left + g_items[i].w;
            g_items[i].bottom = g_items[i].top + g_items[i].h;
            break;
        }
    }
}

// Drops one of the 4 money-bag powerups (types 29-32, weighted 40/27/20/13%) at (x, y).
void SpawnPowerup(float x, float y)
{
    int i;
    int r;
    int nn;

    for (i = 0; i < MAX_ITEMS; i++)
    {
        if (g_items[i].alive == 0)
        {
            r = RandRange(1, 100);
            if (r <= MONEY_ROLL_SMALL_MAX)
                nn = ITEM_MONEY_SMALL;
            if (r > MONEY_ROLL_SMALL_MAX && r <= MONEY_ROLL_MEDIUM_MAX)
                nn = ITEM_MONEY_MEDIUM;
            if (r > MONEY_ROLL_MEDIUM_MAX && r <= MONEY_ROLL_LARGE_MAX)
                nn = ITEM_MONEY_LARGE;
            if (r > MONEY_ROLL_LARGE_MAX)
                nn = ITEM_MONEY_BLUE;

            g_items[i].active = 1;
            g_items[i].alive = 1;
            g_items[i].x = x;
            g_items[i].y = y;
            g_items[i].vx = 0;
            g_items[i].vy = 1.0f + RandFloat(0.0f, 1.0f);
            g_items[i].frameDelay = RandFloat(3.0f, 7.0f);
            g_items[i].frameTimer = g_items[i].frameDelay;

            g_items[i].gfx = g_gfxBonus;
            g_items[i].hma = (int)g_hmaBonuses;
            g_items[i].hmaW = g_bonusItemField34;
            g_items[i].hmaH = g_bonusItemGfxH;
            g_items[i].srcX = g_itemBonusSrcX[nn];
            g_items[i].srcY = g_itemBonusSrcY[nn];
            g_items[i].h = g_itemHeightTable[nn];
            g_items[i].w = g_itemWidthTable[nn];
            g_items[i].frameCount = g_itemFrameCountTable[nn];
            g_items[i].frame = RandRange(0, 10);
            g_items[i].type = nn;

            g_items[i].left = g_items[i].srcX;
            g_items[i].top = g_items[i].srcY;
            g_items[i].right = g_items[i].left + g_items[i].w;
            g_items[i].bottom = g_items[i].top + g_items[i].h;
            break;
        }
    }
}

// Bursts 30-45 money-bag items outward from (x, y) in an evenly spaced, slowly rotating
// fan (used on big enemy/boss kills). `preferBlueMoney` biases toward the blue-money type
// once unlocked; `bigBurst` (with `preferBlueMoney`) shrinks the count and marks the items
// to fall fast.
void SpawnPowerupBurst(int x, int y, unsigned char preferBlueMoney, unsigned char bigBurst)
{
    int count;
    int i;
    int rr;
    int type;
    int ang;
    int step;
    int n;
    int unused;
    float speed;

    count = RandRange(0, 15) + 30;
    if (bigBurst && preferBlueMoney)
        count = count * 0.6f;
    if (bigBurst && !preferBlueMoney)
        count = count * 0.85f;
    unused = 0;
    n = RandRange(0, 6) + 6;
    ang = RandRange(0, 360);
    step = 360 / n;

    for (i = 0; i < MAX_ITEMS; i++)
    {
        if (g_items[i].alive == 0)
        {
            if (preferBlueMoney && g_save.players[g_curPlayer].blueMoneyUnlocked != 0)
            {
                type = ITEM_MONEY_BLUE;
            }
            else
            {
                rr = RandRange(0, 100);
                type = ITEM_MONEY_SMALL;
                if (rr <= MONEY_ROLL_SMALL_MAX)
                    type = ITEM_MONEY_SMALL;
                if (rr > MONEY_ROLL_SMALL_MAX && rr <= MONEY_ROLL_MEDIUM_MAX)
                    type = ITEM_MONEY_MEDIUM;
                if (rr > MONEY_ROLL_MEDIUM_MAX && rr <= MONEY_ROLL_LARGE_MAX)
                    type = ITEM_MONEY_LARGE;
                if (rr > MONEY_ROLL_LARGE_MAX)
                    type = ITEM_MONEY_BLUE;
            }

            g_items[i].active = 1;
            g_items[i].alive = 1;
            g_items[i].x = x;
            g_items[i].y = y;
            speed = RandFloat(2.0f, 8.0f);
            g_items[i].vx = g_cosDeg[ang] * speed;
            g_items[i].vy = g_sinDeg[ang] * speed;
            g_items[i].fastFall = bigBurst & preferBlueMoney;
            g_items[i].timer = 10.0f;
            ang += step;
            if (ang >= 360)
                ang -= 360;
            if (--n < 0)
            {
                n = RandRange(0, 6) + 6;
                ang = RandRange(0, 360);
                step = 360 / n;
            }

            g_items[i].frameDelay = RandFloat(3.0f, 7.0f);
            g_items[i].frameTimer = g_items[i].frameDelay;
            g_items[i].gfx = g_gfxBonus;
            g_items[i].hma = (int)g_hmaBonuses;
            g_items[i].hmaW = g_bonusItemField34;
            g_items[i].hmaH = g_bonusItemGfxH;
            g_items[i].srcX = g_itemBonusSrcX[type];
            g_items[i].srcY = g_itemBonusSrcY[type];
            g_items[i].h = g_itemHeightTable[type];
            g_items[i].w = g_itemWidthTable[type];
            g_items[i].frameCount = g_itemFrameCountTable[type];
            g_items[i].frame = RandRange(0, 10);

            if (type == ITEM_MONEY_SMALL)
                g_items[i].type = ITEM_MONEY_SMALL_BURST;
            if (type == ITEM_MONEY_MEDIUM)
                g_items[i].type = ITEM_MONEY_MEDIUM_BURST;
            if (type == ITEM_MONEY_LARGE)
                g_items[i].type = ITEM_MONEY_LARGE_BURST;
            if (type == ITEM_MONEY_BLUE)
                g_items[i].type = ITEM_MONEY_BLUE_BURST;
            g_items[i].left = g_items[i].srcX;
            g_items[i].top = g_items[i].srcY;
            g_items[i].right = g_items[i].left + g_items[i].w;
            g_items[i].bottom = g_items[i].top + g_items[i].h;
            count--;
            if (count < 1)
                break;
        }
    }
}

// Drops a diamond/gem item at (x, y) with one of 4 sprite offsets, used for the gem-drop
// bonus and the gem-bomb pickup.
void SpawnGem(float x, float y)
{
    int i;
    int r;
    int gemXO[4];

    gemXO[0] = 0;
    gemXO[1] = 16;
    gemXO[2] = 32;
    gemXO[3] = 48;
    for (i = 0; i < MAX_ITEMS; i++)
    {
        if (g_items[i].alive == 0)
        {
            r = RandRange(0, 4);
            g_items[i].active = 1;
            g_items[i].alive = 1;
            g_items[i].x = x;
            g_items[i].y = y;
            g_items[i].vx = 0;
            g_items[i].vy = RandFloat(0.5f, 1.2f);
            g_items[i].frameDelay = RandFloat(3.0f, 7.0f);
            g_items[i].frameTimer = g_items[i].frameDelay;

            g_items[i].gfx = g_gfxDiamond;
            g_items[i].hma = (int)g_hmaDiamond;
            g_items[i].hmaW = g_diamondGfxW;
            g_items[i].hmaH = g_diamondGfxH;
            g_items[i].srcX = gemXO[r];
            g_items[i].srcY = 0;
            g_items[i].h = 13;
            g_items[i].w = 16;
            g_items[i].frameCount = 11;
            g_items[i].frame = RandRange(0, 11);
            g_items[i].type = ITEM_GEM;

            g_items[i].left = g_items[i].srcX;
            g_items[i].top = g_items[i].srcY;
            g_items[i].right = g_items[i].left + g_items[i].w;
            g_items[i].bottom = g_items[i].top + g_items[i].h;
            break;
        }
    }
}

#define PL g_save.players[g_curPlayer]

#define EN g_enemies[playerSlot][i]

#define SHIP g_shipDefs[PL.ship]

#define ADD_SCORE(bonus, pos) \
    g_scoreBefore = PL.score; \
    PL.score += DEOBFUSCATE_VALUE(bonus) * g_scoreMul[g_curPlayer]; \
    PL.score = ClampScore(PL.score); \
    if (PL.score == -1) { \
        sprintf(g_logBuf, "SCORE ERROR S:%d P:%d M:%d B:%d POS:%d\r\n", g_scoreBefore, g_curPlayer, \
                g_scoreMul[g_curPlayer], DEOBFUSCATE_VALUE(bonus), pos); \
        LogPrint(g_logBuf); \
        g_errPos = pos; \
        if (g_errPos != 0) { \
            g_errDiv = 0; \
            g_errDiv = g_errDiv / g_errDiv; \
        } \
    }

#define MSG_COLOR() \
    if (g_gameMode == MODE_DUAL) { \
        if (g_curPlayer == 0) \
            g_msgColor = 1; \
        else \
            g_msgColor = 4; \
    } else { \
        g_msgColor = 1; \
    }

#define COMPLETE(pos, popY, failSample) \
        if (PL.extraLetterE != 0 && PL.extraLetterX != 0 && PL.extraLetterT != 0 && PL.extraLetterR != 0 && \
            PL.extraLetterA != 0) { \
            PL.extraProgress = ' '; \
            PL.artxeProgress = ' '; \
            if (PL.lives < SHIP->minEnergy + SHIP->maxEnergy) { \
                PL.lives = PL.lives + SHIP->cost; \
                SoundPlay(g_sfxFanfare, -1, 0xff, 0.0f, 0xdf, g_sndFlags); \
                g_livesGainedCount = g_livesGainedCount + 10; \
            } else if (PL.armour < SHIP->baseArmour + SHIP->maxArmourBonus) { \
                PL.armour = PL.armour + SHIP->armourStep; \
                sprintf(g_alertMsg, "ARMOUR"); \
                MSG_COLOR(); \
                g_msgTimer = g_time + 1000; \
                SoundQueueAdd(g_sfxArmour, 50, 0); \
                g_armourAddedCount = g_armourAddedCount + 5; \
            } else { \
                ADD_SCORE(g_enemyScoreTable[OBF_1000000], pos); \
                AddScorePopup(g_screenW >> 1, popY, DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_1000000]), 1); \
            } \
            if (PL.lives > SHIP->minEnergy + SHIP->maxEnergy) \
                PL.lives = SHIP->minEnergy + SHIP->maxEnergy; \
            PL.extraLetterE = 0; \
            PL.extraLetterX = 0; \
            PL.extraLetterT = 0; \
            PL.extraLetterR = 0; \
            PL.extraLetterA = 0; \
            g_viewTransitionFlag = 2; \
            g_stateFn = SetViewHud; \
            EmptyViewChangeHook(); \
            g_drawBordersFn = DrawBorders; \
        } else { \
            SoundQueueAdd(failSample, 50, 0); \
        }

#define VOICE(field, id) \
            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) { \
                PL.field = 1; \
                MarkSecretFound(g_profileIndex, id); \
            }

#define WEAPON_HAVE(pos) \
            if (PL.bullets < 0x32) { \
                SoundQueueAdd(g_sfxExtraBullet, 50, 0); \
                PL.bullets++; \
                sprintf(g_alertMsg, "EXTRA BULLET"); \
                MSG_COLOR(); \
                g_msgTimer = g_time + 1000; \
                VOICE(secretFound07, 7); \
            } else { \
                ADD_SCORE(g_enemyScoreTable[OBF_25000_B], pos); \
                AddScorePopup((int)PL.x, (int)(PL.y - 30.0), DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_25000_B]), 0); \
                VOICE(secretFound28, 0x1c); \
            }

#define WEAPON_NEW(n, sample, str) \
            SoundQueueAdd(sample, 50, 0); \
            PL.weapon = n; \
            sprintf(g_alertMsg, str); \
            MSG_COLOR(); \
            g_msgTimer = g_time + 1000;

#define SAVE_WEAPONS() \
            PL.savedHyperspaceOutTimer = PL.hyperspaceOutTimer; \
            PL.savedHyperspaceMidTimer = PL.hyperspaceMidTimer; \
            PL.savedHyperspaceInTimer = PL.hyperspaceInTimer; \
            PL.savedHyperspaceInDuration = PL.hyperspaceInDuration; \
            PL.savedHyperspaceFade = PL.hyperspaceFade; \
            PL.savedScrollSpeedY = PL.scrollSpeedY; \
            PL.savedStarSpeed = PL.starSpeed; \
            PL.savedStarVelZ = PL.starVelZ; \
            PL.hyperspaceOutTimer = 0; \
            PL.hyperspaceMidTimer = 0; \
            PL.hyperspaceInTimer = 0; \
            PL.hyperspaceInDuration = 100.0f; \
            PL.hyperspaceFade = 0; \
            PL.scrollSpeedY = 0; \
            PL.starSpeed = 5.0f; \
            PL.starVelZ = 0;

#define RELEASE_HELD() \
            if (!PL.alienLock) { \
                if (PL.shieldL != 0) { \
                    DropAlienGfxAge((KGraphic *)g_enemies[g_curPlayer][PL.shieldLIdx].gfxA); \
                    PL.shieldL = 0; \
                    g_enemies[playerSlot][PL.shieldLIdx].active = 0; \
                    g_enemies[playerSlot][PL.shieldLIdx].settled = 0; \
                } \
                if (PL.shieldR != 0) { \
                    DropAlienGfxAge((KGraphic *)g_enemies[g_curPlayer][PL.shieldRIdx].gfxA); \
                    PL.shieldR = 0; \
                    g_enemies[playerSlot][PL.shieldRIdx].active = 0; \
                    g_enemies[playerSlot][PL.shieldRIdx].settled = 0; \
                } \
            }

#define PENALTY_HEAD() \
        showSuckerMsg = 1; \
        if (PL.bullets > 4) \
            PL.bullets--; \
        if (PL.weaponFloorAtOne == 0) { \
            if (PL.weapon > 0) \
                PL.weapon--; \
        } else { \
            if (PL.weapon > 1) \
                PL.weapon--; \
        } \
        PL.speed -= g_speedStep; \
        if (PL.speed < g_speedBase) \
            PL.speed = g_speedBase; \
        PL.buffDuration -= 5; \
        if (PL.buffDuration < g_speedMin) \
            PL.buffDuration = g_speedMin; \
        VOICE(secretFound02, 2);

#define CLAMP_UP(g, n) \
        g++; \
        if (g > n) \
            g = n;

#define TRIPLE_CHECK() \
        if (PL.blueMoneyActive != 0 && PL.gemCounterCollected != 0 && PL.msMultiplierActive != 0) { \
            showSuckerMsg = 0; \
            if (PL.weapon < 4) { \
                SoundQueueAdd(g_sfxSuperTripleShot, 50, 0); \
                PL.weapon = 4; \
                g_msgTimer = g_time + 2000; \
                sprintf(g_alertMsg, "SUPER TRIPLE SHOT"); \
            } \
            if (PL.bullets < 0x19) \
                PL.bullets = 0x19; \
            if (PL.speed < g_speedStep * g_speedMax + g_speedBase) \
                PL.speed = g_speedStep * g_speedMax + g_speedBase; \
            if (PL.buffDuration < 0x1e) \
                PL.buffDuration = 0x1e; \
            PL.blueMoneyActive = 0; \
            PL.gemCounterCollected = 0; \
            PL.msMultiplierActive = 0; \
            MSG_COLOR(); \
            VOICE(secretFound01, 1); \
        }

#define COUNTER(cnt, vf, vid, flag, str) \
        PL.cnt++; \
        if (PL.cnt >= 3) { \
            showSuckerMsg = 0; \
            PL.cnt = 0; \
            VOICE(vf, vid); \
            PL.flag = 1; \
            sprintf(g_alertMsg, str); \
            MSG_COLOR(); \
            g_msgTimer = g_time + 3000; \
        }

#define SUCKER(l) \
        if (showSuckerMsg) { \
            sprintf(g_alertMsg, "SUCKER"); \
            MSG_COLOR(); \
            g_msgTimer = g_time + 1000; \
            l = 0; \
            if (g_sfxSucker != 0) \
                l++; \
            if (g_sfxSucker2 != 0) \
                l++; \
            if (g_sfxSucker3 != 0) \
                l++; \
            l = RandRange(0, l); \
            if (l == 0) \
                SoundQueueAdd(g_sfxSucker, 50, 1); \
            if (l == 1) \
                SoundQueueAdd(g_sfxSucker2, 50, 1); \
            if (l == 2) \
                SoundQueueAdd(g_sfxSucker3, 50, 1); \
        }

#define ADD_SCORE2(sc, errsc, spos, pos) \
    g_scoreBefore = PL.score; \
    PL.score += sc * g_scoreMul[g_curPlayer]; \
    PL.score = ClampScore(PL.score); \
    if (PL.score == -1) { \
        sprintf(g_logBuf, "SCORE ERROR S:%d P:%d M:%d B:%d POS:%d\r\n", g_scoreBefore, g_curPlayer, \
                g_scoreMul[g_curPlayer], errsc, spos); \
        LogPrint(g_logBuf); \
        g_errPos = pos; \
        if (g_errPos != 0) { \
            g_errDiv = 0; \
            g_errDiv = g_errDiv / g_errDiv; \
        } \
    }

#define MONEY(bonus, errbonus, spos, pos, extra) \
        SoundQueueAdd(g_sfxMoney, 50, 0); \
        SoundPlay(g_sfxCoin, -1, 200, 0.0f, 0x7f, g_sndFlags); \
        PL.money += (int)DEOBFUSCATE_VALUE(bonus); \
        if (PL.money > PL.moneyMax) { \
            PL.money = PL.moneyMax; \
            ADD_SCORE2(DEOBFUSCATE_VALUE(bonus) * 10, DEOBFUSCATE_VALUE(errbonus) * 10, spos, pos); \
            AddScorePopup((int)PL.x, (int)(PL.y - 30.0), DEOBFUSCATE_VALUE(bonus) * 10, 0); \
            VOICE(secretFound28, 0x1c); \
        } else { \
            sprintf(g_alertMsg, "MONEY"); \
            MSG_COLOR(); \
            g_msgTimer = g_time + 1000; \
            extra \
            g_viewTransitionFlag = 2; \
            g_stateFn = SetViewHud; \
            EmptyViewChangeHook(); \
            g_drawBordersFn = DrawBorders; \
        } \
        if (PL.money > g_moneyMax) \
            g_moneyMax = PL.money;

#define GEMHDR(bit, pos) \
        PL.gemPickups++; \
        SoundPlay(g_sfxBell1, RandRange(22000, 32000), 0xff, g_pan, 0x7f, g_sndFlags); \
        SoundQueueAdd(g_sfxRankMarker, 50, 0); \
        PL.marks = (short)PL.marks | bit; \
        ADD_SCORE(g_enemyScoreTable[OBF_5000], pos); \
        AddScorePopup((int)PL.x + 5, (int)PL.y - 10, DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_5000]), 0); \
        AfterGemPickup();

#define GEMSEQ(want, next, want2, next2) \
        if (PL.gemSeqA == want) \
            PL.gemSeqA = next; \
        else \
            PL.gemSeqA = -1; \
        if (PL.gemSeqB == want2) \
            PL.gemSeqB = next2; \
        else \
            PL.gemSeqB = -1;

// Applies the effect of collecting a pickup item, one giant switch keyed by item `type`.
// Player-indexing note: `playerSlot` is g_curPlayer except in 2-player vs mode (g_gameMode MODE_DUAL)
// where both players share slot 0, so most cases go through the PL macro (g_save.players
// [g_curPlayer]) while enemy/shield lookups (EN, RELEASE_HELD) go through playerSlot.
void Pickup(int type)
{
    int i;
    int j;
    float spawnX;
    float spawnY;
    bool showSuckerMsg;
    char unusedBombFlag;
    int playerSlot;
    int totalBonusWeight;
    int k;
    int weightRoll;
    int bonusIndex;
    bool validPick;

    bool gemBombExplosionDone;
    int itemIdx;
    int bonusIconIndex;
    int multiplierRoll;
    int itemSlot;
    int blueMoneySuckerRoll;
    int gemCounterSuckerRoll;
    int multiplierSuckerRoll;
    bool moneyBombExplosionDone;

    showSuckerMsg = 1;
    unusedBombFlag = 0;
    playerSlot = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        playerSlot = 0;
    g_completePopupYOffset = 0;
    PL.pickupCount++;

    switch (type) {

    // ---- freeze, and the EXTRA / reversed-ARTXE letter pickups ----
    case ITEM_FREEZE:  // FREEZE: freezes enemies for 10s
        if (g_gameMode == MODE_DUAL) {
            g_save.players[0].freezeTimer = g_time + 10000;
            g_save.players[1].freezeTimer = g_time + 10000;
        } else {
            PL.freezeTimer = g_time + 10000;
        }
        SoundQueueAdd(g_sfxFreeze, 50, 0);
        break;

    case ITEM_LETTER_E:  // 'E' letter: feeds both the EXTRA and reversed ARTXE word progress
        if (PL.extraLetterE != 0) {
            ADD_SCORE(g_enemyScoreTable[OBF_100_B], 0xc);
        }
        PL.extraLetterE = 1;
        if (PL.artxeProgress == 'X') {
            if (PL.lives < SHIP->minEnergy + SHIP->maxEnergy ||
                PL.armour < SHIP->baseArmour + SHIP->maxArmourBonus) {
                PL.lives = SHIP->minEnergy + SHIP->maxEnergy;
                PL.armour = SHIP->baseArmour + SHIP->maxArmourBonus;
                if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                    PL.secretFound21 = 1;
                    MarkSecretFound(g_profileIndex, 0x15);
                }
                sprintf(g_alertMsg, "*** A R T X E ***");

            } else {
                if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                    PL.secretFound21 = 1;
                    MarkSecretFound(g_profileIndex, 0x15);
                }
                sprintf(g_alertMsg, "*** S U P E R   A R T X E ***");
                ADD_SCORE(g_enemyScoreTable[OBF_5000000], 0xd);
                AddScorePopup(g_screenW >> 1, g_screenH >> 1, DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_5000000]), 1);
                g_completePopupYOffset = 0x32;
            }
            MSG_COLOR();
            g_msgTimer = g_time + 3000;
        }
        PL.artxeProgress = 'E';
        PL.extraProgress = 'E';
        COMPLETE(0xe, (g_screenH >> 1) + g_completePopupYOffset, g_sfxVoiceLetterE);
        break;

    case ITEM_LETTER_X:  // 'X' letter
        if (PL.extraLetterX != 0) {
            ADD_SCORE(g_enemyScoreTable[OBF_100_B], 0xf);
        }
        PL.extraLetterX = 1;
        if (PL.artxeProgress == 'T')
            PL.artxeProgress = 'X';
        else
            PL.artxeProgress = ' ';
        if (PL.extraProgress == 'E')
            PL.extraProgress = 'X';
        else
            PL.extraProgress = ' ';
        COMPLETE(0x10, g_screenH >> 1, g_sfxVoiceLetterX);
        break;

    case ITEM_LETTER_T:  // 'T' letter
        if (PL.extraLetterT != 0) {
            ADD_SCORE(g_enemyScoreTable[OBF_100_B], 0x11);
        }
        PL.extraLetterT = 1;
        if (PL.artxeProgress == 'R')
            PL.artxeProgress = 'T';
        else
            PL.artxeProgress = ' ';
        if (PL.extraProgress == 'X')
            PL.extraProgress = 'T';
        else
            PL.extraProgress = ' ';
        COMPLETE(0x12, g_screenH >> 1, g_sfxVoiceLetterT);
        break;

    case ITEM_LETTER_R:  // 'R' letter
        if (PL.extraLetterR != 0) {
            ADD_SCORE(g_enemyScoreTable[OBF_100_B], 0x13);
        }
        PL.extraLetterR = 1;
        if (PL.artxeProgress == 'A')
            PL.artxeProgress = 'R';
        else
            PL.artxeProgress = ' ';
        if (PL.extraProgress == 'T')
            PL.extraProgress = 'R';
        else
            PL.extraProgress = ' ';
        COMPLETE(0x14, g_screenH >> 1, g_sfxVoiceLetterR);
        break;

    case ITEM_LETTER_A:  // 'A' letter: completes EXTRA (and, via COMPLETE, ARTXE) if the rest are collected
        if (PL.extraLetterA != 0) {
            ADD_SCORE(g_enemyScoreTable[OBF_100_B], 0x15);
        }
        PL.extraLetterA = 1;
        PL.artxeProgress = 'A';
        if (PL.extraProgress == 'R') {
            PL.extraProgress = ' ';
            if (PL.lives < SHIP->minEnergy + SHIP->maxEnergy ||
                PL.armour < SHIP->baseArmour + SHIP->maxArmourBonus) {
                PL.lives = SHIP->minEnergy + SHIP->maxEnergy;
                PL.armour = SHIP->baseArmour + SHIP->maxArmourBonus;
                if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                    PL.secretFound20 = 1;
                    MarkSecretFound(g_profileIndex, 0x14);
                }
                sprintf(g_alertMsg, "*** E X T R A ***");

            } else {
                ADD_SCORE(g_enemyScoreTable[OBF_5000000], 0x16);
                AddScorePopup(g_screenW >> 1, g_screenH >> 1, DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_5000000]), 1);
                if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                    PL.secretFound20 = 1;
                    MarkSecretFound(g_profileIndex, 0x14);
                }
                sprintf(g_alertMsg, "*** S U P E R   E X T R A ***");
                g_completePopupYOffset = 0x32;
            }
            MSG_COLOR();
            g_msgTimer = g_time + 3000;
        }
        PL.extraProgress = 'A';
        COMPLETE(0x17, (g_screenH >> 1) + g_completePopupYOffset, g_sfxVoiceLetterA);
        break;

    // ---- random bonus, bonus-stage entrances, score multipliers, and basic pickups ----
    case ITEM_RANDOM_BONUS:  // "random bonus": rolls a weighted item from g_itemTypePool and recurses into Pickup for it
        if (g_inRandom == 0) {
            g_inRandom = 1;
            totalBonusWeight = 0;
            validPick = true;
            for (k = 0; k < NUM_BONUS_WEIGHTS; k++)
                totalBonusWeight = totalBonusWeight + g_bonusWeight[k];

            // Reroll until we land on a nonzero-weight slot that also passes the per-item gates below.
            do {
                validPick = true;
                do {
                    weightRoll = RandRange(0, totalBonusWeight - 1);
                    bonusIndex = 0;
                    while (weightRoll > g_bonusWeight[bonusIndex]) {
                        weightRoll = weightRoll - g_bonusWeight[bonusIndex];
                        bonusIndex++;
                    }
                    if (bonusIndex > NUM_BONUS_WEIGHTS - 1)
                        bonusIndex = NUM_BONUS_WEIGHTS - 1;
                    if (bonusIndex < 0)
                        bonusIndex = 0;
                } while (g_bonusWeight[bonusIndex] == 0);

                // Some item types get an extra level-scaled rejection chance on top of their weight.
                if (g_itemTypePool[bonusIndex] == ITEM_WEAPON_SINGLE && RandRange(0, 0x32) < PL.level)
                    validPick = false;
                if (g_itemTypePool[bonusIndex] == ITEM_WEAPON_DOUBLE && RandRange(0, 0x96) < PL.level)
                    validPick = false;
                if (g_itemTypePool[bonusIndex] == ITEM_WEAPON_TRIPLE && RandRange(0, 0x12c) < PL.level)
                    validPick = false;
                if (g_itemTypePool[bonusIndex] == ITEM_RANDOM_BONUS)
                    validPick = false;
            } while (!validPick);
            Pickup(g_itemTypePool[bonusIndex]);
            g_inRandom = 0;
        }
        break;

    case ITEM_MEMORY_STATION:  // memory-station entrance: saves weapon/hyperspace state and switches to STATE_MEMORY_STATION
        if (g_playerUpdateFn == StateDemo) {
            ResetToTitle();
            break;
        }

        if (g_bonusResultsTime == 0) {
            SoundStopAll();
            if (PL.trackKillsFlag != 0)
                PL.bonusKilled = PL.totalEnemies;
            if (g_gameMode == MODE_DUAL)
                g_bonusStagePlayer = g_curPlayer;
            SAVE_WEAPONS();
            PL.tries = 0;
            RELEASE_HELD();
            PlayMemoryStationMusic();
            g_memoryIntroTimer = g_time + 3000;
            g_flashOverlayActive = 1;
            g_fadeStep = 0;
            g_fadeColorSet = 1;
            InitGrid();
            g_buttonsOn = 0;
            KInput::hidePointer();
            g_memCountdownStage = 0xb;
            g_state = STATE_MEMORY_STATION;
            SoundQueueAdd(g_sfxMemoryStation, 50, 0);
        }
        break;

    case ITEM_TIMES2:  // TIMES 2 score multiplier
        PL.scoreMult2Timer = PL.buffDuration * 1000 + g_time;
        PL.scoreMult5Timer = 0;
        g_scoreMul[g_curPlayer] = 2;
        g_viewTransitionFlag = 2;
        g_stateFn = SetViewHud;
        EmptyViewChangeHook();
        g_drawBordersFn = DrawBorders;
        SoundQueueAdd(g_sfxTimes2, 50, 0);
        break;

    case ITEM_TIMES5:  // TIMES 5 score multiplier
        PL.scoreMult5Timer = PL.buffDuration * 1000 + g_time;
        PL.scoreMult2Timer = 0;
        g_scoreMul[g_curPlayer] = 5;
        g_viewTransitionFlag = 2;
        g_stateFn = SetViewHud;
        EmptyViewChangeHook();
        g_drawBordersFn = DrawBorders;
        SoundQueueAdd(g_sfxTimes5, 50, 0);
        break;

    case ITEM_EXTRA_BULLET:  // EXTRA BULLET
        SoundQueueAdd(g_sfxExtraBullet, 50, 0);
        if (PL.bullets < 0x32) {
            PL.bullets++;
        } else {
            ADD_SCORE(g_enemyScoreTable[OBF_25000_B], 0x18);
            AddScorePopup((int)PL.x, (int)(PL.y - 30.0), DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_25000_B]), 0);
            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                PL.secretFound28 = 1;
                MarkSecretFound(g_profileIndex, 0x1c);
            }
        }
        sprintf(g_alertMsg, "EXTRA BULLET");
        MSG_COLOR();
        g_msgTimer = g_time + 1000;
        break;

    case ITEM_EXTRA_SPEED:  // EXTRA SPEED
        SoundQueueAdd(g_sfxExtraSpeed, 50, 0);
        PL.speed += g_speedStep;
        if (PL.speed > g_speedStep * g_maxSpeedMul + g_speedBase) {
            PL.speed = g_speedStep * g_maxSpeedMul + g_speedBase;
            ADD_SCORE(g_enemyScoreTable[OBF_25000_B], 0x19);
            AddScorePopup((int)PL.x, (int)(PL.y - 30.0), DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_25000_B]), 0);
            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                PL.secretFound28 = 1;
                MarkSecretFound(g_profileIndex, 0x1c);
            }
        }
        sprintf(g_alertMsg, "EXTRA SPEED");
        MSG_COLOR();
        g_msgTimer = g_time + 1000;
        break;

    case ITEM_SHIELD:  // SHIELD
        SoundQueueAdd(g_sfxShield, 50, 0);
        PL.shieldTimer = PL.buffDuration * 1000 + g_time;
        sprintf(g_alertMsg, "SHIELD");
        MSG_COLOR();
        g_msgTimer = g_time + 1000;
        g_chanShieldHum = SoundPlayChannel(g_chanShieldHum, g_sfxShieldHum, -1, 0xb4, 0.0f, 0xdf);
        break;

    case ITEM_WEAPON_SINGLE:  // SINGLE SHOT weapon
        if (PL.weapon == 0) {
            WEAPON_HAVE(0x1a);
        } else {
            if (PL.weapon >= 3 && PL.weapon <= 6)
                SoundPlay2(g_sfxHahaha, -1, 0xb4, 0.0f, 0xff, g_sndFlags);
            if (PL.weapon >= 7) {
                SoundPlay2(g_sfxHahaha, -1, 0xc8, 0.0f, 0xff, g_sndFlags);
                SoundPlay2(g_sfxHahaha, -1, 0xff, 0.0f, 0xff, g_sndFlags);
            }
            WEAPON_NEW(0, g_sfxSingleShotVoice, "SINGLE SHOT");
        }
        break;

    case ITEM_WEAPON_DOUBLE:  // DOUBLE SHOT weapon
        if (PL.weapon == 1) {
            WEAPON_HAVE(0x1b);
        } else {
            WEAPON_NEW(1, g_sfxDoubleShot, "DOUBLE SHOT");
        }
        break;

    case ITEM_WEAPON_TRIPLE:  // TRIPLE SHOT weapon
        if (PL.weapon == 2) {
            WEAPON_HAVE(0x1c);
        } else {
            WEAPON_NEW(2, g_sfxTripleShot, "TRIPLE SHOT");
        }
        break;

    case ITEM_WARP:  // WARP: forces the current level to finish (skip)
        if (g_gameMode == MODE_DUAL) {
            g_save.players[0].shieldHitFlashSpeed = g_save.players[0].shieldHitFlashSpeed + 0.5;
            if (g_save.players[0].shieldHitFlashSpeed > 8.0)
                g_save.players[0].shieldHitFlashSpeed = 8.0f;
            g_save.players[0].enemyHpBonusRoll = g_save.players[0].enemyHpBonusRoll + 2;
            if (g_save.players[0].enemyHpBonusRoll > 0x4b)
                g_save.players[0].enemyHpBonusRoll = 0x4b;
            g_save.players[0].levelFinished = 1;
            g_save.players[0].levelWarpPending = 1;

            g_save.players[1].shieldHitFlashSpeed = g_save.players[1].shieldHitFlashSpeed + 0.5;
            if (g_save.players[1].shieldHitFlashSpeed > 8.0)
                g_save.players[1].shieldHitFlashSpeed = 8.0f;
            g_save.players[1].enemyHpBonusRoll = g_save.players[1].enemyHpBonusRoll + 2;
            if (g_save.players[1].enemyHpBonusRoll > 0x4b)
                g_save.players[1].enemyHpBonusRoll = 0x4b;
            g_save.players[1].levelFinished = 1;
            g_save.players[1].levelWarpPending = 1;
        } else {
            PL.shieldHitFlashSpeed += 0.5;
            if (PL.shieldHitFlashSpeed > 8.0)
                PL.shieldHitFlashSpeed = 8.0f;
            PL.enemyHpBonusRoll = PL.enemyHpBonusRoll + 2;
            if (PL.enemyHpBonusRoll > 0x4b)
                PL.enemyHpBonusRoll = 0x4b;
            PL.levelFinished = 1;
            PL.levelWarpPending = 1;
        }
        sprintf(g_alertMsg, "WARP");
        MSG_COLOR();
        g_msgTimer = g_time + 1000;
        break;

    case ITEM_SCOOP:  // SCOOP: arms the money/item scoop and its MAX_SCOOP spawn slots
        sprintf(g_alertMsg, "SCOOP");
        MSG_COLOR();
        SoundQueueAdd(g_sfxScoop, 50, 0);
        g_msgTimer = g_time + 1000;
        PL.scoopTimer = PL.buffDuration * 1000 + g_time;
        for (i = 0; i < MAX_SCOOP; i++) {
            g_scoop[i].pos = 0;
            g_scoop[i].spawnDelay = i + 5;
            g_scoop[i].timer = 2;
        }
        g_scoopRange = 0;
        g_chanScopeHum = SoundPlayChannel(g_chanScopeHum, g_sfxScopeHum, -1, 0x96, 0.0f, 0xdf);
        break;

    case ITEM_WEAPON_QUAD:  // QUAD SHOT weapon
        if (PL.weapon == 3) {
            WEAPON_HAVE(0x1d);
        } else {
            WEAPON_NEW(3, g_sfxQuadShot, "QUAD SHOT");
        }
        break;

    case ITEM_AUTOFIRE:  // AUTO FIRE
        SoundQueueAdd(g_sfxAutofire, 50, 0);
        if (PL.superAuto != 0 || PL.autofire != 0) {
            ADD_SCORE(g_enemyScoreTable[OBF_25000_B], 0x1e);
            AddScorePopup((int)PL.x, (int)(PL.y - 30.0), DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_25000_B]), 0);
            VOICE(secretFound28, 0x1c);
        }
        if (PL.superAuto == 0) {
            PL.autofireTimer = g_time;
            if (PL.autofire == 0) {
                PL.autofire = 1;
                sprintf(g_alertMsg, "AUTO FIRE");
                MSG_COLOR();
                g_msgTimer = g_time + 1000;
            }
        }
        break;

    // ---- bombs ----
    case ITEM_GEM_BOMB:  // GEM BOMB: kills all on-screen enemies (except a few immune types) and drops gems/items
        PL.bombPickups++;
        SoundPlay(g_sfxGemBomb, -1, 0xff, 0.0f, 0xdf, g_sndFlags);
        unusedBombFlag = 0;
        for (j = 0; j < 2; j++)
            AddParticle(g_gfxFlareBomb, (int)PL.x + 16, (int)PL.y + 16, 16.0f, RandFloat(0.0f, 40.0f) + 50.0,
                RandFloat(0.0f, 359.0f), 0.0f, 0, 0xff, RandRange(0, 100) + 32, 0, 600, 30.0f, 0.0f, -1, 0.0f,
                0, 0, 0, 1);
        SoundQueueAdd(g_sfxBomb, 50, 0);
        sprintf(g_alertMsg, "GEM BOMB");
        MSG_COLOR();
        g_msgTimer = g_time + 1000;

        // Kill every enemy on screen that isn't immune, dropping gems/items appropriate to its type.
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (EN.active == 1 && EN.type != ENEMY_CAPTURED && EN.type != ENEMY_MOTHERSHIP &&
                EN.type != ENEMY_DEBRIS && EN.type != ENEMY_GUARD &&
                EN.type != ENEMY_MONEY_SHIP && EN.attackStaggerTimer < 1.0) {
                EN.hp = 0;
                g_flashOverlayActive = 1;
                g_fadeStep = 0;
                g_fadeColorSet = 0;
                EN.active = 0;
                EN.forcedDir = -1;
                CreditKill(g_curPlayer);
                if (PL.trackKillsFlag != 0)
                    PL.bonusKilled++;
                if (EN.type == ENEMY_HOVER) {
                    spawnX = EN.x + EN.offsetX;
                    spawnY = EN.y + EN.offsetY;
                } else {
                    spawnX = EN.x;
                    spawnY = EN.y;
                }
                gemBombExplosionDone = false;

                if (EN.type == ENEMY_WRAPPER) {
                    gemBombExplosionDone = true;
                    SpawnBigExplosion(spawnX, spawnY, 0x40, 0x40, 0, 6, 0, 0xff, 0);
                    AddParticle(g_gfxFlare5, (int)spawnX + 32, (int)spawnY + 32, 16.0f,
                        RandFloat(0.0f, 5.0f) + 30.0, RandFloat(0.0f, 359.0f), 0.0f, 0, 0, RandRange(0, 0xff),
                        0xff, 600, 13.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);
                    SpawnItem(spawnX, spawnY, PL.color);
                    SpawnSlots(spawnX + 32.0, spawnY + 32.0, 0x1e, 0xff, 0, 0xff);
                }

                if (EN.type == ENEMY_MONEY_SUCKER) {
                    gemBombExplosionDone = true;
                    SpawnBigExplosion(spawnX, spawnY, 0x40, 0x40, 2, 6, 0, 0xc8, 0xff);
                    AddParticle(g_gfxFlare5, (int)spawnX + 32, (int)spawnY + 32, 16.0f,
                        RandFloat(0.0f, 5.0f) + 30.0, RandFloat(0.0f, 359.0f), 0.0f, 0, 0, RandRange(0, 0xff),
                        0xff, 600, 13.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);
                    SpawnSlots(spawnX + 32.0, spawnY + 32.0, 0x96, 0, 0xff, 0);
                    SpawnItems();
                    SpawnGem(spawnX + 9.0 + RandFloat(-10.0f, 10.0f), spawnY + 2.0 + RandFloat(-3.0f, 3.0f));
                    SpawnGem(spawnX + 9.0 + RandFloat(-10.0f, 10.0f), spawnY + 2.0 + RandFloat(-3.0f, 3.0f));
                    SpawnGem(spawnX + 9.0 + RandFloat(-10.0f, 10.0f), spawnY + 2.0 + RandFloat(-3.0f, 3.0f));
                    SpawnGem(spawnX + 9.0 + RandFloat(-10.0f, 10.0f), spawnY + 2.0 + RandFloat(-3.0f, 3.0f));
                    SpawnGem(spawnX + 9.0 + RandFloat(-10.0f, 10.0f), spawnY + 2.0 + RandFloat(-3.0f, 3.0f));
                }

                if (!gemBombExplosionDone) {
                    SpawnExplosion(spawnX, spawnY, 0x20, 0x20, 0x96, 0, 6, 0, 0xff, 0, 0, 0xff, 0);
                    AddParticle(g_gfxFlare5, (int)spawnX + 16, (int)spawnY + 16, 16.0f,
                        RandFloat(0.0f, 5.0f) + 20.0, RandFloat(0.0f, 359.0f), 0.0f, 0, 0, RandRange(0, 0xff),
                        0xff, 600, 10.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);
                    SpawnGem(spawnX + 9.0, spawnY + 2.0);
                    SpawnSlots(spawnX + 16.0, spawnY + 16.0, 0x32, 0xff, 0, 0xff);
                }
            }
        }
        if (PL.trackKillsFlag != 0)
            PL.bonusKilled = PL.totalEnemies;
        break;

    case ITEM_METEOR_STORM:  // meteor-storm entrance: saves state and switches to STATE_BONUS_RACE
        if (g_bonusResultsTime == 0) {
            SoundStopAll();
            PL.mirrorTime = 0;
            if (PL.trackKillsFlag != 0)
                PL.bonusKilled = PL.totalEnemies;
            if (g_gameMode == MODE_DUAL)
                g_vsTurnPlayer = g_curPlayer;
            SAVE_WEAPONS();
            PL.bonusRoundEnded = 1;
            RELEASE_HELD();
            for (itemIdx = 0; itemIdx < MAX_ITEMS; itemIdx++)
                g_items[itemIdx].alive = 0;
            PickBgTint();
            g_bonusSpawnTarget = 4.0f;
            InitMeteorStormLevel();

            // Chance-unlocked score multiplier icon: spawn one falling bonus item for it.
            if (PL.multiplierUnlocked != 0) {
                bonusIconIndex = 0;
                multiplierRoll = RandRange(0, 100);
                if (multiplierRoll < 0x32)
                    bonusIconIndex = 7;
                else
                    bonusIconIndex = 8;

                for (itemSlot = 0; itemSlot < MAX_ITEMS; itemSlot++) {
                    if (g_items[itemSlot].alive == 0) {
                        g_items[itemSlot].active = 1;
                        g_items[itemSlot].alive = 1;
                        g_items[itemSlot].x = (float)RandRange(0x40, g_screenW - 0x50);
                        g_items[itemSlot].y = -300.0f;
                        g_items[itemSlot].vx = 0;
                        g_items[itemSlot].vy = RandFloat(1.0f, 2.0f);
                        g_items[itemSlot].frameDelay = RandFloat(3.0f, 7.0f);
                        g_items[itemSlot].frameTimer = g_items[itemSlot].frameDelay;

                        g_items[itemSlot].gfx = g_gfxBonus;
                        g_items[itemSlot].hma = (int)g_hmaBonuses;
                        g_items[itemSlot].hmaW = g_bonusItemField34;
                        g_items[itemSlot].hmaH = g_bonusItemGfxH;
                        g_items[itemSlot].srcX = g_itemBonusSrcX[bonusIconIndex];
                        g_items[itemSlot].srcY = g_itemBonusSrcY[bonusIconIndex];
                        g_items[itemSlot].h = g_itemHeightTable[bonusIconIndex];
                        g_items[itemSlot].w = g_itemWidthTable[bonusIconIndex];
                        g_items[itemSlot].frameCount = g_itemFrameCountTable[bonusIconIndex];
                        g_items[itemSlot].frame = RandRange(0, 5) + 2.0f;
                        g_items[itemSlot].type = g_itemTypePool[bonusIconIndex];

                        g_items[itemSlot].left = g_items[itemSlot].srcX;
                        g_items[itemSlot].top = g_items[itemSlot].srcY;
                        g_items[itemSlot].right = g_items[itemSlot].left + g_items[itemSlot].w;
                        g_items[itemSlot].bottom = g_items[itemSlot].top + g_items[itemSlot].h;
                        break;
                    }
                }
            }

            PlayMeteorStormMusic();
            g_resultsScreenEndTime = g_time + 3000;
            PL.bonusRoundScore = 0;
            g_flashOverlayActive = 1;
            g_fadeStep = 0;
            g_fadeColorSet = 1;
            g_transitionLock = 0;
            g_state = STATE_BONUS_RACE;
            SoundQueueAdd(g_sfxMeteorStorm, 50, 0);
        }
        break;

    case ITEM_ARMOUR:  // ARMOUR
        if (PL.armour < SHIP->baseArmour + SHIP->maxArmourBonus) {
            PL.armour = PL.armour + SHIP->armourStep;
            sprintf(g_alertMsg, "ARMOUR");
            MSG_COLOR();
            g_msgTimer = g_time + 1000;
            SoundQueueAdd(g_sfxArmour, 50, 0);
            g_armourAddedCount = g_armourAddedCount + 5;
        } else {
            ADD_SCORE(g_enemyScoreTable[OBF_25000_B], 0x1f);
            AddScorePopup((int)PL.x, (int)(PL.y - 30.0), DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_25000_B]), 0);
            VOICE(secretFound28, 0x1c);
        }
        if (PL.armour > SHIP->baseArmour + SHIP->maxArmourBonus)
            PL.armour = SHIP->baseArmour + SHIP->maxArmourBonus;
        break;

    case ITEM_SUCKER_BLUE_MONEY:  // "SUCKER" trap pickup: penalizes like a hit, but counts toward the blue-money secret
        PENALTY_HEAD();
        PL.blueMoneyActive = 1;
        CLAMP_UP(g_bonusWeight[27], 0xf);
        CLAMP_UP(g_bonusWeight[33], 0xf);
        CLAMP_UP(g_bonusWeight[26], 0x23);
        TRIPLE_CHECK();
        COUNTER(blueMoneyPicks, secretFound14, 0xe, blueMoneyUnlocked, "** BLUE MONEY ACTIVATED **");
        SUCKER(blueMoneySuckerRoll);
        break;

    case ITEM_SUCKER_GEMS:  // "SUCKER" trap pickup, counts toward the gem-counter secret
        PENALTY_HEAD();
        CLAMP_UP(g_bonusWeight[27], 0xf);
        CLAMP_UP(g_bonusWeight[33], 0xf);
        CLAMP_UP(g_bonusWeight[26], 0x23);
        CLAMP_UP(g_bonusWeight[19], 0x2d);
        PL.gemCounterCollected = 1;
        TRIPLE_CHECK();
        COUNTER(gemCounterPicks, secretFound13, 0xd, gemCounterUnlocked, "** GEM COUNTER ACTIVATED **");
        SUCKER(gemCounterSuckerRoll);
        break;

    case ITEM_SUCKER_MULTIPLIER:  // "SUCKER" trap pickup, counts toward the memory-station multiplier secret
        PENALTY_HEAD();
        CLAMP_UP(g_bonusWeight[27], 0xf);
        CLAMP_UP(g_bonusWeight[33], 0xf);
        CLAMP_UP(g_bonusWeight[26], 0x23);
        PL.msMultiplierActive = 1;
        TRIPLE_CHECK();
        COUNTER(multiplierPicks, secretFound15, 0xf, multiplierUnlocked, "** MULTIPLIER IN M.S. ENABLED **");
        SUCKER(multiplierSuckerRoll);
        break;

    case ITEM_MIRROR:  // MIRROR MODE: reverses player controls
        SoundQueueAdd(g_sfxMirror, 50, 0);
        PL.mirrorTime = PL.buffDuration * 1000 + g_time;
        PL.mirrorX = PL.x;
        sprintf(g_alertMsg, "MIRROR MODE");
        MSG_COLOR();
        g_msgTimer = g_time + 1000;
        break;

    case ITEM_MONEY_BOMB:  // MONEY BOMB: kills all on-screen enemies and drops money powerups instead of gems
        PL.bombPickups++;
        SoundPlay(g_sfxMoneyBomb, -1, 0xff, 0.0f, 0xdf, g_sndFlags);
        unusedBombFlag = 0;
        for (j = 0; j < 2; j++)
            AddParticle(g_gfxFlareBomb, (int)PL.x + 16, (int)PL.y + 16, 16.0f, RandFloat(0.0f, 40.0f) + 60.0,
                RandFloat(0.0f, 359.0f), 0.0f, 0, 0, RandRange(0, 100) + 32, 0xff, 600, 30.0f, 0.0f, 0x1e,
                0.0f, 0, 0, 0, 1);
        SoundQueueAdd(g_sfxBomb, 50, 0);
        sprintf(g_alertMsg, "MONEY BOMB");
        MSG_COLOR();
        g_msgTimer = g_time + 1000;

        // Kill every enemy on screen, dropping money powerups (instead of gems) per its type.
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (EN.active == 1 && EN.type != ENEMY_CAPTURED && EN.type != ENEMY_MOTHERSHIP &&
                EN.type != ENEMY_DEBRIS && EN.type != ENEMY_GUARD &&
                EN.type != ENEMY_MONEY_SHIP && EN.attackStaggerTimer < 1.0) {
                EN.hp = 0;
                g_flashOverlayActive = 1;
                g_fadeStep = 0;
                g_fadeColorSet = 0;
                if (EN.type == ENEMY_HOVER) {
                    spawnX = EN.x + EN.offsetX;
                    spawnY = EN.y + EN.offsetY;
                } else {
                    spawnX = EN.x;
                    spawnY = EN.y;
                }
                EN.forcedDir = -1;
                EN.active = 0;
                CreditKill(g_curPlayer);
                if (PL.trackKillsFlag != 0)
                    PL.bonusKilled++;
                moneyBombExplosionDone = false;

                if (EN.type == ENEMY_WRAPPER) {
                    moneyBombExplosionDone = true;
                    SpawnBigExplosion(spawnX, spawnY, 0x40, 0x40, 2, 6, 0, 0xc8, 0xff);
                    AddParticle(g_gfxFlare5, (int)spawnX + 32, (int)spawnY + 32, 16.0f,
                        RandFloat(0.0f, 5.0f) + 30.0, RandFloat(0.0f, 359.0f), 0.0f, 0, 0, RandRange(0, 0xff),
                        0xff, 600, 13.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);
                    SpawnItem(spawnX + 24.0, spawnY + 24.0, g_save.players[playerSlot].color);
                    SpawnSlots(spawnX + 32.0, spawnY + 32.0, 0x96, 0, 0xff, 0);
                }

                if (EN.type == ENEMY_MONEY_SUCKER) {
                    moneyBombExplosionDone = true;
                    SpawnBigExplosion(spawnX, spawnY, 0x40, 0x40, 2, 6, 0, 0xc8, 0xff);
                    AddParticle(g_gfxFlare5, (int)spawnX + 32, (int)spawnY + 32, 16.0f,
                        RandFloat(0.0f, 5.0f) + 30.0, RandFloat(0.0f, 359.0f), 0.0f, 0, 0, RandRange(0, 0xff),
                        0xff, 600, 13.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);
                    SpawnSlots(spawnX + 32.0, spawnY + 32.0, 0x96, 0, 0xff, 0);
                    SpawnItems();
                }

                if (!moneyBombExplosionDone) {
                    SpawnExplosion(spawnX, spawnY, 0x20, 0x20, 0x96, 2, 6, 0, 0xc8, 0xff, 0, 0xc8, 0xff);
                    AddParticle(g_gfxFlare5, (int)spawnX + 16, (int)spawnY + 16, 16.0f,
                        RandFloat(0.0f, 5.0f) + 20.0, RandFloat(0.0f, 359.0f), 0.0f, 0, 0, RandRange(0, 0xff),
                        0xff, 600, 10.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);
                    SpawnPowerup(spawnX + 4.0, spawnY + 3.0);
                    SpawnSlots(spawnX + 16.0, spawnY + 16.0, 0x32, 0, 0xff, 0);
                }
            }
        }
        if (PL.trackKillsFlag != 0)
            PL.bonusKilled = PL.totalEnemies;
        break;

    // ---- lives, time, money ----
    case ITEM_EXTRA_LIFE:  // EXTRA LIFE (falls back to armour, then score, once maxed)
        if (PL.lives < SHIP->minEnergy + SHIP->maxEnergy) {
            PL.lives = PL.lives + SHIP->cost;
            SoundQueueAdd(g_sfxExtraLife, 50, 0);
            SoundPlay(g_sfxFanfare1, -1, 0xff, 0.0f, 0xdf, g_sndFlags);
            g_livesGainedCount = g_livesGainedCount + 10;
        } else if (PL.armour < SHIP->baseArmour + SHIP->maxArmourBonus) {
            PL.armour = PL.armour + SHIP->armourStep;
            sprintf(g_alertMsg, "ARMOUR");
            MSG_COLOR();
            g_msgTimer = g_time + 1000;
            SoundQueueAdd(g_sfxArmour, 50, 0);
            g_armourAddedCount = g_armourAddedCount + 5;
        } else {
            ADD_SCORE(g_enemyScoreTable[OBF_1000000], 0x20);
            AddScorePopup(g_screenW >> 1, g_screenH >> 1, DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_1000000]), 1);
        }
        if (PL.lives > SHIP->minEnergy + SHIP->maxEnergy)
            PL.lives = SHIP->minEnergy + SHIP->maxEnergy;
        sprintf(g_alertMsg, "EXTRA LIFE");
        MSG_COLOR();
        g_msgTimer = g_time + 1000;
        break;

    case ITEM_EXTRA_TIME:  // EXTRA TIME
        SoundQueueAdd(g_sfxExtraTime, 50, 0);
        if (PL.buffDuration < g_timeMax) {
            PL.buffDuration = PL.buffDuration + 5;
        } else {
            PL.buffDuration = g_timeMax;
            ADD_SCORE(g_enemyScoreTable[OBF_25000_B], 0x21);
            AddScorePopup((int)PL.x, (int)(PL.y - 30.0), DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_25000_B]), 0);
            VOICE(secretFound28, 0x1c);
        }
        sprintf(g_alertMsg, "EXTRA TIME");
        MSG_COLOR();
        g_msgTimer = g_time + 1000;
        break;

    case ITEM_MONEY_SMALL:  // small money pickup
        MONEY(g_enemyScoreTable[OBF_10], g_enemyScoreTable[OBF_0_D], 0x22, 0x22, ;);
        break;

    case ITEM_MONEY_MEDIUM:  // medium money pickup
        MONEY(g_enemyScoreTable[OBF_50], g_enemyScoreTable[OBF_50], 0x23, 0x23, ;);
        break;

    case ITEM_MONEY_LARGE:  // large money pickup
        MONEY(g_enemyScoreTable[OBF_100_B], g_enemyScoreTable[OBF_100_B], 0x24, 0x24, g_moneyBlinkTimer = 0;);
        break;

    case ITEM_MONEY_BLUE:  // blue/rare money pickup
        MONEY(g_enemyScoreTable[OBF_200], g_enemyScoreTable[OBF_200], 0x24, 0x88, g_moneyBlinkTimer = 0;);
        break;

    case ITEM_MONEY_DOUBLER:  // MONEY DOUBLER (malfunctions with a different message above a money threshold)
        if (PL.money < DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_450000])) {
            SoundPlay(g_sfxChaching, -1, 200, 0.0f, 0x7f, g_sndFlags);
            PL.money = PL.money * 2;
            if (PL.money > PL.moneyMax) {
                PL.money = PL.moneyMax;
                ADD_SCORE2(PL.moneyMax * 2, PL.moneyMax, 0x25, 0x25);
                AddScorePopup((int)PL.x, (int)(PL.y - 30.0), (__int64)PL.moneyMax * 2, 0);
                VOICE(secretFound28, 0x1c);
            } else {
                if (PL.money == 0) {
                    if (PL.buffDuration < g_timeMax)
                        PL.buffDuration = PL.buffDuration + 0x1e;
                    if (PL.buffDuration > g_timeMax)
                        PL.buffDuration = g_timeMax;
                    VOICE(secretFound16, 0x10);
                }

                sprintf(g_alertMsg, "MONEY DOUBLER");
                MSG_COLOR();
                g_msgTimer = g_time + 2000;
                g_moneyBlinkTimer = 0;
                g_viewTransitionFlag = 2;
                g_stateFn = SetViewHud;
                EmptyViewChangeHook();
                g_drawBordersFn = DrawBorders;
            }
            if (PL.money > g_moneyMax)
                g_moneyMax = PL.money;

        } else {
            SoundPlay(g_sfxChaching, RandRange(15000, 18000), 0xff, 0.0f, 0x7f, g_sndFlags);
            sprintf(g_alertMsg, "MONEY DOUBLER MALFUNCTION");
            g_msgColor = 0;
            g_msgTimer = g_time + 1000;
        }
        break;

    // ---- drunk mode, gem-sequence marks, bad gem, bullet speed ----
    case ITEM_DRUNK:  // DRUNK MODE: wobbly controls
        SoundQueueAdd(g_sfxDrunk, 10, 0);
        PL.drunkModeTimer = PL.buffDuration * 1000 + g_time;
        sprintf(g_alertMsg, "DRUNK MODE");
        MSG_COLOR();
        g_msgTimer = g_time + 2000;
        break;

    case ITEM_GEM:  // gem pickup: every 100 gems drops a bonus (a 1000th triggers a "super" drop) and
                // enters STATE_GEM_DROP
        SoundPlay(g_sfxBell1, RandRange(22000, 41000), 0xff, g_pan, 0x7f, g_sndFlags);
        PL.gems = PL.gems + SHIP->gemStep;
        if ((PL.gems - SHIP->gemBase) / SHIP->gemStep % 100 == 0) {
            if (PL.trackKillsFlag != 0)
                PL.bonusKilled = PL.totalEnemies;
            SoundQueueAdd(g_sfxGemDrop, 50, 0);
            sprintf(g_alertMsg, "G E M   D R O P");
            g_msgTimer = g_time + 2000;
            if ((PL.gems - SHIP->gemBase) / SHIP->gemStep >= 1000) {
                g_superGemDrop = 1;
                PL.gems = SHIP->gemBase + SHIP->gemStep;
                sprintf(g_alertMsg, "S U P E R   G E M   D R O P");
                g_msgTimer = g_time + 3000;
                VOICE(secretFound30, 0x1e);
            }
            RELEASE_HELD();
            VOICE(secretFound12, 0xc);

            g_msgColor = 3;
            g_flashOverlayActive = 1;
            g_fadeStep = 0;
            g_fadeColorSet = 1;
            g_maxFallingGems = 1.0f;
            InitGemDropLevel();
            PlayGemDropMusic();
            g_gemDropIntroTimer = g_time + 4000;
            g_state = STATE_GEM_DROP;
            g_buttonsOn = 0;
            KInput::hidePointer();
        }

        ADD_SCORE(g_enemyScoreTable[OBF_1000], 0x26);
        AddScorePopup((int)PL.x + 5, (int)PL.y - 10, DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_1000]), 0);
        g_viewTransitionFlag = 2;
        g_stateFn = SetViewHud;
        EmptyViewChangeHook();
        g_drawBordersFn = DrawBorders;
        break;

    case ITEM_RANK_GEM_1:  // rank gem 1/6: sets MARK_1; a full A-R-T-X-E gem sequence bulk-adds 500 levels played
        GEMHDR(MARK_1, 0x27);
        if (PL.gemSeqA == 2 && PL.bulkLevelsCooldown == 0) {
            VOICE(secretFound19, 0x13);
            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                g_pendingLevelsPlayed = g_pendingLevelsPlayed + 500;
                AddLevelsPlayed(g_profileIndex, g_pendingLevelsPlayed);
                g_pendingLevelsPlayed = 0;
            }
            sprintf(g_alertMsg, "*** 500 LEVELS ADDED ***");
            PL.bulkLevelsCooldown = (int)DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_50]);
            MSG_COLOR();
            g_msgTimer = g_time + 3000;
        }
        PL.gemSeqA = -1;
        PL.gemSeqB = 1;
        break;

    case ITEM_RANK_GEM_2:  // rank gem 2/6
        GEMHDR(MARK_2, 0x28);
        GEMSEQ(4, 2, 1, 2);
        break;

    case ITEM_RANK_GEM_3:  // rank gem 3/6
        GEMHDR(MARK_3, 0x29);
        GEMSEQ(8, 4, 2, 4);
        break;

    case ITEM_RANK_GEM_4:  // rank gem 4/6
        GEMHDR(MARK_4, 0x2a);
        GEMSEQ(0x10, 8, 4, 8);
        break;

    case ITEM_RANK_GEM_5:  // rank gem 5/6
        GEMHDR(MARK_5, 0x2b);
        GEMSEQ(0x20, 0x10, 8, 0x10);
        break;

    case ITEM_RANK_GEM_6:  // rank gem 6/6: completing the set (with gemSeqB flagged) unlocks super autofire
        GEMHDR(MARK_6, 0x2c);
        PL.gemSeqA = 0x20;
        if (PL.gemSeqB == 0x10) {
            VOICE(secretFound10, 10);
            PL.autofire = 1;
            PL.superAuto = 1;
            PL.autofireInterval = 0x19;
            sprintf(g_alertMsg, "*** SUPER AUTO FIRE ON ***");
            MSG_COLOR();
            g_msgTimer = g_time + 3000;
        }
        PL.gemSeqB = -1;
        break;

    case ITEM_BAD_GEM:  // "OH NO" bad gem: clears the rank-gem marks/sequences and docks 2-4 rank
        SoundPlay(g_sfxBell1, RandRange(9000, 0x2774), 0xff, g_pan, 0x7f, g_sndFlags);
        SoundQueueAdd(g_sfxOhNo, 50, 0);
        VOICE(secretFound24, 0x18);
        PL.marks = 0;
        PL.gemSeqB = -1;
        PL.gemSeqA = -1;
        PL.rank -= RandRange(2, 5);
        if (PL.rank < 0)
            PL.rank = 0;
        PL.gemSeqB = -1;
        PL.gemSeqA = -1;
        break;

    case ITEM_EXTRA_BULLET_SPEED:  // EXTRA BULLET SPEED
        if (g_gameMode != MODE_TIME_TRIAL) {
            SoundQueueAdd(g_sfxExtraBullet, 50, 1);
            SoundQueueAdd(g_sfxSpeed, 50, 1);
        }
        if (PL.bulletSpeedMult < g_bulletSpeedMax) {
            PL.bulletSpeedMult += (double)0.1f;
            sprintf(g_alertMsg, "EXTRA BULLET SPEED");
            MSG_COLOR();
            g_msgTimer = g_time + 2000;
        } else {
            PL.bulletSpeedMult = g_bulletSpeedMax;
            ADD_SCORE(g_enemyScoreTable[OBF_25000_B], 0x2d);
            AddScorePopup((int)PL.x, (int)(PL.y - 30.0), DEOBFUSCATE_VALUE(g_enemyScoreTable[OBF_25000_B]), 0);
            VOICE(secretFound28, 0x1c);
        }
        break;
    }
}

#undef PL
#undef EN
#undef SHIP
#undef ADD_SCORE
#undef MSG_COLOR
#undef COMPLETE
#undef VOICE
#undef WEAPON_HAVE
#undef WEAPON_NEW
#undef SAVE_WEAPONS
#undef RELEASE_HELD
#undef PENALTY_HEAD
#undef CLAMP_UP
#undef TRIPLE_CHECK
#undef COUNTER
#undef SUCKER
#undef ADD_SCORE2
#undef MONEY
#undef GEMHDR
#undef GEMSEQ

// Turns pending "spawn a specific powerup" markers (item types 42-45) into live items
// (types 29-32), and activates the already-spawned scoop/coin item types (29-32, 38-41).
void SpawnItems()
{
    int i;
    int kind;
    for (i = 0; i < MAX_ITEMS; i++) {
        if (g_items[i].alive != 0) {
            if (g_items[i].type == ITEM_MONEY_SMALL_TALLY || g_items[i].type == ITEM_MONEY_MEDIUM_TALLY ||
                g_items[i].type == ITEM_MONEY_LARGE_TALLY || g_items[i].type == ITEM_MONEY_BLUE_TALLY) {
                kind = -1;
                if (g_items[i].type == ITEM_MONEY_SMALL_TALLY) kind = ITEM_MONEY_SMALL;
                if (g_items[i].type == ITEM_MONEY_MEDIUM_TALLY) kind = ITEM_MONEY_MEDIUM;
                if (g_items[i].type == ITEM_MONEY_LARGE_TALLY) kind = ITEM_MONEY_LARGE;
                if (g_items[i].type == ITEM_MONEY_BLUE_TALLY) kind = ITEM_MONEY_BLUE;

                if (kind != -1) {
                    g_items[i].active = 1;
                    g_items[i].alive = 1;
                    g_items[i].vx = 0;
                    g_items[i].vy = 1 + RandFloat(0, 1);
                    g_items[i].frameDelay = RandFloat(3, 7);
                    g_items[i].frameTimer = g_items[i].frameDelay;
                    g_items[i].gfx = g_gfxBonus;
                    g_items[i].hma = (int)g_hmaBonuses;
                    g_items[i].hmaW = g_bonusItemField34;
                    g_items[i].hmaH = g_bonusItemGfxH;
                    g_items[i].srcX = g_itemBonusSrcX[kind];
                    g_items[i].srcY = g_itemBonusSrcY[kind];
                    g_items[i].h = g_itemHeightTable[kind];
                    g_items[i].w = g_itemWidthTable[kind];
                    g_items[i].frameCount = g_itemFrameCountTable[kind];
                    g_items[i].frame = (float)RandRange(0, 10);
                    g_items[i].type = kind;

                    g_items[i].left = g_items[i].srcX;
                    g_items[i].top = g_items[i].srcY;
                    g_items[i].right = g_items[i].left + g_items[i].w;
                    g_items[i].bottom = g_items[i].top + g_items[i].h;
                } else {
                    g_items[i].alive = 0;
                }
            } else {
                if (g_items[i].type == ITEM_MONEY_SMALL || g_items[i].type == ITEM_MONEY_MEDIUM ||
                    g_items[i].type == ITEM_MONEY_LARGE || g_items[i].type == ITEM_MONEY_BLUE)
                    g_items[i].active = 1;
                if (g_items[i].type == ITEM_MONEY_SMALL_BURST || g_items[i].type == ITEM_MONEY_MEDIUM_BURST ||
                    g_items[i].type == ITEM_MONEY_LARGE_BURST || g_items[i].type == ITEM_MONEY_BLUE_BURST)
                    g_items[i].active = 1;
            }
        }
    }
}

// Per-frame physics for bonus/pickup items: during hyperspace scroll all but stars just track the
// screen scroll; otherwise a homing item (ITEM_GEM) drifts toward the player's x, exploded-fragment
// items (ITEM_MONEY_*_BURST) decay their velocity and convert to falling pickups when their timer expires,
// and score/tally items (ITEM_MONEY_*_TALLY) home in on the on-screen score counter and despawn once they
// arrive. All items animate their frame and wrap/despawn at the screen edges. Called every frame
// during bonus levels.
void UpdateItems()
{
    int i;
    float dx;
    float dy;
    float spd;

    for (i = 0; i < MAX_ITEMS; i++) {
        if (g_items[i].alive != 0) {
            if (g_save.players[g_curPlayer].hyperspaceFade > 0.0 && g_items[i].type != ITEM_STAR) {
                g_items[i].y += g_save.players[g_curPlayer].scrollSpeedY * g_frameDt;
            } else {
                if (g_items[i].type == ITEM_GEM && g_save.players[g_curPlayer].dead == 0) {
                    // Homing item: nudge toward whichever tracked player(s) are holding the
                    // "attract" input, 1 px/frame (scaled by dt) either side of their ship center.
                    if (g_gameMode == MODE_DUAL) {
                        if (InputDown(0)) {
                            if (g_save.players[0].x + 14.0 < g_items[i].x)
                                g_items[i].x -= 1.0 * g_frameDt;
                            if (g_save.players[0].x + 14.0 > g_items[i].x)
                                g_items[i].x += 1.0 * g_frameDt;
                        }
                        if (InputDown(1)) {
                            if (g_save.players[1].x + 14.0 < g_items[i].x)
                                g_items[i].x -= 1.0 * g_frameDt;
                            if (g_save.players[1].x + 14.0 > g_items[i].x)
                                g_items[i].x += 1.0 * g_frameDt;
                        }

                    } else {
                        if (InputDown(g_curPlayer)) {
                            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo)
                                g_itemSteered = 1;
                            if (g_save.players[g_curPlayer].x + 14.0 < g_items[i].x)
                                g_items[i].x -= 1.0 * g_frameDt;
                            if (g_save.players[g_curPlayer].x + 14.0 > g_items[i].x)
                                g_items[i].x += 1.0 * g_frameDt;
                        }
                    }
                    g_items[i].y += g_items[i].vy * g_frameDt;
                }

                if (g_items[i].type == ITEM_MONEY_SMALL_BURST || g_items[i].type == ITEM_MONEY_MEDIUM_BURST ||
                    g_items[i].type == ITEM_MONEY_LARGE_BURST || g_items[i].type == ITEM_MONEY_BLUE_BURST) {
                    // Exploded fragment: flies outward, decelerating (exponential drag), until its
                    // timer runs out, then converts to the matching falling-pickup type and starts
                    // dropping straight down.
                    g_items[i].x += g_items[i].vx * g_frameDt;
                    g_items[i].y += g_items[i].vy * g_frameDt;
                    g_items[i].vx /= 1 + g_frameDt * 0.02f;
                    g_items[i].vy /= 1 + g_frameDt * 0.02f;
                    g_items[i].timer -= 1.0 * g_frameDt;

                    if (g_items[i].timer < 0.0) {
                        if (g_items[i].type == ITEM_MONEY_SMALL_BURST)
                            g_items[i].type = ITEM_MONEY_SMALL;
                        if (g_items[i].type == ITEM_MONEY_MEDIUM_BURST)
                            g_items[i].type = ITEM_MONEY_MEDIUM;
                        if (g_items[i].type == ITEM_MONEY_LARGE_BURST)
                            g_items[i].type = ITEM_MONEY_LARGE;
                        if (g_items[i].type == ITEM_MONEY_BLUE_BURST)
                            g_items[i].type = ITEM_MONEY_BLUE;
                        g_items[i].vx = 0;
                        if (g_items[i].fastFall)
                            g_items[i].vy = 1 + RandFloat(1.0f, 6.0f);
                        else
                            g_items[i].vy = 1 + RandFloat(0.0f, 1.0f);
                    }
                }

                if (g_items[i].type == ITEM_MONEY_SMALL_TALLY || g_items[i].type == ITEM_MONEY_MEDIUM_TALLY ||
                    g_items[i].type == ITEM_MONEY_LARGE_TALLY || g_items[i].type == ITEM_MONEY_BLUE_TALLY) {
                    // Score-tally item: home in on the on-screen score target (g_targetX/Y) at a
                    // speed inversely proportional to its current vx (so slower items curve harder),
                    // and disappear once close enough to the target.
                    spd = g_items[i].vx;
                    if (spd == 0.0)
                        spd = 1;
                    dx = 0 - (g_items[i].x - g_targetX) / spd;
                    dy = 0 - (g_items[i].y - g_targetY) / spd;
                    g_items[i].vx /= 1 + g_frameDt * 0.025f;
                    if (g_items[i].x - g_targetX > -40.0 && g_items[i].x - g_targetX < 40.0 &&
                        g_items[i].y - g_targetY > -30.0 && g_items[i].y - g_targetY < 30.0)
                        g_items[i].alive = 0;
                    g_items[i].x += dx * g_frameDt;
                    g_items[i].y += dy * g_frameDt;
                } else {
                    g_items[i].x += g_items[i].vx * g_frameDt;
                    g_items[i].y += g_items[i].vy * g_frameDt;
                }

                g_items[i].frameTimer -= 1.0 * g_frameDt;
                if (g_items[i].frameTimer < 0.0) {
                    g_items[i].frameTimer = g_items[i].frameDelay;
                    g_items[i].frame += 1.0;
                    if (g_items[i].frame >= g_items[i].frameCount)
                        g_items[i].frame = 0;
                }

                if (g_items[i].y > g_screenH + 30)
                    g_items[i].alive = 0;
                if (g_items[i].x > g_screenW + 100)
                    g_items[i].x = -100.0f;
            }
        }
    }
}

// Moves the drifting starfield-item type (ITEM_STAR) across the screen and respawns it at a random
// height and speed once it exits the right edge. Called every frame.
void UpdateStarItems()
{
    int i;

    for (i = 0; i < MAX_ITEMS; i++) {
        if (g_items[i].alive == 1 && g_items[i].type == ITEM_STAR) {
            g_items[i].x += g_items[i].vx * g_frameDt;
            if (g_items[i].x > g_screenW + 100) {
                g_items[i].x = -100.0f;
                g_items[i].y = RandRange(0, g_screenH);
                g_items[i].vx = RandFloat(1.5f, 8.0f);
            }
        }
    }
}

// Draws all live bonus/item sprites, clipping each one's source rect against the visible screen
// area before blitting (items partly off the top/left/right/bottom get their source rect trimmed;
// fully off-screen items are skipped). Called once per frame during bonus levels.
void DrawSprites()
{
    Rect16 src;
    int dy;
    int dx;
    int off;
    int i;

    off = 0;
    g_noSpritesDrawn = 1;

    for (i = 0; i < MAX_ITEMS; i++) {
        if (g_items[i].alive == 1) {
            // NOTE: single-pass "loop" used as a multi-exit block (break = skip this sprite);
            // kept as-is for the byte match.
            while (1) {
                dy = 0;
                dx = 0;
                if (g_items[i].y > g_clipTop) {
                    src.y1 = g_items[i].srcY;
                } else if (g_clipTop - (int)g_items[i].y >= g_items[i].h) {
                    break;
                } else {
                    dy = g_clipTop - (int)g_items[i].y;
                    src.y1 = g_items[i].srcY + dy;
                }
                if (g_items[i].x > g_clipLeft) {
                    src.x1 = g_items[i].srcX;
                } else if (g_clipLeft - (int)g_items[i].x >= g_items[i].w) {
                    break;
                } else {
                    dx = g_clipLeft - (int)g_items[i].x;
                    src.x1 = g_items[i].srcX + dx;
                }

                src.x2 = g_items[i].w - dx + src.x1;
                if ((int)g_items[i].x + g_items[i].w - dx > g_clipRight) {
                    if (g_items[i].x > g_clipRight)
                        break;
                    else
                        src.x2 -= (int)g_items[i].x + g_items[i].w - dx - g_clipRight;
                }

                src.y2 = g_items[i].h - dy + src.y1;
                if ((int)g_items[i].y + (g_items[i].h - dy) > g_clipBottom) {
                    if (g_items[i].y > g_clipBottom)
                        break;
                    else
                        src.y2 -= (int)g_items[i].y + (g_items[i].h - dy) - g_clipBottom;
                }

                if (g_items[i].type == ITEM_GEM) {
                    // Homing item's sheet is laid out in a vertical strip (frames stacked by height).
                    off = (int)g_items[i].frame * g_items[i].h;
                    src.y1 += off;
                    src.y2 += off;
                } else {
                    // All other items use a horizontal strip (frames stacked by width).
                    off = (int)g_items[i].frame * g_items[i].w;
                    src.x1 += off;
                    src.x2 += off;
                }

                if (g_gameMode != MODE_TIME_TRIAL)
                    g_noSpritesDrawn = 0;
                QueueBlit(g_items[i].x + dx, g_items[i].y + dy,
                                 g_items[i].gfx, &src);
                break;
            }
        }
    }
}
