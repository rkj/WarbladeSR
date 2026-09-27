// savegame.cpp: Suspended games (profileNNN.svg) and their scrambling blocks, profile directories.
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"
#include "game.h"

enum {
    SAVE_BUFFER_SIZE     = 0xa9a18,  // sizeof(SaveData), the suspended-game save buffer
    SCRAMBLE_BLOCK_SIZE  = 1024,     // bytes per scrambling block (g_blocks[][])
    NUM_SCRAMBLE_BLOCKS  = 6
};


// Writes the suspended-game save for profile: scrambles a scratch buffer, stamps the
// save signature/timestamps, converts running timers to elapsed-time deltas so they can
// be resumed later, zlib-compresses g_save into the buffer, and writes it to
// profile%03d.svg.
void SaveProfile(int profile)
{
    if (profile > -1) {
        int fd = 0;
        int written;
        char savename[1024];
        char *buf;
        char *p;
        int size;
        int i;
        int res;
        int SD_uSize;
        int SD_cSize;

        sprintf(savename, "%s\\warblade\\profiles\\profile%03d.svg", KMiscTools::getUserFolder(), profile);
        size = SAVE_BUFFER_SIZE;
        buf = (char *)malloc(size);
        if (buf != 0) {
            p = buf;
            for (i = 0; i < size; i++, p++)
                *p = RandRange(0, 0xff);

            // ---- stamp the save signature and elapsed play time ----
            g_save.sig[0] = 'S';
            g_save.sig[1] = 'D';
            g_save.sig[2] = 'Y';
            StampTimeC();
            g_timeStampA.LowPart = g_timeA.LowPart;
            g_timeStampA.HighPart = g_timeA.HighPart;
            g_timeStampB.LowPart = g_timeMarkC.LowPart;
            g_timeStampB.HighPart = g_timeMarkC.HighPart;
            g_resumeTimeOffset = g_timeStampB.QuadPart - g_timeStampA.QuadPart - g_pausedDuration.QuadPart;
            g_savedDifficultySave = g_cfg.difficulty;

            // ---- convert running timers to elapsed-time deltas so they can resume later ----
            if (g_save.players[g_curPlayer].scoreMult2Timer != 0)
                g_save.players[g_curPlayer].scoreMult2Timer =
                    g_save.players[g_curPlayer].scoreMult2Timer - g_time;
            if (g_save.players[g_curPlayer].scoreMult5Timer != 0)
                g_save.players[g_curPlayer].scoreMult5Timer =
                    g_save.players[g_curPlayer].scoreMult5Timer - g_time;
            if (g_save.players[g_curPlayer].drunkModeTimer != 0)
                g_save.players[g_curPlayer].drunkModeTimer =
                    g_save.players[g_curPlayer].drunkModeTimer - g_time;
            if (g_save.players[g_curPlayer].mirrorTime != 0)
                g_save.players[g_curPlayer].mirrorTime = g_save.players[g_curPlayer].mirrorTime - g_time;
            if (g_save.players[g_curPlayer].shieldTimer != 0)
                g_save.players[g_curPlayer].shieldTimer = g_save.players[g_curPlayer].shieldTimer - g_time;

            // ---- compress g_save and write it out ----
            SD_uSize = size;
            SD_cSize = size;
            g_saveVersion = g_saveMagic;
            g_save.saveId = MakeRandomId();
            SetProfileLastSaveId(profile, g_save.saveId);
            if (g_playerBroke || g_freshStart)
                DecProfileLives(profile);
            res = compress((unsigned char *)buf, (unsigned long *)&SD_cSize,
                                  (const unsigned char *)&g_save, SD_uSize);
            _fmode = _O_BINARY;
            fd = _open(savename, _O_CREAT | _O_TRUNC | _O_RDWR, _S_IREAD | _S_IWRITE);
            if (fd != -1) {
                written = _write(fd, buf, SD_cSize);
                _close(fd);
            }
            free(buf);
        }
    }
}

// Loads the suspended-game save for profile: reads and zlib-decompresses profileNNN.svg
// into g_save, discards the save file, restores each player's ship gfx/hit-mask
// pointers (not part of the save data), and re-applies any active warp-portal hazard
// graphics. Converts saved timer deltas back to absolute times, recomputes the
// difficulty/fps setup, and routes into the shop screen to resume play.
// Sets `enemy`'s cached hazard graphics/hit-shot frame from alien-gfx cache slot `n` and
// bumps its LRU age (used when re-applying an active warp-portal hazard's graphics).
#define SET_HAZARD_GFX(enemy, n)                 \
    (enemy).gfxA = g_alienGfxCache[n].gfx1;       \
    (enemy).gfxB = g_alienGfxCache[n].gfx2;       \
    (enemy).shotFrame = g_alienGfxMem[n];         \
    (enemy).shotGfxW = g_hazard##n##GfxW;         \
    (enemy).shotGfxH = g_hazard##n##GfxH;         \
    BumpAlienGfxAge(g_alienGfxCache[n].gfx1);

void LoadSuspended(int profile)
{
    ULARGE_INTEGER gt;
    if (profile > -1) {
        int fh = 0;
        unsigned int n;
        char openname[1024];
        char *buf;
        char *backup;
        unsigned int size;
        unsigned long SD_uSize;
        unsigned long len;
        int res;
        int idx;
        int idx2;

        size = SAVE_BUFFER_SIZE;
        if (ProfileValid(profile)) {
            sprintf(openname, "%s\\warblade\\profiles\\profile%03d.svg",
                          KMiscTools::getUserFolder(), profile);
            buf = (char *)malloc(size);
            if (buf != 0) {
                backup = (char *)malloc(size);
                if (backup != 0) {
                    n = 0;
                    g_fileModeFlag = _O_BINARY;
                    fh = _open(openname, 0, 0);
                    if (fh != -1) {
                        n = _read(fh, buf, size);
                        _close(fh);
                    }

                    if (n > 0) {
                        memcpy(backup, &g_save, size);
                        SD_uSize = size;
                        len = size;
                        res = uncompress((unsigned char *)&g_save, (unsigned long *)&SD_uSize,
                                                (const unsigned char *)buf, len);

                        if (res == 0) {
                            ProfileHistPush(profile, g_save.saveId);
                            DeleteProfile(profile);

                            // ---- restore ship gfx/hit-mask pointers (not part of the save data) ----
                            g_save.players[0].gfx = g_gfxFighter1;
                            g_save.players[0].hitMask = (int)g_ship1Hma;
                            g_save.players[0].hitMaskParamA = g_ship1GfxParamA;
                            g_save.players[0].hitMaskParamB = g_ship1GfxParamB;
                            g_save.players[1].gfx = g_gfxFighter2;
                            g_save.players[1].hitMask = (int)g_ship2Hma;
                            g_save.players[1].hitMaskParamA = g_ship2GfxParamA;
                            g_save.players[1].hitMaskParamB = g_ship2GfxParamB;
                            ResetObjectsKeep();

                            // ---- re-apply any active warp-portal hazard graphics ----
                            g_save.players[g_curPlayer].energy = 0;
                            if (g_save.players[g_curPlayer].alienLock == 0) {
                                g_save.players[g_curPlayer].shieldL = 0;
                                g_save.players[g_curPlayer].shieldR = 0;
                            } else {
                                // ---- left shield's portal ----
                                if (g_warpLevelL != -1 && g_save.players[g_curPlayer].shieldL != 0) {
                                    BlocksRandomize();
                                    g_curLevelNum = g_warpLevelL;
                                    LoadLevelData();
                                    idx = g_save.players[g_curPlayer].shieldLIdx;
                                    switch (g_enemies[g_curPlayer][idx].hazardType) {
                                    case 1:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 0)
                                        break;

                                    case 2:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 1)
                                        break;

                                    case 3:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 2)
                                        break;

                                    case 4:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 3)
                                        break;

                                    case 5:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 4)
                                        break;

                                    case 6:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 5)
                                        break;
                                    }
                                    g_portalGfxW = HAZARD_GFX_W;
                                    g_portalGfxH = HAZARD_GFX_H;
                                }

                                // ---- right shield's portal ----
                                if (g_warpLevelR != -1 && g_save.players[g_curPlayer].shieldR != 0) {
                                    BlocksRandomize();
                                    g_curLevelNum = g_warpLevelR;
                                    LoadLevelData();
                                    idx2 = g_save.players[g_curPlayer].shieldRIdx;
                                    switch (g_enemies[g_curPlayer][idx2].hazardType) {
                                    case 1:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 0)
                                        break;

                                    case 2:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 1)
                                        break;

                                    case 3:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 2)
                                        break;

                                    case 4:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 3)
                                        break;

                                    case 5:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 4)
                                        break;

                                    case 6:
                                        SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 5)
                                        break;
                                    }
                                    g_portalGfxW2 = HAZARD_GFX_W;
                                    g_portalGfxH2 = HAZARD_GFX_H;
                                }
                            }

                            // ---- rebase the elapsed-time clock past the suspended period ----
                            g_save.players[g_curPlayer].freezeTimer = 0;
                            EmptyPostTransitionHook();
                            StampTimeA();
                            gt.HighPart = g_timeA.HighPart;
                            gt.LowPart = g_timeA.LowPart;
                            if (g_resumeTimeOffset < 0)
                                g_resumeTimeOffset = 0;
                            gt.QuadPart = gt.QuadPart - g_resumeTimeOffset;
                            g_timeA.HighPart = gt.HighPart;
                            g_timeA.LowPart = gt.LowPart;

                            // ---- convert saved timer deltas back to absolute times ----
                            if (g_save.players[g_curPlayer].scoreMult2Timer != 0) {
                                g_save.players[g_curPlayer].scoreMult2Timer =
                                    g_save.players[g_curPlayer].scoreMult2Timer + g_time;
                                g_scoreMul[g_curPlayer] = 2;
                            }
                            if (g_save.players[g_curPlayer].scoreMult5Timer != 0) {
                                g_save.players[g_curPlayer].scoreMult5Timer =
                                    g_save.players[g_curPlayer].scoreMult5Timer + g_time;
                                g_scoreMul[g_curPlayer] = 5;
                            }
                            if (g_save.players[g_curPlayer].drunkModeTimer != 0)
                                g_save.players[g_curPlayer].drunkModeTimer =
                                    g_save.players[g_curPlayer].drunkModeTimer + g_time;
                            if (g_save.players[g_curPlayer].mirrorTime != 0)
                                g_save.players[g_curPlayer].mirrorTime =
                                    g_save.players[g_curPlayer].mirrorTime + g_time;
                            if (g_save.players[g_curPlayer].shieldTimer != 0)
                                g_save.players[g_curPlayer].shieldTimer =
                                    g_save.players[g_curPlayer].shieldTimer + g_time;

                            // ---- recompute the difficulty/fps setup for the restored level ----
                            g_bonusResultsTime = 0;
                            g_cfg.difficulty = g_savedDifficultySave;
                            if (g_cfg.difficulty == DIFF_EASY)
                                g_cfg.fps = 0x32;
                            if (g_cfg.difficulty == DIFF_NORMAL)
                                g_cfg.fps = 0x3c;
                            if (g_cfg.difficulty == DIFF_HARD)
                                g_cfg.fps = 0x46;
                            if (g_cfg.difficulty == DIFF_ACE)
                                g_cfg.fps = 0x50;
                            g_save.players[g_curPlayer].gameSpeedSetting = g_cfg.fps;
                            SetupDifficulty();
                            DoNothing();
                            EmptyPostTransitionHook();

                            // ---- route into the shop screen to resume play ----
                            g_shopCurPlayer = 0;
                            g_extraLifeGranted = 0;
                            CheckProfileBonus();
                            PlayShopMusic();
                            if (g_save.players[g_shopCurPlayer].superAuto != 0)
                                g_save.players[g_shopCurPlayer].autofireInterval = 0x19;
                            if (g_save.players[g_shopCurPlayer].autofireUnlocked == 0)
                                g_save.players[g_shopCurPlayer].autofire =
                                    g_save.players[g_shopCurPlayer].superAuto;
                            g_save.players[g_shopCurPlayer].autofireTimer = g_time;
                            g_save.players[g_shopCurPlayer].nextShotSnd = g_time;
                            if (g_save.players[g_shopCurPlayer].level > 100)
                                g_bonusWeight[36] = 0x3c;
                            g_shopTransition = 500.0f;
                            g_state = STATE_SHOP;
                            g_buttonsOn = 0;
                            KInput::hidePointer();

                            // ---- reset the per-frame draw-call counters ----
                            g_blitCount = 0;
                            g_blit2Count = 0;
                            g_quadCount = 0;
                            g_stretchFCount = 0;
                            g_stretchRotCount = 0;
                            g_stretchRot2Count = 0;
                        }
                    }
                    free(backup);
                }
                free(buf);
            }
        }
    }
}
#undef SET_HAZARD_GFX

// Checks whether profile's save file exists and decompresses to a valid, current-version
// save (signature "SDY" and matching g_saveMagic). Decompresses into g_save to check it,
// then restores g_save from a backup taken beforehand so the check has no side effect.
bool ProfileValid(int profile)
{
    bool ok = false;
    if (profile > -1) {
        int fh = 0;
        unsigned int n;
        char openname[1024];
        char *buf;
        char *backup;
        unsigned int size;
        unsigned long SD_uSize;
        unsigned long len;
        int res;

        size = SAVE_BUFFER_SIZE;
        sprintf(openname, "%s\\warblade\\profiles\\profile%03d.svg", KMiscTools::getUserFolder(), profile);
        buf = (char *)malloc(size);
        if (buf != 0) {
            backup = (char *)malloc(size);
            if (backup != 0) {
                n = 0;
                g_fileModeFlag = _O_BINARY;
                fh = _open(openname, 0, 0);
                if (fh != -1) {
                    n = _read(fh, buf, size);
                    _close(fh);
                }

                if (n > 0) {
                    memcpy(backup, &g_save, size);
                    SD_uSize = size;
                    len = size;
                    res = uncompress((unsigned char *)&g_save, (unsigned long *)&SD_uSize,
                                            (const unsigned char *)buf, len);
                    if (res == 0 && g_save.sig[0] == 'S' && g_save.sig[1] == 'D' && g_save.sig[2] == 'Y'
                        && g_saveMagic == g_saveVersion)
                        ok = true;
                    memcpy(&g_save, backup, size);
                }
                free(backup);
            }
            free(buf);
        }
    }
    return ok;
}

// Deletes profile's suspended-game save file.
void DeleteProfile(int profile)
{
    char openname[512];
    if (profile > -1) {
        sprintf(openname, "%s\\warblade\\profiles\\profile%03d.svg", KMiscTools::getUserFolder(), profile);
        DeleteFileA(openname);
    }
}

// Creates the profiles directory under the user's Warblade folder if it doesn't exist.
void MakeProfilesDir()
{
    char path[512];
    sprintf(path, "%s\\warblade\\profiles", KMiscTools::getUserFolder());
    bool exists = GetFileAttributesA(path) != 0xffffffff;
    if (!exists)
        CreateDirectoryA(path, 0);
}

// Compares block `idx` (SCRAMBLE_BLOCK_SIZE bytes) against `buf` byte by byte.
bool BlockEquals(int idx, unsigned char *buf)
{
    bool same = true;
    int i;

    i = 0;
    do {
        if (g_blocks[idx][i] != buf[i])
            same = false;
        i++;
    } while (i < SCRAMBLE_BLOCK_SIZE && same);
    return same;
}

// Copies SCRAMBLE_BLOCK_SIZE bytes from `buf` into block `idx`.
void BlockSet(int idx, unsigned char *buf)
{
    int i;

    for (i = 0; i < SCRAMBLE_BLOCK_SIZE; i++)
        g_blocks[idx][i] = buf[i];
}

// Fills all NUM_SCRAMBLE_BLOCKS data blocks with random bytes (used for garbage/noise content).
void BlocksRandomize()
{
    int i;
    int j;

    for (i = 0; i < NUM_SCRAMBLE_BLOCKS; i++)
        for (j = 0; j < SCRAMBLE_BLOCK_SIZE; j++)
            g_blocks[i][j] = RandRange(0, 0xff);
}
