// savefile.c: The suspended-game file format (profileNNN.svg), field by field.
//
// The original game compressed the raw 0xa9a18 bytes of memory from g_save to the end of
// g_killCount: 61 globals in the exe's address order, pointers included. SaveFile describes
// that image explicitly, so the globals can move, shrink or disappear and existing saves
// still load. Generated once by tools/savefile_gen.py from types.h and globals.c; since
// the file format is fixed, it is maintained by hand from now on and must not follow
// later changes to the game's structs.
//
// Only fixed-size types (int32_t, int64_t, float, char arrays), so the layout doesn't depend
// on the compiler; the C_ASSERTs pin it.
//
// Dead zones (dead_* and pad_* members): pointers the original stored (they are stale in
// any file), struct padding and filler globals. They are written as 0 and ignored when
// loading; the game rebuilds the pointers after a load (LoadSuspended).
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"
#include "game.h"
#include <zlib.h>

#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e) ? 1 : -1]
#endif


typedef struct Rect16File Rect16File;
typedef struct FrameSetFile FrameSetFile;
typedef struct LevelRecFile LevelRecFile;
typedef struct PlayerFile PlayerFile;
typedef struct EnemyFile EnemyFile;
typedef struct SaveDataFile SaveDataFile;
typedef struct SaveFile SaveFile;


// Copies one field between the game and the file; both sides must have the same size.
#define FIELD(dst, src) \
    ((void)sizeof(char[sizeof(dst) == sizeof(src) ? 1 : -1]), memcpy(&(dst), &(src), sizeof(dst)))

struct Rect16File {   // 0x10 bytes
    union {
        struct {
            int32_t x1; // +0x0
            int32_t y1; // +0x4
            int32_t x2; // +0x8
            int32_t y2; // +0xc
        };
        struct {
            int32_t v[4]; // +0x0
        };
    };
};
C_ASSERT(sizeof(Rect16File) == 0x10);
C_ASSERT(offsetof(Rect16File, x1) == 0x0);
C_ASSERT(offsetof(Rect16File, y1) == 0x4);
C_ASSERT(offsetof(Rect16File, x2) == 0x8);
C_ASSERT(offsetof(Rect16File, y2) == 0xc);
C_ASSERT(offsetof(Rect16File, v) == 0x0);

struct FrameSetFile {   // 0x20 bytes
    union {
        struct {
            int32_t bLeft; // +0x0
            int32_t bTop; // +0x4
            int32_t bWidth; // +0x8
            int32_t bHeight; // +0xc
            int32_t aLeft; // +0x10
            int32_t aTop; // +0x14
            int32_t aWidth; // +0x18
            int32_t aHeight; // +0x1c
        };
        struct {
            int32_t raw[8]; // +0x0
        };
    };
};
C_ASSERT(sizeof(FrameSetFile) == 0x20);
C_ASSERT(offsetof(FrameSetFile, bLeft) == 0x0);
C_ASSERT(offsetof(FrameSetFile, bTop) == 0x4);
C_ASSERT(offsetof(FrameSetFile, bWidth) == 0x8);
C_ASSERT(offsetof(FrameSetFile, bHeight) == 0xc);
C_ASSERT(offsetof(FrameSetFile, aLeft) == 0x10);
C_ASSERT(offsetof(FrameSetFile, aTop) == 0x14);
C_ASSERT(offsetof(FrameSetFile, aWidth) == 0x18);
C_ASSERT(offsetof(FrameSetFile, aHeight) == 0x1c);
C_ASSERT(offsetof(FrameSetFile, raw) == 0x0);

struct LevelRecFile {   // 0x20 bytes
    union {
        struct {
            int64_t score; // +0x0
            int32_t money; // +0x8
            int32_t shots; // +0xc
            char verify0; // +0x10
            uint8_t verify1; // +0x11
            char verify2; // +0x12
            uint8_t livesGainedByte; // +0x13
            uint8_t deathsByte; // +0x14
            uint8_t armourAddedByte; // +0x15
            uint8_t verify3; // +0x16
            char verify4; // +0x17
            char rank; // +0x18
            uint8_t verify5; // +0x19
            uint8_t verify6; // +0x1a
            uint8_t frameBucket; // +0x1b
        };
        struct {
            char data[32]; // +0x0
        };
    };
};
C_ASSERT(sizeof(LevelRecFile) == 0x20);
C_ASSERT(offsetof(LevelRecFile, score) == 0x0);
C_ASSERT(offsetof(LevelRecFile, money) == 0x8);
C_ASSERT(offsetof(LevelRecFile, shots) == 0xc);
C_ASSERT(offsetof(LevelRecFile, verify0) == 0x10);
C_ASSERT(offsetof(LevelRecFile, verify1) == 0x11);
C_ASSERT(offsetof(LevelRecFile, verify2) == 0x12);
C_ASSERT(offsetof(LevelRecFile, livesGainedByte) == 0x13);
C_ASSERT(offsetof(LevelRecFile, deathsByte) == 0x14);
C_ASSERT(offsetof(LevelRecFile, armourAddedByte) == 0x15);
C_ASSERT(offsetof(LevelRecFile, verify3) == 0x16);
C_ASSERT(offsetof(LevelRecFile, verify4) == 0x17);
C_ASSERT(offsetof(LevelRecFile, rank) == 0x18);
C_ASSERT(offsetof(LevelRecFile, verify5) == 0x19);
C_ASSERT(offsetof(LevelRecFile, verify6) == 0x1a);
C_ASSERT(offsetof(LevelRecFile, frameBucket) == 0x1b);
C_ASSERT(offsetof(LevelRecFile, data) == 0x0);

struct PlayerFile {   // 0x4d8 bytes
    int32_t inputDevice; // +0x0
    float x; // +0x4
    float y; // +0x8
    float mirrorX; // +0xc
    int32_t extraLetterE; // +0x10
    float speed; // +0x14
    float bank; // +0x18
    float writeOnlyF1c; // +0x1c
    char pad_20[0x20]; // +0x20 (dead)
    int32_t dead_gfx; // +0x40 (dead: was KGraphic *)
    int32_t dead_hitMask; // +0x44 (dead: was the ship's hit-mask pointer)
    int32_t hitMaskParamA; // +0x48
    int32_t hitMaskParamB; // +0x4c
    Rect16File box; // +0x50
    int32_t shots; // +0x60
    int32_t hits; // +0x64
    int32_t lives; // +0x68
    int32_t deaths; // +0x6c
    int32_t collisionsTaken; // +0x70
    int32_t extraLetterR; // +0x74
    int64_t score; // +0x78
    int32_t autofire; // +0x80
    int32_t autofireUnlocked; // +0x84
    int32_t turretTrackingReduction; // +0x88
    char pad_8c[0x4]; // +0x8c (dead)
    union {
        struct {
            int32_t meteorBonus; // +0x90
        };
        struct {
            int64_t bonusRoundScore; // +0x90
        };
    };
    int32_t gemCounterCollected; // +0x98
    char pad_9c[0x4]; // +0x9c (dead)
    int64_t bonusHighScore; // +0xa0
    int32_t superAuto; // +0xa8
    int32_t money; // +0xac
    int32_t rank; // +0xb0
    int32_t bestRank; // +0xb4
    float bonusRoundCount; // +0xb8
    int32_t gems; // +0xbc
    int32_t bombPickups; // +0xc0
    float marks; // +0xc4
    int32_t extraLetterX; // +0xc8
    uint8_t color; // +0xcc
    char pad_cd[0x3]; // +0xcd (dead)
    int32_t secretBirdHits; // +0xd0
    int32_t level; // +0xd4
    int32_t gemSeqB; // +0xd8
    int32_t gemSeqA; // +0xdc
    int32_t pickupCount; // +0xe0
    char extraProgress; // +0xe4
    char artxeProgress; // +0xe5
    char pad_e6[0x2]; // +0xe6 (dead)
    int32_t displayLevel; // +0xe8
    int16_t unusedScoreThreshold; // +0xec
    char pad_ee[0x6]; // +0xee (dead)
    int32_t extraLetterA; // +0xf4
    int32_t msMultiplierActive; // +0xf8
    int32_t writeOnlyMirrorFlag; // +0xfc
    int32_t armour; // +0x100
    int32_t freezeTimer; // +0x104
    int32_t extraLetterT; // +0x108
    int32_t scoreMult2Timer; // +0x10c
    int32_t scoreMult5Timer; // +0x110
    int32_t mirrorTime; // +0x114
    uint32_t drunkModeTimer; // +0x118
    uint32_t scoopTimer; // +0x11c
    uint32_t shieldTimer; // +0x120
    int32_t blueMoneyActive; // +0x124
    int32_t unusedInvulnBlinkFrame; // +0x128
    int32_t unused12c; // +0x12c
    int32_t unusedInvulnBlinkTick; // +0x130
    int32_t autofireTimer; // +0x134
    int32_t autofireInterval; // +0x138
    int32_t shieldL; // +0x13c
    int32_t shieldLIdx; // +0x140
    int32_t shieldR; // +0x144
    int32_t shieldRIdx; // +0x148
    union {
        struct {
            int16_t weapon; // +0x14c
            int16_t bullets; // +0x14e
        };
        struct {
            int32_t weaponAmmoPacked; // +0x14c
        };
    };
    int16_t energy; // +0x150
    int16_t unused152; // +0x152
    float bulletSpeedMult; // +0x154
    int32_t buffDuration; // +0x158
    char pad_15c[0x4]; // +0x15c (dead)
    int32_t dead; // +0x160
    int32_t unused164; // +0x164
    uint32_t respawnTime; // +0x168
    int32_t time; // +0x16c
    int32_t shopVisits; // +0x170
    float shieldHitFlashSpeed; // +0x174
    int32_t enemyHpBonusRoll; // +0x178
    int32_t hurryupComboCount; // +0x17c
    int32_t rows; // +0x180
    int32_t cols; // +0x184
    int32_t rocketsFired; // +0x188
    int32_t memoryGridUpgradeStreak; // +0x18c
    int64_t memoryBonus; // +0x190
    int32_t gemPickups; // +0x198
    int32_t unused19c; // +0x19c
    int32_t shieldGlowG; // +0x1a0
    int32_t shieldGlowB; // +0x1a4
    int32_t unused1a8; // +0x1a8
    int32_t shieldGlowGStep; // +0x1ac
    int32_t shieldGlowBStep; // +0x1b0
    int32_t unusedF1b4_5; // +0x1b4
    int32_t unusedF1b8_1; // +0x1b8
    int32_t placeSlot; // +0x1bc
    float flame; // +0x1c0
    int32_t shipDestroyedThisLevel; // +0x1c4
    int32_t trackKillsFlag; // +0x1c8
    int32_t bonusRoundEnded; // +0x1cc
    int32_t levelEnemyDataCount; // +0x1d0
    int32_t unusedF1d4; // +0x1d4
    int32_t ship; // +0x1d8
    int32_t unusedF1dc; // +0x1dc
    float enemySwayX; // +0x1e0
    float enemySwayY; // +0x1e4
    char pad_1e8[0x8]; // +0x1e8 (dead)
    float enemySwayVelX; // +0x1f0
    float enemySwayAccel; // +0x1f4
    float enemySwayMax; // +0x1f8
    float enemySwayMin; // +0x1fc
    char pad_200[0x8]; // +0x200 (dead)
    float unusedF208; // +0x208
    float unusedF20c; // +0x20c
    float unusedF210; // +0x210
    float unusedF214; // +0x214
    int32_t moneyMax; // +0x218
    int32_t moneyMaxShown; // +0x21c
    int32_t unusedF220; // +0x220
    int32_t unusedF224; // +0x224
    int32_t primaryEnemyCount; // +0x228
    int32_t secondaryEnemyCount; // +0x22c
    int32_t totalEnemies; // +0x230
    int32_t spawnReserve; // +0x234
    int32_t killed; // +0x238
    int32_t escaped; // +0x23c
    int32_t done; // +0x240
    int32_t doneTime; // +0x244
    int64_t chainBonusValue; // +0x248
    int32_t started; // +0x250
    float starVelX; // +0x254
    float starSpeed; // +0x258
    float starVelZ; // +0x25c
    float savedRaceStarSpeed; // +0x260
    float hyperspaceOutTimer; // +0x264
    float hyperspaceMidTimer; // +0x268
    float hyperspaceInTimer; // +0x26c
    float hyperspaceInDuration; // +0x270
    float hyperspaceFade; // +0x274
    float savedHyperspaceFade; // +0x278
    float savedHyperspaceInDuration; // +0x27c
    float savedHyperspaceInTimer; // +0x280
    float savedHyperspaceOutTimer; // +0x284
    float savedStarSpeed; // +0x288
    float savedStarVelZ; // +0x28c
    float savedScrollSpeedY; // +0x290
    float savedHyperspaceMidTimer; // +0x294
    float scrollSpeedY; // +0x298
    int32_t levelFinished; // +0x29c
    int32_t levelTransitioning; // +0x2a0
    int32_t effectDuration; // +0x2a4
    int32_t levelWarpPending; // +0x2a8
    int32_t keyLatchFire; // +0x2ac
    int32_t keyLatchRocket; // +0x2b0
    int32_t keyLatchPause; // +0x2b4
    int32_t keyLatchProfile; // +0x2b8
    int32_t bonusTally; // +0x2bc
    int32_t bonusTallyTick; // +0x2c0
    float bonusTallyDelay; // +0x2c4
    float perfectTextSpacing; // +0x2c8
    float perfectTextSpacingVel; // +0x2cc
    int32_t perfectDone; // +0x2d0
    int32_t bonusResultsInitDone; // +0x2d4
    int32_t bonusKilled; // +0x2d8
    int32_t gemCounterPicks; // +0x2dc
    int32_t gemCounterUnlocked; // +0x2e0
    int32_t highScoreMilestone; // +0x2e4
    int32_t blueMoneyPicks; // +0x2e8
    int32_t blueMoneyUnlocked; // +0x2ec
    int32_t multiplierPicks; // +0x2f0
    int32_t multiplierUnlocked; // +0x2f4
    int32_t weaponFloorAtOne; // +0x2f8
    union {
        struct {
            int32_t secretFound01; // +0x2fc
            int32_t secretFound02; // +0x300
            int32_t secretFound03; // +0x304
            int32_t secretFound04; // +0x308
            int32_t secretFound05; // +0x30c
            int32_t secretFound06; // +0x310
            int32_t secretFound07; // +0x314
            int32_t secretFound08; // +0x318
            int32_t secretFound09; // +0x31c
            int32_t secretFound10; // +0x320
            int32_t secretFound11; // +0x324
            int32_t secretFound12; // +0x328
            int32_t secretFound13; // +0x32c
            int32_t secretFound14; // +0x330
            int32_t secretFound15; // +0x334
            int32_t secretFound16; // +0x338
            int32_t secretFound17; // +0x33c
            int32_t secretFound18; // +0x340
            int32_t secretFound19; // +0x344
            int32_t secretFound20; // +0x348
            int32_t secretFound21; // +0x34c
            int32_t secretFound22; // +0x350
            int32_t secretFound23; // +0x354
            int32_t secretFound24; // +0x358
            int32_t secretFound25; // +0x35c
            int32_t secretFound26; // +0x360
            int32_t secretFound27; // +0x364
            int32_t secretFound28; // +0x368
            int32_t secretFound29; // +0x36c
            int32_t secretFound30; // +0x370
            int32_t secretFound31to50[20]; // +0x374
            int32_t secretSeen[50]; // +0x3c4
        };
        struct {
            int32_t secretFlags[100]; // +0x2fc
        };
    };
    int32_t secretCount; // +0x48c
    int64_t bonusRoundPoints; // +0x490
    int32_t perfectStreak; // +0x498
    int32_t drunkStreak; // +0x49c
    int32_t rockets; // +0x4a0
    uint8_t alienLock; // +0x4a4
    char pad_4a5[0x3]; // +0x4a5 (dead)
    int32_t tries; // +0x4a8
    uint8_t levelMilestoneHandled; // +0x4ac
    char pad_4ad[0x3]; // +0x4ad (dead)
    int32_t secretBirdCounter; // +0x4b0
    int32_t secretBirdTick; // +0x4b4
    int32_t raceDistance; // +0x4b8
    int32_t gameSpeedSetting; // +0x4bc
    uint8_t maxRankReached; // +0x4c0
    char pad_4c1[0x3]; // +0x4c1 (dead)
    int32_t bulkLevelsCooldown; // +0x4c4
    int32_t nextShotSnd; // +0x4c8
    char pad_4cc[0x4]; // +0x4cc (dead)
    int64_t sessionPlayTime; // +0x4d0
};
C_ASSERT(sizeof(PlayerFile) == 0x4d8);
C_ASSERT(offsetof(PlayerFile, inputDevice) == 0x0);
C_ASSERT(offsetof(PlayerFile, x) == 0x4);
C_ASSERT(offsetof(PlayerFile, y) == 0x8);
C_ASSERT(offsetof(PlayerFile, mirrorX) == 0xc);
C_ASSERT(offsetof(PlayerFile, extraLetterE) == 0x10);
C_ASSERT(offsetof(PlayerFile, speed) == 0x14);
C_ASSERT(offsetof(PlayerFile, bank) == 0x18);
C_ASSERT(offsetof(PlayerFile, writeOnlyF1c) == 0x1c);
C_ASSERT(offsetof(PlayerFile, pad_20) == 0x20);
C_ASSERT(offsetof(PlayerFile, dead_gfx) == 0x40);
C_ASSERT(offsetof(PlayerFile, dead_hitMask) == 0x44);
C_ASSERT(offsetof(PlayerFile, hitMaskParamA) == 0x48);
C_ASSERT(offsetof(PlayerFile, hitMaskParamB) == 0x4c);
C_ASSERT(offsetof(PlayerFile, box) == 0x50);
C_ASSERT(offsetof(PlayerFile, shots) == 0x60);
C_ASSERT(offsetof(PlayerFile, hits) == 0x64);
C_ASSERT(offsetof(PlayerFile, lives) == 0x68);
C_ASSERT(offsetof(PlayerFile, deaths) == 0x6c);
C_ASSERT(offsetof(PlayerFile, collisionsTaken) == 0x70);
C_ASSERT(offsetof(PlayerFile, extraLetterR) == 0x74);
C_ASSERT(offsetof(PlayerFile, score) == 0x78);
C_ASSERT(offsetof(PlayerFile, autofire) == 0x80);
C_ASSERT(offsetof(PlayerFile, autofireUnlocked) == 0x84);
C_ASSERT(offsetof(PlayerFile, turretTrackingReduction) == 0x88);
C_ASSERT(offsetof(PlayerFile, pad_8c) == 0x8c);
C_ASSERT(offsetof(PlayerFile, meteorBonus) == 0x90);
C_ASSERT(offsetof(PlayerFile, bonusRoundScore) == 0x90);
C_ASSERT(offsetof(PlayerFile, gemCounterCollected) == 0x98);
C_ASSERT(offsetof(PlayerFile, pad_9c) == 0x9c);
C_ASSERT(offsetof(PlayerFile, bonusHighScore) == 0xa0);
C_ASSERT(offsetof(PlayerFile, superAuto) == 0xa8);
C_ASSERT(offsetof(PlayerFile, money) == 0xac);
C_ASSERT(offsetof(PlayerFile, rank) == 0xb0);
C_ASSERT(offsetof(PlayerFile, bestRank) == 0xb4);
C_ASSERT(offsetof(PlayerFile, bonusRoundCount) == 0xb8);
C_ASSERT(offsetof(PlayerFile, gems) == 0xbc);
C_ASSERT(offsetof(PlayerFile, bombPickups) == 0xc0);
C_ASSERT(offsetof(PlayerFile, marks) == 0xc4);
C_ASSERT(offsetof(PlayerFile, extraLetterX) == 0xc8);
C_ASSERT(offsetof(PlayerFile, color) == 0xcc);
C_ASSERT(offsetof(PlayerFile, pad_cd) == 0xcd);
C_ASSERT(offsetof(PlayerFile, secretBirdHits) == 0xd0);
C_ASSERT(offsetof(PlayerFile, level) == 0xd4);
C_ASSERT(offsetof(PlayerFile, gemSeqB) == 0xd8);
C_ASSERT(offsetof(PlayerFile, gemSeqA) == 0xdc);
C_ASSERT(offsetof(PlayerFile, pickupCount) == 0xe0);
C_ASSERT(offsetof(PlayerFile, extraProgress) == 0xe4);
C_ASSERT(offsetof(PlayerFile, artxeProgress) == 0xe5);
C_ASSERT(offsetof(PlayerFile, pad_e6) == 0xe6);
C_ASSERT(offsetof(PlayerFile, displayLevel) == 0xe8);
C_ASSERT(offsetof(PlayerFile, unusedScoreThreshold) == 0xec);
C_ASSERT(offsetof(PlayerFile, pad_ee) == 0xee);
C_ASSERT(offsetof(PlayerFile, extraLetterA) == 0xf4);
C_ASSERT(offsetof(PlayerFile, msMultiplierActive) == 0xf8);
C_ASSERT(offsetof(PlayerFile, writeOnlyMirrorFlag) == 0xfc);
C_ASSERT(offsetof(PlayerFile, armour) == 0x100);
C_ASSERT(offsetof(PlayerFile, freezeTimer) == 0x104);
C_ASSERT(offsetof(PlayerFile, extraLetterT) == 0x108);
C_ASSERT(offsetof(PlayerFile, scoreMult2Timer) == 0x10c);
C_ASSERT(offsetof(PlayerFile, scoreMult5Timer) == 0x110);
C_ASSERT(offsetof(PlayerFile, mirrorTime) == 0x114);
C_ASSERT(offsetof(PlayerFile, drunkModeTimer) == 0x118);
C_ASSERT(offsetof(PlayerFile, scoopTimer) == 0x11c);
C_ASSERT(offsetof(PlayerFile, shieldTimer) == 0x120);
C_ASSERT(offsetof(PlayerFile, blueMoneyActive) == 0x124);
C_ASSERT(offsetof(PlayerFile, unusedInvulnBlinkFrame) == 0x128);
C_ASSERT(offsetof(PlayerFile, unused12c) == 0x12c);
C_ASSERT(offsetof(PlayerFile, unusedInvulnBlinkTick) == 0x130);
C_ASSERT(offsetof(PlayerFile, autofireTimer) == 0x134);
C_ASSERT(offsetof(PlayerFile, autofireInterval) == 0x138);
C_ASSERT(offsetof(PlayerFile, shieldL) == 0x13c);
C_ASSERT(offsetof(PlayerFile, shieldLIdx) == 0x140);
C_ASSERT(offsetof(PlayerFile, shieldR) == 0x144);
C_ASSERT(offsetof(PlayerFile, shieldRIdx) == 0x148);
C_ASSERT(offsetof(PlayerFile, weapon) == 0x14c);
C_ASSERT(offsetof(PlayerFile, bullets) == 0x14e);
C_ASSERT(offsetof(PlayerFile, weaponAmmoPacked) == 0x14c);
C_ASSERT(offsetof(PlayerFile, energy) == 0x150);
C_ASSERT(offsetof(PlayerFile, unused152) == 0x152);
C_ASSERT(offsetof(PlayerFile, bulletSpeedMult) == 0x154);
C_ASSERT(offsetof(PlayerFile, buffDuration) == 0x158);
C_ASSERT(offsetof(PlayerFile, pad_15c) == 0x15c);
C_ASSERT(offsetof(PlayerFile, dead) == 0x160);
C_ASSERT(offsetof(PlayerFile, unused164) == 0x164);
C_ASSERT(offsetof(PlayerFile, respawnTime) == 0x168);
C_ASSERT(offsetof(PlayerFile, time) == 0x16c);
C_ASSERT(offsetof(PlayerFile, shopVisits) == 0x170);
C_ASSERT(offsetof(PlayerFile, shieldHitFlashSpeed) == 0x174);
C_ASSERT(offsetof(PlayerFile, enemyHpBonusRoll) == 0x178);
C_ASSERT(offsetof(PlayerFile, hurryupComboCount) == 0x17c);
C_ASSERT(offsetof(PlayerFile, rows) == 0x180);
C_ASSERT(offsetof(PlayerFile, cols) == 0x184);
C_ASSERT(offsetof(PlayerFile, rocketsFired) == 0x188);
C_ASSERT(offsetof(PlayerFile, memoryGridUpgradeStreak) == 0x18c);
C_ASSERT(offsetof(PlayerFile, memoryBonus) == 0x190);
C_ASSERT(offsetof(PlayerFile, gemPickups) == 0x198);
C_ASSERT(offsetof(PlayerFile, unused19c) == 0x19c);
C_ASSERT(offsetof(PlayerFile, shieldGlowG) == 0x1a0);
C_ASSERT(offsetof(PlayerFile, shieldGlowB) == 0x1a4);
C_ASSERT(offsetof(PlayerFile, unused1a8) == 0x1a8);
C_ASSERT(offsetof(PlayerFile, shieldGlowGStep) == 0x1ac);
C_ASSERT(offsetof(PlayerFile, shieldGlowBStep) == 0x1b0);
C_ASSERT(offsetof(PlayerFile, unusedF1b4_5) == 0x1b4);
C_ASSERT(offsetof(PlayerFile, unusedF1b8_1) == 0x1b8);
C_ASSERT(offsetof(PlayerFile, placeSlot) == 0x1bc);
C_ASSERT(offsetof(PlayerFile, flame) == 0x1c0);
C_ASSERT(offsetof(PlayerFile, shipDestroyedThisLevel) == 0x1c4);
C_ASSERT(offsetof(PlayerFile, trackKillsFlag) == 0x1c8);
C_ASSERT(offsetof(PlayerFile, bonusRoundEnded) == 0x1cc);
C_ASSERT(offsetof(PlayerFile, levelEnemyDataCount) == 0x1d0);
C_ASSERT(offsetof(PlayerFile, unusedF1d4) == 0x1d4);
C_ASSERT(offsetof(PlayerFile, ship) == 0x1d8);
C_ASSERT(offsetof(PlayerFile, unusedF1dc) == 0x1dc);
C_ASSERT(offsetof(PlayerFile, enemySwayX) == 0x1e0);
C_ASSERT(offsetof(PlayerFile, enemySwayY) == 0x1e4);
C_ASSERT(offsetof(PlayerFile, pad_1e8) == 0x1e8);
C_ASSERT(offsetof(PlayerFile, enemySwayVelX) == 0x1f0);
C_ASSERT(offsetof(PlayerFile, enemySwayAccel) == 0x1f4);
C_ASSERT(offsetof(PlayerFile, enemySwayMax) == 0x1f8);
C_ASSERT(offsetof(PlayerFile, enemySwayMin) == 0x1fc);
C_ASSERT(offsetof(PlayerFile, pad_200) == 0x200);
C_ASSERT(offsetof(PlayerFile, unusedF208) == 0x208);
C_ASSERT(offsetof(PlayerFile, unusedF20c) == 0x20c);
C_ASSERT(offsetof(PlayerFile, unusedF210) == 0x210);
C_ASSERT(offsetof(PlayerFile, unusedF214) == 0x214);
C_ASSERT(offsetof(PlayerFile, moneyMax) == 0x218);
C_ASSERT(offsetof(PlayerFile, moneyMaxShown) == 0x21c);
C_ASSERT(offsetof(PlayerFile, unusedF220) == 0x220);
C_ASSERT(offsetof(PlayerFile, unusedF224) == 0x224);
C_ASSERT(offsetof(PlayerFile, primaryEnemyCount) == 0x228);
C_ASSERT(offsetof(PlayerFile, secondaryEnemyCount) == 0x22c);
C_ASSERT(offsetof(PlayerFile, totalEnemies) == 0x230);
C_ASSERT(offsetof(PlayerFile, spawnReserve) == 0x234);
C_ASSERT(offsetof(PlayerFile, killed) == 0x238);
C_ASSERT(offsetof(PlayerFile, escaped) == 0x23c);
C_ASSERT(offsetof(PlayerFile, done) == 0x240);
C_ASSERT(offsetof(PlayerFile, doneTime) == 0x244);
C_ASSERT(offsetof(PlayerFile, chainBonusValue) == 0x248);
C_ASSERT(offsetof(PlayerFile, started) == 0x250);
C_ASSERT(offsetof(PlayerFile, starVelX) == 0x254);
C_ASSERT(offsetof(PlayerFile, starSpeed) == 0x258);
C_ASSERT(offsetof(PlayerFile, starVelZ) == 0x25c);
C_ASSERT(offsetof(PlayerFile, savedRaceStarSpeed) == 0x260);
C_ASSERT(offsetof(PlayerFile, hyperspaceOutTimer) == 0x264);
C_ASSERT(offsetof(PlayerFile, hyperspaceMidTimer) == 0x268);
C_ASSERT(offsetof(PlayerFile, hyperspaceInTimer) == 0x26c);
C_ASSERT(offsetof(PlayerFile, hyperspaceInDuration) == 0x270);
C_ASSERT(offsetof(PlayerFile, hyperspaceFade) == 0x274);
C_ASSERT(offsetof(PlayerFile, savedHyperspaceFade) == 0x278);
C_ASSERT(offsetof(PlayerFile, savedHyperspaceInDuration) == 0x27c);
C_ASSERT(offsetof(PlayerFile, savedHyperspaceInTimer) == 0x280);
C_ASSERT(offsetof(PlayerFile, savedHyperspaceOutTimer) == 0x284);
C_ASSERT(offsetof(PlayerFile, savedStarSpeed) == 0x288);
C_ASSERT(offsetof(PlayerFile, savedStarVelZ) == 0x28c);
C_ASSERT(offsetof(PlayerFile, savedScrollSpeedY) == 0x290);
C_ASSERT(offsetof(PlayerFile, savedHyperspaceMidTimer) == 0x294);
C_ASSERT(offsetof(PlayerFile, scrollSpeedY) == 0x298);
C_ASSERT(offsetof(PlayerFile, levelFinished) == 0x29c);
C_ASSERT(offsetof(PlayerFile, levelTransitioning) == 0x2a0);
C_ASSERT(offsetof(PlayerFile, effectDuration) == 0x2a4);
C_ASSERT(offsetof(PlayerFile, levelWarpPending) == 0x2a8);
C_ASSERT(offsetof(PlayerFile, keyLatchFire) == 0x2ac);
C_ASSERT(offsetof(PlayerFile, keyLatchRocket) == 0x2b0);
C_ASSERT(offsetof(PlayerFile, keyLatchPause) == 0x2b4);
C_ASSERT(offsetof(PlayerFile, keyLatchProfile) == 0x2b8);
C_ASSERT(offsetof(PlayerFile, bonusTally) == 0x2bc);
C_ASSERT(offsetof(PlayerFile, bonusTallyTick) == 0x2c0);
C_ASSERT(offsetof(PlayerFile, bonusTallyDelay) == 0x2c4);
C_ASSERT(offsetof(PlayerFile, perfectTextSpacing) == 0x2c8);
C_ASSERT(offsetof(PlayerFile, perfectTextSpacingVel) == 0x2cc);
C_ASSERT(offsetof(PlayerFile, perfectDone) == 0x2d0);
C_ASSERT(offsetof(PlayerFile, bonusResultsInitDone) == 0x2d4);
C_ASSERT(offsetof(PlayerFile, bonusKilled) == 0x2d8);
C_ASSERT(offsetof(PlayerFile, gemCounterPicks) == 0x2dc);
C_ASSERT(offsetof(PlayerFile, gemCounterUnlocked) == 0x2e0);
C_ASSERT(offsetof(PlayerFile, highScoreMilestone) == 0x2e4);
C_ASSERT(offsetof(PlayerFile, blueMoneyPicks) == 0x2e8);
C_ASSERT(offsetof(PlayerFile, blueMoneyUnlocked) == 0x2ec);
C_ASSERT(offsetof(PlayerFile, multiplierPicks) == 0x2f0);
C_ASSERT(offsetof(PlayerFile, multiplierUnlocked) == 0x2f4);
C_ASSERT(offsetof(PlayerFile, weaponFloorAtOne) == 0x2f8);
C_ASSERT(offsetof(PlayerFile, secretFound01) == 0x2fc);
C_ASSERT(offsetof(PlayerFile, secretFound02) == 0x300);
C_ASSERT(offsetof(PlayerFile, secretFound03) == 0x304);
C_ASSERT(offsetof(PlayerFile, secretFound04) == 0x308);
C_ASSERT(offsetof(PlayerFile, secretFound05) == 0x30c);
C_ASSERT(offsetof(PlayerFile, secretFound06) == 0x310);
C_ASSERT(offsetof(PlayerFile, secretFound07) == 0x314);
C_ASSERT(offsetof(PlayerFile, secretFound08) == 0x318);
C_ASSERT(offsetof(PlayerFile, secretFound09) == 0x31c);
C_ASSERT(offsetof(PlayerFile, secretFound10) == 0x320);
C_ASSERT(offsetof(PlayerFile, secretFound11) == 0x324);
C_ASSERT(offsetof(PlayerFile, secretFound12) == 0x328);
C_ASSERT(offsetof(PlayerFile, secretFound13) == 0x32c);
C_ASSERT(offsetof(PlayerFile, secretFound14) == 0x330);
C_ASSERT(offsetof(PlayerFile, secretFound15) == 0x334);
C_ASSERT(offsetof(PlayerFile, secretFound16) == 0x338);
C_ASSERT(offsetof(PlayerFile, secretFound17) == 0x33c);
C_ASSERT(offsetof(PlayerFile, secretFound18) == 0x340);
C_ASSERT(offsetof(PlayerFile, secretFound19) == 0x344);
C_ASSERT(offsetof(PlayerFile, secretFound20) == 0x348);
C_ASSERT(offsetof(PlayerFile, secretFound21) == 0x34c);
C_ASSERT(offsetof(PlayerFile, secretFound22) == 0x350);
C_ASSERT(offsetof(PlayerFile, secretFound23) == 0x354);
C_ASSERT(offsetof(PlayerFile, secretFound24) == 0x358);
C_ASSERT(offsetof(PlayerFile, secretFound25) == 0x35c);
C_ASSERT(offsetof(PlayerFile, secretFound26) == 0x360);
C_ASSERT(offsetof(PlayerFile, secretFound27) == 0x364);
C_ASSERT(offsetof(PlayerFile, secretFound28) == 0x368);
C_ASSERT(offsetof(PlayerFile, secretFound29) == 0x36c);
C_ASSERT(offsetof(PlayerFile, secretFound30) == 0x370);
C_ASSERT(offsetof(PlayerFile, secretFound31to50) == 0x374);
C_ASSERT(offsetof(PlayerFile, secretSeen) == 0x3c4);
C_ASSERT(offsetof(PlayerFile, secretFlags) == 0x2fc);
C_ASSERT(offsetof(PlayerFile, secretCount) == 0x48c);
C_ASSERT(offsetof(PlayerFile, bonusRoundPoints) == 0x490);
C_ASSERT(offsetof(PlayerFile, perfectStreak) == 0x498);
C_ASSERT(offsetof(PlayerFile, drunkStreak) == 0x49c);
C_ASSERT(offsetof(PlayerFile, rockets) == 0x4a0);
C_ASSERT(offsetof(PlayerFile, alienLock) == 0x4a4);
C_ASSERT(offsetof(PlayerFile, pad_4a5) == 0x4a5);
C_ASSERT(offsetof(PlayerFile, tries) == 0x4a8);
C_ASSERT(offsetof(PlayerFile, levelMilestoneHandled) == 0x4ac);
C_ASSERT(offsetof(PlayerFile, pad_4ad) == 0x4ad);
C_ASSERT(offsetof(PlayerFile, secretBirdCounter) == 0x4b0);
C_ASSERT(offsetof(PlayerFile, secretBirdTick) == 0x4b4);
C_ASSERT(offsetof(PlayerFile, raceDistance) == 0x4b8);
C_ASSERT(offsetof(PlayerFile, gameSpeedSetting) == 0x4bc);
C_ASSERT(offsetof(PlayerFile, maxRankReached) == 0x4c0);
C_ASSERT(offsetof(PlayerFile, pad_4c1) == 0x4c1);
C_ASSERT(offsetof(PlayerFile, bulkLevelsCooldown) == 0x4c4);
C_ASSERT(offsetof(PlayerFile, nextShotSnd) == 0x4c8);
C_ASSERT(offsetof(PlayerFile, pad_4cc) == 0x4cc);
C_ASSERT(offsetof(PlayerFile, sessionPlayTime) == 0x4d0);

struct EnemyFile {   // 0x3a8 bytes
    int32_t unused00; // +0x0
    int32_t active; // +0x4
    float x; // +0x8
    float y; // +0xc
    float homeX; // +0x10
    float homeY; // +0x14
    float velX; // +0x18
    float velY; // +0x1c
    float accelX; // +0x20
    float accelY; // +0x24
    float speedScale; // +0x28
    float patternTimer; // +0x2c
    float hoverX; // +0x30
    float hoverY; // +0x34
    float speedX; // +0x38
    float speedY; // +0x3c
    float offsetX; // +0x40
    float offsetY; // +0x44
    int32_t oscillateMul; // +0x48
    float oscillateAccel; // +0x4c
    float oscillatePhase; // +0x50
    int32_t ownerPlayer; // +0x54
    int32_t beamOffsetX; // +0x58
    int32_t beamSide; // +0x5c
    int32_t homing; // +0x60
    int32_t settled; // +0x64
    int32_t altFireActive; // +0x68
    int32_t pairedEnemyIdx; // +0x6c
    char pad_70[0x4]; // +0x70 (dead)
    int32_t savedFireDelay; // +0x74
    float savedHp; // +0x78
    float unusedF7c; // +0x7c
    float unusedF80; // +0x80
    float debrisVelX; // +0x84
    float debrisVelY; // +0x88
    float attackStaggerTimer; // +0x8c
    float turnTimer; // +0x90
    char pad_94[0x8]; // +0x94 (dead)
    float dirStepTimer; // +0x9c
    char pad_a0[0xc]; // +0xa0 (dead)
    int32_t type; // +0xac
    int32_t patternId; // +0xb0
    char pad_b4[0x8]; // +0xb4 (dead)
    int32_t hazardType; // +0xbc
    int32_t srcX; // +0xc0
    int32_t srcY; // +0xc4
    int32_t facing; // +0xc8
    int32_t turnState; // +0xcc
    int32_t zigzagTimer; // +0xd0
    int32_t unusedAlpha; // +0xd4
    float zigzagVelX; // +0xd8
    float zigzagAccel; // +0xdc
    float descendVelY; // +0xe0
    float descendAccelY; // +0xe4
    float descendVelX; // +0xe8
    float descendAccelX; // +0xec
    float turnTimer2; // +0xf0
    float turnTimer2Max; // +0xf4
    float animFrame; // +0xf8
    float animDelay; // +0xfc
    float animTimer; // +0x100
    int32_t animReverse; // +0x104
    int32_t useDirRemap; // +0x108
    int32_t altFrameCounter; // +0x10c
    char pad_110[0x24]; // +0x110 (dead)
    int32_t dead_gfxA; // +0x134 (dead: was KGraphic *)
    int32_t dead_gfxB; // +0x138 (dead: was KGraphic *)
    int32_t dead_hitFlashGfxA; // +0x13c (dead: was KGraphic *)
    int32_t dead_hitFlashGfxB; // +0x140 (dead: was KGraphic *)
    int32_t dead_shotFrame; // +0x144 (dead: was void *)
    int32_t shotGfxW; // +0x148
    int32_t shotGfxH; // +0x14c
    Rect16File srcRect; // +0x150
    int32_t hitFlashTimer; // +0x160
    float hp; // +0x164
    float maxHp; // +0x168
    char pad_16c[0x4]; // +0x16c (dead)
    int64_t score; // +0x170
    char pad_178[0x4]; // +0x178 (dead)
    int32_t attackDelay; // +0x17c
    int32_t attackDelayStep; // +0x180
    int32_t fireDelay; // +0x184
    int32_t fireDelayStep; // +0x188
    char pad_18c[0x4]; // +0x18c (dead)
    int32_t patternEndAction; // +0x190
    char pad_194[0x10]; // +0x194 (dead)
    FrameSetFile frameSet; // +0x1a4
    union {
        struct {
            int32_t unused1c4; // +0x1c4
            int32_t unused1c8; // +0x1c8
            int32_t unused1cc; // +0x1cc
            int32_t unused1d0; // +0x1d0
        };
        struct {
            Rect16File hitRect; // +0x1c4
        };
    };
    char pad_1d4[0x28]; // +0x1d4 (dead)
    int32_t groupIndex; // +0x1fc
    int32_t bonusGroupIndex; // +0x200
    int32_t patternStep; // +0x204
    float animSpeedDivisor; // +0x208
    int32_t unusedSpawnDelay; // +0x20c
    float animStepTime; // +0x210
    int32_t animFrameCount; // +0x214
    int32_t animPingPong; // +0x218
    int32_t forcedDir; // +0x21c
    int32_t bossGunASlot[10]; // +0x220
    int32_t bossGunBSlot[10]; // +0x248
    int32_t bossGunCSlot[10]; // +0x270
    float bossGunAX[10]; // +0x298
    float bossGunAY[10]; // +0x2c0
    float bossGunBX[10]; // +0x2e8
    float bossGunBY[10]; // +0x310
    float bossGunCX[10]; // +0x338
    float bossGunCY[10]; // +0x360
    int32_t deathExplosionGfx; // +0x388
    int32_t deathExplosionLife; // +0x38c
    int32_t deathExplosionR; // +0x390
    int32_t deathExplosionG; // +0x394
    int32_t deathExplosionB; // +0x398
    uint8_t locked; // +0x39c
    uint8_t fixedFireDelay; // +0x39d
    uint8_t flashActive; // +0x39e
    char pad_39f[0x1]; // +0x39f (dead)
    float flashTimer; // +0x3a0
    char pad_3a4[0x4]; // +0x3a4 (dead)
};
C_ASSERT(sizeof(EnemyFile) == 0x3a8);
C_ASSERT(offsetof(EnemyFile, unused00) == 0x0);
C_ASSERT(offsetof(EnemyFile, active) == 0x4);
C_ASSERT(offsetof(EnemyFile, x) == 0x8);
C_ASSERT(offsetof(EnemyFile, y) == 0xc);
C_ASSERT(offsetof(EnemyFile, homeX) == 0x10);
C_ASSERT(offsetof(EnemyFile, homeY) == 0x14);
C_ASSERT(offsetof(EnemyFile, velX) == 0x18);
C_ASSERT(offsetof(EnemyFile, velY) == 0x1c);
C_ASSERT(offsetof(EnemyFile, accelX) == 0x20);
C_ASSERT(offsetof(EnemyFile, accelY) == 0x24);
C_ASSERT(offsetof(EnemyFile, speedScale) == 0x28);
C_ASSERT(offsetof(EnemyFile, patternTimer) == 0x2c);
C_ASSERT(offsetof(EnemyFile, hoverX) == 0x30);
C_ASSERT(offsetof(EnemyFile, hoverY) == 0x34);
C_ASSERT(offsetof(EnemyFile, speedX) == 0x38);
C_ASSERT(offsetof(EnemyFile, speedY) == 0x3c);
C_ASSERT(offsetof(EnemyFile, offsetX) == 0x40);
C_ASSERT(offsetof(EnemyFile, offsetY) == 0x44);
C_ASSERT(offsetof(EnemyFile, oscillateMul) == 0x48);
C_ASSERT(offsetof(EnemyFile, oscillateAccel) == 0x4c);
C_ASSERT(offsetof(EnemyFile, oscillatePhase) == 0x50);
C_ASSERT(offsetof(EnemyFile, ownerPlayer) == 0x54);
C_ASSERT(offsetof(EnemyFile, beamOffsetX) == 0x58);
C_ASSERT(offsetof(EnemyFile, beamSide) == 0x5c);
C_ASSERT(offsetof(EnemyFile, homing) == 0x60);
C_ASSERT(offsetof(EnemyFile, settled) == 0x64);
C_ASSERT(offsetof(EnemyFile, altFireActive) == 0x68);
C_ASSERT(offsetof(EnemyFile, pairedEnemyIdx) == 0x6c);
C_ASSERT(offsetof(EnemyFile, pad_70) == 0x70);
C_ASSERT(offsetof(EnemyFile, savedFireDelay) == 0x74);
C_ASSERT(offsetof(EnemyFile, savedHp) == 0x78);
C_ASSERT(offsetof(EnemyFile, unusedF7c) == 0x7c);
C_ASSERT(offsetof(EnemyFile, unusedF80) == 0x80);
C_ASSERT(offsetof(EnemyFile, debrisVelX) == 0x84);
C_ASSERT(offsetof(EnemyFile, debrisVelY) == 0x88);
C_ASSERT(offsetof(EnemyFile, attackStaggerTimer) == 0x8c);
C_ASSERT(offsetof(EnemyFile, turnTimer) == 0x90);
C_ASSERT(offsetof(EnemyFile, pad_94) == 0x94);
C_ASSERT(offsetof(EnemyFile, dirStepTimer) == 0x9c);
C_ASSERT(offsetof(EnemyFile, pad_a0) == 0xa0);
C_ASSERT(offsetof(EnemyFile, type) == 0xac);
C_ASSERT(offsetof(EnemyFile, patternId) == 0xb0);
C_ASSERT(offsetof(EnemyFile, pad_b4) == 0xb4);
C_ASSERT(offsetof(EnemyFile, hazardType) == 0xbc);
C_ASSERT(offsetof(EnemyFile, srcX) == 0xc0);
C_ASSERT(offsetof(EnemyFile, srcY) == 0xc4);
C_ASSERT(offsetof(EnemyFile, facing) == 0xc8);
C_ASSERT(offsetof(EnemyFile, turnState) == 0xcc);
C_ASSERT(offsetof(EnemyFile, zigzagTimer) == 0xd0);
C_ASSERT(offsetof(EnemyFile, unusedAlpha) == 0xd4);
C_ASSERT(offsetof(EnemyFile, zigzagVelX) == 0xd8);
C_ASSERT(offsetof(EnemyFile, zigzagAccel) == 0xdc);
C_ASSERT(offsetof(EnemyFile, descendVelY) == 0xe0);
C_ASSERT(offsetof(EnemyFile, descendAccelY) == 0xe4);
C_ASSERT(offsetof(EnemyFile, descendVelX) == 0xe8);
C_ASSERT(offsetof(EnemyFile, descendAccelX) == 0xec);
C_ASSERT(offsetof(EnemyFile, turnTimer2) == 0xf0);
C_ASSERT(offsetof(EnemyFile, turnTimer2Max) == 0xf4);
C_ASSERT(offsetof(EnemyFile, animFrame) == 0xf8);
C_ASSERT(offsetof(EnemyFile, animDelay) == 0xfc);
C_ASSERT(offsetof(EnemyFile, animTimer) == 0x100);
C_ASSERT(offsetof(EnemyFile, animReverse) == 0x104);
C_ASSERT(offsetof(EnemyFile, useDirRemap) == 0x108);
C_ASSERT(offsetof(EnemyFile, altFrameCounter) == 0x10c);
C_ASSERT(offsetof(EnemyFile, pad_110) == 0x110);
C_ASSERT(offsetof(EnemyFile, dead_gfxA) == 0x134);
C_ASSERT(offsetof(EnemyFile, dead_gfxB) == 0x138);
C_ASSERT(offsetof(EnemyFile, dead_hitFlashGfxA) == 0x13c);
C_ASSERT(offsetof(EnemyFile, dead_hitFlashGfxB) == 0x140);
C_ASSERT(offsetof(EnemyFile, dead_shotFrame) == 0x144);
C_ASSERT(offsetof(EnemyFile, shotGfxW) == 0x148);
C_ASSERT(offsetof(EnemyFile, shotGfxH) == 0x14c);
C_ASSERT(offsetof(EnemyFile, srcRect) == 0x150);
C_ASSERT(offsetof(EnemyFile, hitFlashTimer) == 0x160);
C_ASSERT(offsetof(EnemyFile, hp) == 0x164);
C_ASSERT(offsetof(EnemyFile, maxHp) == 0x168);
C_ASSERT(offsetof(EnemyFile, pad_16c) == 0x16c);
C_ASSERT(offsetof(EnemyFile, score) == 0x170);
C_ASSERT(offsetof(EnemyFile, pad_178) == 0x178);
C_ASSERT(offsetof(EnemyFile, attackDelay) == 0x17c);
C_ASSERT(offsetof(EnemyFile, attackDelayStep) == 0x180);
C_ASSERT(offsetof(EnemyFile, fireDelay) == 0x184);
C_ASSERT(offsetof(EnemyFile, fireDelayStep) == 0x188);
C_ASSERT(offsetof(EnemyFile, pad_18c) == 0x18c);
C_ASSERT(offsetof(EnemyFile, patternEndAction) == 0x190);
C_ASSERT(offsetof(EnemyFile, pad_194) == 0x194);
C_ASSERT(offsetof(EnemyFile, frameSet) == 0x1a4);
C_ASSERT(offsetof(EnemyFile, unused1c4) == 0x1c4);
C_ASSERT(offsetof(EnemyFile, unused1c8) == 0x1c8);
C_ASSERT(offsetof(EnemyFile, unused1cc) == 0x1cc);
C_ASSERT(offsetof(EnemyFile, unused1d0) == 0x1d0);
C_ASSERT(offsetof(EnemyFile, hitRect) == 0x1c4);
C_ASSERT(offsetof(EnemyFile, pad_1d4) == 0x1d4);
C_ASSERT(offsetof(EnemyFile, groupIndex) == 0x1fc);
C_ASSERT(offsetof(EnemyFile, bonusGroupIndex) == 0x200);
C_ASSERT(offsetof(EnemyFile, patternStep) == 0x204);
C_ASSERT(offsetof(EnemyFile, animSpeedDivisor) == 0x208);
C_ASSERT(offsetof(EnemyFile, unusedSpawnDelay) == 0x20c);
C_ASSERT(offsetof(EnemyFile, animStepTime) == 0x210);
C_ASSERT(offsetof(EnemyFile, animFrameCount) == 0x214);
C_ASSERT(offsetof(EnemyFile, animPingPong) == 0x218);
C_ASSERT(offsetof(EnemyFile, forcedDir) == 0x21c);
C_ASSERT(offsetof(EnemyFile, bossGunASlot) == 0x220);
C_ASSERT(offsetof(EnemyFile, bossGunBSlot) == 0x248);
C_ASSERT(offsetof(EnemyFile, bossGunCSlot) == 0x270);
C_ASSERT(offsetof(EnemyFile, bossGunAX) == 0x298);
C_ASSERT(offsetof(EnemyFile, bossGunAY) == 0x2c0);
C_ASSERT(offsetof(EnemyFile, bossGunBX) == 0x2e8);
C_ASSERT(offsetof(EnemyFile, bossGunBY) == 0x310);
C_ASSERT(offsetof(EnemyFile, bossGunCX) == 0x338);
C_ASSERT(offsetof(EnemyFile, bossGunCY) == 0x360);
C_ASSERT(offsetof(EnemyFile, deathExplosionGfx) == 0x388);
C_ASSERT(offsetof(EnemyFile, deathExplosionLife) == 0x38c);
C_ASSERT(offsetof(EnemyFile, deathExplosionR) == 0x390);
C_ASSERT(offsetof(EnemyFile, deathExplosionG) == 0x394);
C_ASSERT(offsetof(EnemyFile, deathExplosionB) == 0x398);
C_ASSERT(offsetof(EnemyFile, locked) == 0x39c);
C_ASSERT(offsetof(EnemyFile, fixedFireDelay) == 0x39d);
C_ASSERT(offsetof(EnemyFile, flashActive) == 0x39e);
C_ASSERT(offsetof(EnemyFile, pad_39f) == 0x39f);
C_ASSERT(offsetof(EnemyFile, flashTimer) == 0x3a0);
C_ASSERT(offsetof(EnemyFile, pad_3a4) == 0x3a4);

struct SaveDataFile {   // 0x1370 bytes
    char sig[8]; // +0x0
    int64_t saveId; // +0x8
    PlayerFile players[4]; // +0x10
};
C_ASSERT(sizeof(SaveDataFile) == 0x1370);
C_ASSERT(offsetof(SaveDataFile, sig) == 0x0);
C_ASSERT(offsetof(SaveDataFile, saveId) == 0x8);
C_ASSERT(offsetof(SaveDataFile, players) == 0x10);

struct SaveFile {   // 0xa9a18 bytes: the original's memory from 0x8486d8 (g_save) to 0x8f20f0
    SaveDataFile save; // +0x0 g_save
    EnemyFile enemies[2][150]; // +0x1370 g_enemies
    uint8_t dead_pad_88e328[280800]; // +0x45c50 g_pad_88e328 (dead)
    LevelRecFile levelRecs[4000]; // +0x8a530 g_levelRecs
    int32_t curLevelNum; // +0xa9930 g_curLevelNum
    int32_t levelDataLoaded; // +0xa9934 g_levelDataLoaded
    int32_t loadedLevel; // +0xa9938 g_loadedLevel
    int32_t warpLevelR; // +0xa993c g_warpLevelR
    int32_t warpLevelL; // +0xa9940 g_warpLevelL
    uint8_t marksBonusGiven; // +0xa9944 g_marksBonusGiven
    uint8_t enemyAimAtPlayer; // +0xa9945 g_enemyAimAtPlayer
    uint8_t fastEnemyBullets; // +0xa9946 g_fastEnemyBullets
    uint8_t dead_pad_8f201f; // +0xa9947 g_pad_8f201f (dead)
    int32_t diffEnemyFireChance; // +0xa9948 g_diffEnemyFireChance
    int32_t diffShotFuseBase; // +0xa994c g_diffShotFuseBase
    int32_t diffShotFuseRange; // +0xa9950 g_diffShotFuseRange
    int32_t hurryUpInterval; // +0xa9954 g_hurryUpInterval
    float diffShotSpeedMin; // +0xa9958 g_diffShotSpeedMin
    float diffShotSpeedMax; // +0xa995c g_diffShotSpeedMax
    float diffTurretTrackChance; // +0xa9960 g_diffTurretTrackChance
    float diffBonusDropRoll; // +0xa9964 g_diffBonusDropRoll
    int32_t curPlayer; // +0xa9968 g_curPlayer
    int32_t shopCurPlayer; // +0xa996c g_shopCurPlayer
    int32_t promoPlayer; // +0xa9970 g_promoPlayer
    int32_t bonusStagePlayer; // +0xa9974 g_bonusStagePlayer
    int32_t vsTurnPlayer; // +0xa9978 g_vsTurnPlayer
    int32_t saveVersion; // +0xa997c g_saveVersion
    int32_t diffScoreBonus; // +0xa9980 g_diffScoreBonus
    float defaultObjAlpha; // +0xa9984 g_defaultObjAlpha
    float enemyBulletSpeed; // +0xa9988 g_enemyBulletSpeed
    float playerStartSpeed; // +0xa998c g_playerStartSpeed
    float diffEnemyTimerMul; // +0xa9990 g_diffEnemyTimerMul
    int32_t diffEnemyHpBonus; // +0xa9994 g_diffEnemyHpBonus
    int32_t moneySuckerBaseHp; // +0xa9998 g_moneySuckerBaseHp
    float diffHurryUpSpeedMax; // +0xa999c g_diffHurryUpSpeedMax
    int32_t eliteHpBonus; // +0xa99a0 g_eliteHpBonus
    int32_t hurryUpHpBonus; // +0xa99a4 g_hurryUpHpBonus
    float speedBase; // +0xa99a8 g_speedBase
    float speedStep; // +0xa99ac g_speedStep
    float maxSpeedMul; // +0xa99b0 g_maxSpeedMul
    float speedMax; // +0xa99b4 g_speedMax
    int32_t bonusDuration; // +0xa99b8 g_bonusDuration
    float bonusSpawnRampRate; // +0xa99bc g_bonusSpawnRampRate
    int32_t bonusThresholdBase; // +0xa99c0 g_bonusThresholdBase
    int32_t bonusRareChance; // +0xa99c4 g_bonusRareChance
    int32_t timeMax; // +0xa99c8 g_timeMax
    int32_t speedMin; // +0xa99cc g_speedMin
    int32_t lastEliteSpawnLevel; // +0xa99d0 g_lastEliteSpawnLevel
    int32_t fireDelayMin; // +0xa99d4 g_fireDelayMin
    int32_t enemyFireRateMin; // +0xa99d8 g_enemyFireRateMin
    int32_t fireDelayBiasA; // +0xa99dc g_fireDelayBiasA
    int32_t fireDelayBiasB; // +0xa99e0 g_fireDelayBiasB
    uint8_t dead_pad_8f20bc[4]; // +0xa99e4 g_pad_8f20bc (dead)
    int64_t resumeTimeOffset; // +0xa99e8 g_resumeTimeOffset
    int32_t comboLevel; // +0xa99f0 g_comboLevel
    int32_t comboStep; // +0xa99f4 g_comboStep
    int32_t sessionScore; // +0xa99f8 g_sessionScore
    int32_t hits; // +0xa99fc g_hits
    int32_t gameMode; // +0xa9a00 g_gameMode
    int32_t pendingGameMode; // +0xa9a04 g_pendingGameMode
    int32_t hofMode; // +0xa9a08 g_hofMode
    int32_t savedDifficultySave; // +0xa9a0c g_savedDifficultySave
    int32_t perfectCount; // +0xa9a10 g_perfectCount
    int32_t killCount; // +0xa9a14 g_killCount
};
C_ASSERT(sizeof(SaveFile) == 0xa9a18);
C_ASSERT(offsetof(SaveFile, save) == 0x0);
C_ASSERT(offsetof(SaveFile, enemies) == 0x1370);
C_ASSERT(offsetof(SaveFile, dead_pad_88e328) == 0x45c50);
C_ASSERT(offsetof(SaveFile, levelRecs) == 0x8a530);
C_ASSERT(offsetof(SaveFile, curLevelNum) == 0xa9930);
C_ASSERT(offsetof(SaveFile, levelDataLoaded) == 0xa9934);
C_ASSERT(offsetof(SaveFile, loadedLevel) == 0xa9938);
C_ASSERT(offsetof(SaveFile, warpLevelR) == 0xa993c);
C_ASSERT(offsetof(SaveFile, warpLevelL) == 0xa9940);
C_ASSERT(offsetof(SaveFile, marksBonusGiven) == 0xa9944);
C_ASSERT(offsetof(SaveFile, enemyAimAtPlayer) == 0xa9945);
C_ASSERT(offsetof(SaveFile, fastEnemyBullets) == 0xa9946);
C_ASSERT(offsetof(SaveFile, dead_pad_8f201f) == 0xa9947);
C_ASSERT(offsetof(SaveFile, diffEnemyFireChance) == 0xa9948);
C_ASSERT(offsetof(SaveFile, diffShotFuseBase) == 0xa994c);
C_ASSERT(offsetof(SaveFile, diffShotFuseRange) == 0xa9950);
C_ASSERT(offsetof(SaveFile, hurryUpInterval) == 0xa9954);
C_ASSERT(offsetof(SaveFile, diffShotSpeedMin) == 0xa9958);
C_ASSERT(offsetof(SaveFile, diffShotSpeedMax) == 0xa995c);
C_ASSERT(offsetof(SaveFile, diffTurretTrackChance) == 0xa9960);
C_ASSERT(offsetof(SaveFile, diffBonusDropRoll) == 0xa9964);
C_ASSERT(offsetof(SaveFile, curPlayer) == 0xa9968);
C_ASSERT(offsetof(SaveFile, shopCurPlayer) == 0xa996c);
C_ASSERT(offsetof(SaveFile, promoPlayer) == 0xa9970);
C_ASSERT(offsetof(SaveFile, bonusStagePlayer) == 0xa9974);
C_ASSERT(offsetof(SaveFile, vsTurnPlayer) == 0xa9978);
C_ASSERT(offsetof(SaveFile, saveVersion) == 0xa997c);
C_ASSERT(offsetof(SaveFile, diffScoreBonus) == 0xa9980);
C_ASSERT(offsetof(SaveFile, defaultObjAlpha) == 0xa9984);
C_ASSERT(offsetof(SaveFile, enemyBulletSpeed) == 0xa9988);
C_ASSERT(offsetof(SaveFile, playerStartSpeed) == 0xa998c);
C_ASSERT(offsetof(SaveFile, diffEnemyTimerMul) == 0xa9990);
C_ASSERT(offsetof(SaveFile, diffEnemyHpBonus) == 0xa9994);
C_ASSERT(offsetof(SaveFile, moneySuckerBaseHp) == 0xa9998);
C_ASSERT(offsetof(SaveFile, diffHurryUpSpeedMax) == 0xa999c);
C_ASSERT(offsetof(SaveFile, eliteHpBonus) == 0xa99a0);
C_ASSERT(offsetof(SaveFile, hurryUpHpBonus) == 0xa99a4);
C_ASSERT(offsetof(SaveFile, speedBase) == 0xa99a8);
C_ASSERT(offsetof(SaveFile, speedStep) == 0xa99ac);
C_ASSERT(offsetof(SaveFile, maxSpeedMul) == 0xa99b0);
C_ASSERT(offsetof(SaveFile, speedMax) == 0xa99b4);
C_ASSERT(offsetof(SaveFile, bonusDuration) == 0xa99b8);
C_ASSERT(offsetof(SaveFile, bonusSpawnRampRate) == 0xa99bc);
C_ASSERT(offsetof(SaveFile, bonusThresholdBase) == 0xa99c0);
C_ASSERT(offsetof(SaveFile, bonusRareChance) == 0xa99c4);
C_ASSERT(offsetof(SaveFile, timeMax) == 0xa99c8);
C_ASSERT(offsetof(SaveFile, speedMin) == 0xa99cc);
C_ASSERT(offsetof(SaveFile, lastEliteSpawnLevel) == 0xa99d0);
C_ASSERT(offsetof(SaveFile, fireDelayMin) == 0xa99d4);
C_ASSERT(offsetof(SaveFile, enemyFireRateMin) == 0xa99d8);
C_ASSERT(offsetof(SaveFile, fireDelayBiasA) == 0xa99dc);
C_ASSERT(offsetof(SaveFile, fireDelayBiasB) == 0xa99e0);
C_ASSERT(offsetof(SaveFile, dead_pad_8f20bc) == 0xa99e4);
C_ASSERT(offsetof(SaveFile, resumeTimeOffset) == 0xa99e8);
C_ASSERT(offsetof(SaveFile, comboLevel) == 0xa99f0);
C_ASSERT(offsetof(SaveFile, comboStep) == 0xa99f4);
C_ASSERT(offsetof(SaveFile, sessionScore) == 0xa99f8);
C_ASSERT(offsetof(SaveFile, hits) == 0xa99fc);
C_ASSERT(offsetof(SaveFile, gameMode) == 0xa9a00);
C_ASSERT(offsetof(SaveFile, pendingGameMode) == 0xa9a04);
C_ASSERT(offsetof(SaveFile, hofMode) == 0xa9a08);
C_ASSERT(offsetof(SaveFile, savedDifficultySave) == 0xa9a0c);
C_ASSERT(offsetof(SaveFile, perfectCount) == 0xa9a10);
C_ASSERT(offsetof(SaveFile, killCount) == 0xa9a14);

static void Rect16ToFile(Rect16File *f, const Rect16 *g)
{
    FIELD(f->v, g->v);
}

static void Rect16FromFile(Rect16 *g, const Rect16File *f)
{
    FIELD(g->v, f->v);
}

static void FrameSetToFile(FrameSetFile *f, const FrameSet *g)
{
    FIELD(f->raw, g->raw);
}

static void FrameSetFromFile(FrameSet *g, const FrameSetFile *f)
{
    FIELD(g->raw, f->raw);
}

static void LevelRecToFile(LevelRecFile *f, const LevelRec *g)
{
    FIELD(f->data, g->data);
}

static void LevelRecFromFile(LevelRec *g, const LevelRecFile *f)
{
    FIELD(g->data, f->data);
}

static void PlayerToFile(PlayerFile *f, const Player *g)
{
    FIELD(f->inputDevice, g->inputDevice);
    FIELD(f->x, g->x);
    FIELD(f->y, g->y);
    FIELD(f->mirrorX, g->mirrorX);
    FIELD(f->extraLetterE, g->extraLetterE);
    FIELD(f->speed, g->speed);
    FIELD(f->bank, g->bank);
    FIELD(f->writeOnlyF1c, g->writeOnlyF1c);
    FIELD(f->hitMaskParamA, g->hitMaskParamA);
    FIELD(f->hitMaskParamB, g->hitMaskParamB);
    Rect16ToFile(&f->box, &g->box);
    FIELD(f->shots, g->shots);
    FIELD(f->hits, g->hits);
    FIELD(f->lives, g->lives);
    FIELD(f->deaths, g->deaths);
    FIELD(f->collisionsTaken, g->collisionsTaken);
    FIELD(f->extraLetterR, g->extraLetterR);
    FIELD(f->score, g->score);
    FIELD(f->autofire, g->autofire);
    FIELD(f->autofireUnlocked, g->autofireUnlocked);
    FIELD(f->turretTrackingReduction, g->turretTrackingReduction);
    FIELD(f->bonusRoundScore, g->bonusRoundScore);
    FIELD(f->gemCounterCollected, g->gemCounterCollected);
    FIELD(f->bonusHighScore, g->bonusHighScore);
    FIELD(f->superAuto, g->superAuto);
    FIELD(f->money, g->money);
    FIELD(f->rank, g->rank);
    FIELD(f->bestRank, g->bestRank);
    FIELD(f->bonusRoundCount, g->bonusRoundCount);
    FIELD(f->gems, g->gems);
    FIELD(f->bombPickups, g->bombPickups);
    FIELD(f->marks, g->marks);
    FIELD(f->extraLetterX, g->extraLetterX);
    FIELD(f->color, g->color);
    FIELD(f->secretBirdHits, g->secretBirdHits);
    FIELD(f->level, g->level);
    FIELD(f->gemSeqB, g->gemSeqB);
    FIELD(f->gemSeqA, g->gemSeqA);
    FIELD(f->pickupCount, g->pickupCount);
    FIELD(f->extraProgress, g->extraProgress);
    FIELD(f->artxeProgress, g->artxeProgress);
    FIELD(f->displayLevel, g->displayLevel);
    FIELD(f->unusedScoreThreshold, g->unusedScoreThreshold);
    FIELD(f->extraLetterA, g->extraLetterA);
    FIELD(f->msMultiplierActive, g->msMultiplierActive);
    FIELD(f->writeOnlyMirrorFlag, g->writeOnlyMirrorFlag);
    FIELD(f->armour, g->armour);
    FIELD(f->freezeTimer, g->freezeTimer);
    FIELD(f->extraLetterT, g->extraLetterT);
    FIELD(f->scoreMult2Timer, g->scoreMult2Timer);
    FIELD(f->scoreMult5Timer, g->scoreMult5Timer);
    FIELD(f->mirrorTime, g->mirrorTime);
    FIELD(f->drunkModeTimer, g->drunkModeTimer);
    FIELD(f->scoopTimer, g->scoopTimer);
    FIELD(f->shieldTimer, g->shieldTimer);
    FIELD(f->blueMoneyActive, g->blueMoneyActive);
    FIELD(f->unusedInvulnBlinkFrame, g->unusedInvulnBlinkFrame);
    FIELD(f->unused12c, g->unused12c);
    FIELD(f->unusedInvulnBlinkTick, g->unusedInvulnBlinkTick);
    FIELD(f->autofireTimer, g->autofireTimer);
    FIELD(f->autofireInterval, g->autofireInterval);
    FIELD(f->shieldL, g->shieldL);
    FIELD(f->shieldLIdx, g->shieldLIdx);
    FIELD(f->shieldR, g->shieldR);
    FIELD(f->shieldRIdx, g->shieldRIdx);
    FIELD(f->weaponAmmoPacked, g->weaponAmmoPacked);
    FIELD(f->energy, g->energy);
    FIELD(f->unused152, g->unused152);
    FIELD(f->bulletSpeedMult, g->bulletSpeedMult);
    FIELD(f->buffDuration, g->buffDuration);
    FIELD(f->dead, g->dead);
    FIELD(f->unused164, g->unused164);
    FIELD(f->respawnTime, g->respawnTime);
    FIELD(f->time, g->time);
    FIELD(f->shopVisits, g->shopVisits);
    FIELD(f->shieldHitFlashSpeed, g->shieldHitFlashSpeed);
    FIELD(f->enemyHpBonusRoll, g->enemyHpBonusRoll);
    FIELD(f->hurryupComboCount, g->hurryupComboCount);
    FIELD(f->rows, g->rows);
    FIELD(f->cols, g->cols);
    FIELD(f->rocketsFired, g->rocketsFired);
    FIELD(f->memoryGridUpgradeStreak, g->memoryGridUpgradeStreak);
    FIELD(f->memoryBonus, g->memoryBonus);
    FIELD(f->gemPickups, g->gemPickups);
    FIELD(f->unused19c, g->unused19c);
    FIELD(f->shieldGlowG, g->shieldGlowG);
    FIELD(f->shieldGlowB, g->shieldGlowB);
    FIELD(f->unused1a8, g->unused1a8);
    FIELD(f->shieldGlowGStep, g->shieldGlowGStep);
    FIELD(f->shieldGlowBStep, g->shieldGlowBStep);
    FIELD(f->unusedF1b4_5, g->unusedF1b4_5);
    FIELD(f->unusedF1b8_1, g->unusedF1b8_1);
    FIELD(f->placeSlot, g->placeSlot);
    FIELD(f->flame, g->flame);
    FIELD(f->shipDestroyedThisLevel, g->shipDestroyedThisLevel);
    FIELD(f->trackKillsFlag, g->trackKillsFlag);
    FIELD(f->bonusRoundEnded, g->bonusRoundEnded);
    FIELD(f->levelEnemyDataCount, g->levelEnemyDataCount);
    FIELD(f->unusedF1d4, g->unusedF1d4);
    FIELD(f->ship, g->ship);
    FIELD(f->unusedF1dc, g->unusedF1dc);
    FIELD(f->enemySwayX, g->enemySwayX);
    FIELD(f->enemySwayY, g->enemySwayY);
    FIELD(f->enemySwayVelX, g->enemySwayVelX);
    FIELD(f->enemySwayAccel, g->enemySwayAccel);
    FIELD(f->enemySwayMax, g->enemySwayMax);
    FIELD(f->enemySwayMin, g->enemySwayMin);
    FIELD(f->unusedF208, g->unusedF208);
    FIELD(f->unusedF20c, g->unusedF20c);
    FIELD(f->unusedF210, g->unusedF210);
    FIELD(f->unusedF214, g->unusedF214);
    FIELD(f->moneyMax, g->moneyMax);
    FIELD(f->moneyMaxShown, g->moneyMaxShown);
    FIELD(f->unusedF220, g->unusedF220);
    FIELD(f->unusedF224, g->unusedF224);
    FIELD(f->primaryEnemyCount, g->primaryEnemyCount);
    FIELD(f->secondaryEnemyCount, g->secondaryEnemyCount);
    FIELD(f->totalEnemies, g->totalEnemies);
    FIELD(f->spawnReserve, g->spawnReserve);
    FIELD(f->killed, g->killed);
    FIELD(f->escaped, g->escaped);
    FIELD(f->done, g->done);
    FIELD(f->doneTime, g->doneTime);
    FIELD(f->chainBonusValue, g->chainBonusValue);
    FIELD(f->started, g->started);
    FIELD(f->starVelX, g->starVelX);
    FIELD(f->starSpeed, g->starSpeed);
    FIELD(f->starVelZ, g->starVelZ);
    FIELD(f->savedRaceStarSpeed, g->savedRaceStarSpeed);
    FIELD(f->hyperspaceOutTimer, g->hyperspaceOutTimer);
    FIELD(f->hyperspaceMidTimer, g->hyperspaceMidTimer);
    FIELD(f->hyperspaceInTimer, g->hyperspaceInTimer);
    FIELD(f->hyperspaceInDuration, g->hyperspaceInDuration);
    FIELD(f->hyperspaceFade, g->hyperspaceFade);
    FIELD(f->savedHyperspaceFade, g->savedHyperspaceFade);
    FIELD(f->savedHyperspaceInDuration, g->savedHyperspaceInDuration);
    FIELD(f->savedHyperspaceInTimer, g->savedHyperspaceInTimer);
    FIELD(f->savedHyperspaceOutTimer, g->savedHyperspaceOutTimer);
    FIELD(f->savedStarSpeed, g->savedStarSpeed);
    FIELD(f->savedStarVelZ, g->savedStarVelZ);
    FIELD(f->savedScrollSpeedY, g->savedScrollSpeedY);
    FIELD(f->savedHyperspaceMidTimer, g->savedHyperspaceMidTimer);
    FIELD(f->scrollSpeedY, g->scrollSpeedY);
    FIELD(f->levelFinished, g->levelFinished);
    FIELD(f->levelTransitioning, g->levelTransitioning);
    FIELD(f->effectDuration, g->effectDuration);
    FIELD(f->levelWarpPending, g->levelWarpPending);
    FIELD(f->keyLatchFire, g->keyLatchFire);
    FIELD(f->keyLatchRocket, g->keyLatchRocket);
    FIELD(f->keyLatchPause, g->keyLatchPause);
    FIELD(f->keyLatchProfile, g->keyLatchProfile);
    FIELD(f->bonusTally, g->bonusTally);
    FIELD(f->bonusTallyTick, g->bonusTallyTick);
    FIELD(f->bonusTallyDelay, g->bonusTallyDelay);
    FIELD(f->perfectTextSpacing, g->perfectTextSpacing);
    FIELD(f->perfectTextSpacingVel, g->perfectTextSpacingVel);
    FIELD(f->perfectDone, g->perfectDone);
    FIELD(f->bonusResultsInitDone, g->bonusResultsInitDone);
    FIELD(f->bonusKilled, g->bonusKilled);
    FIELD(f->gemCounterPicks, g->gemCounterPicks);
    FIELD(f->gemCounterUnlocked, g->gemCounterUnlocked);
    FIELD(f->highScoreMilestone, g->highScoreMilestone);
    FIELD(f->blueMoneyPicks, g->blueMoneyPicks);
    FIELD(f->blueMoneyUnlocked, g->blueMoneyUnlocked);
    FIELD(f->multiplierPicks, g->multiplierPicks);
    FIELD(f->multiplierUnlocked, g->multiplierUnlocked);
    FIELD(f->weaponFloorAtOne, g->weaponFloorAtOne);
    FIELD(f->secretFlags, g->secretFlags);
    FIELD(f->secretCount, g->secretCount);
    FIELD(f->bonusRoundPoints, g->bonusRoundPoints);
    FIELD(f->perfectStreak, g->perfectStreak);
    FIELD(f->drunkStreak, g->drunkStreak);
    FIELD(f->rockets, g->rockets);
    FIELD(f->alienLock, g->alienLock);
    FIELD(f->tries, g->tries);
    FIELD(f->levelMilestoneHandled, g->levelMilestoneHandled);
    FIELD(f->secretBirdCounter, g->secretBirdCounter);
    FIELD(f->secretBirdTick, g->secretBirdTick);
    FIELD(f->raceDistance, g->raceDistance);
    FIELD(f->gameSpeedSetting, g->gameSpeedSetting);
    FIELD(f->maxRankReached, g->maxRankReached);
    FIELD(f->bulkLevelsCooldown, g->bulkLevelsCooldown);
    FIELD(f->nextShotSnd, g->nextShotSnd);
    FIELD(f->sessionPlayTime, g->sessionPlayTime);
}

static void PlayerFromFile(Player *g, const PlayerFile *f)
{
    FIELD(g->inputDevice, f->inputDevice);
    FIELD(g->x, f->x);
    FIELD(g->y, f->y);
    FIELD(g->mirrorX, f->mirrorX);
    FIELD(g->extraLetterE, f->extraLetterE);
    FIELD(g->speed, f->speed);
    FIELD(g->bank, f->bank);
    FIELD(g->writeOnlyF1c, f->writeOnlyF1c);
    FIELD(g->hitMaskParamA, f->hitMaskParamA);
    FIELD(g->hitMaskParamB, f->hitMaskParamB);
    Rect16FromFile(&g->box, &f->box);
    FIELD(g->shots, f->shots);
    FIELD(g->hits, f->hits);
    FIELD(g->lives, f->lives);
    FIELD(g->deaths, f->deaths);
    FIELD(g->collisionsTaken, f->collisionsTaken);
    FIELD(g->extraLetterR, f->extraLetterR);
    FIELD(g->score, f->score);
    FIELD(g->autofire, f->autofire);
    FIELD(g->autofireUnlocked, f->autofireUnlocked);
    FIELD(g->turretTrackingReduction, f->turretTrackingReduction);
    FIELD(g->bonusRoundScore, f->bonusRoundScore);
    FIELD(g->gemCounterCollected, f->gemCounterCollected);
    FIELD(g->bonusHighScore, f->bonusHighScore);
    FIELD(g->superAuto, f->superAuto);
    FIELD(g->money, f->money);
    FIELD(g->rank, f->rank);
    FIELD(g->bestRank, f->bestRank);
    FIELD(g->bonusRoundCount, f->bonusRoundCount);
    FIELD(g->gems, f->gems);
    FIELD(g->bombPickups, f->bombPickups);
    FIELD(g->marks, f->marks);
    FIELD(g->extraLetterX, f->extraLetterX);
    FIELD(g->color, f->color);
    FIELD(g->secretBirdHits, f->secretBirdHits);
    FIELD(g->level, f->level);
    FIELD(g->gemSeqB, f->gemSeqB);
    FIELD(g->gemSeqA, f->gemSeqA);
    FIELD(g->pickupCount, f->pickupCount);
    FIELD(g->extraProgress, f->extraProgress);
    FIELD(g->artxeProgress, f->artxeProgress);
    FIELD(g->displayLevel, f->displayLevel);
    FIELD(g->unusedScoreThreshold, f->unusedScoreThreshold);
    FIELD(g->extraLetterA, f->extraLetterA);
    FIELD(g->msMultiplierActive, f->msMultiplierActive);
    FIELD(g->writeOnlyMirrorFlag, f->writeOnlyMirrorFlag);
    FIELD(g->armour, f->armour);
    FIELD(g->freezeTimer, f->freezeTimer);
    FIELD(g->extraLetterT, f->extraLetterT);
    FIELD(g->scoreMult2Timer, f->scoreMult2Timer);
    FIELD(g->scoreMult5Timer, f->scoreMult5Timer);
    FIELD(g->mirrorTime, f->mirrorTime);
    FIELD(g->drunkModeTimer, f->drunkModeTimer);
    FIELD(g->scoopTimer, f->scoopTimer);
    FIELD(g->shieldTimer, f->shieldTimer);
    FIELD(g->blueMoneyActive, f->blueMoneyActive);
    FIELD(g->unusedInvulnBlinkFrame, f->unusedInvulnBlinkFrame);
    FIELD(g->unused12c, f->unused12c);
    FIELD(g->unusedInvulnBlinkTick, f->unusedInvulnBlinkTick);
    FIELD(g->autofireTimer, f->autofireTimer);
    FIELD(g->autofireInterval, f->autofireInterval);
    FIELD(g->shieldL, f->shieldL);
    FIELD(g->shieldLIdx, f->shieldLIdx);
    FIELD(g->shieldR, f->shieldR);
    FIELD(g->shieldRIdx, f->shieldRIdx);
    FIELD(g->weaponAmmoPacked, f->weaponAmmoPacked);
    FIELD(g->energy, f->energy);
    FIELD(g->unused152, f->unused152);
    FIELD(g->bulletSpeedMult, f->bulletSpeedMult);
    FIELD(g->buffDuration, f->buffDuration);
    FIELD(g->dead, f->dead);
    FIELD(g->unused164, f->unused164);
    FIELD(g->respawnTime, f->respawnTime);
    FIELD(g->time, f->time);
    FIELD(g->shopVisits, f->shopVisits);
    FIELD(g->shieldHitFlashSpeed, f->shieldHitFlashSpeed);
    FIELD(g->enemyHpBonusRoll, f->enemyHpBonusRoll);
    FIELD(g->hurryupComboCount, f->hurryupComboCount);
    FIELD(g->rows, f->rows);
    FIELD(g->cols, f->cols);
    FIELD(g->rocketsFired, f->rocketsFired);
    FIELD(g->memoryGridUpgradeStreak, f->memoryGridUpgradeStreak);
    FIELD(g->memoryBonus, f->memoryBonus);
    FIELD(g->gemPickups, f->gemPickups);
    FIELD(g->unused19c, f->unused19c);
    FIELD(g->shieldGlowG, f->shieldGlowG);
    FIELD(g->shieldGlowB, f->shieldGlowB);
    FIELD(g->unused1a8, f->unused1a8);
    FIELD(g->shieldGlowGStep, f->shieldGlowGStep);
    FIELD(g->shieldGlowBStep, f->shieldGlowBStep);
    FIELD(g->unusedF1b4_5, f->unusedF1b4_5);
    FIELD(g->unusedF1b8_1, f->unusedF1b8_1);
    FIELD(g->placeSlot, f->placeSlot);
    FIELD(g->flame, f->flame);
    FIELD(g->shipDestroyedThisLevel, f->shipDestroyedThisLevel);
    FIELD(g->trackKillsFlag, f->trackKillsFlag);
    FIELD(g->bonusRoundEnded, f->bonusRoundEnded);
    FIELD(g->levelEnemyDataCount, f->levelEnemyDataCount);
    FIELD(g->unusedF1d4, f->unusedF1d4);
    FIELD(g->ship, f->ship);
    FIELD(g->unusedF1dc, f->unusedF1dc);
    FIELD(g->enemySwayX, f->enemySwayX);
    FIELD(g->enemySwayY, f->enemySwayY);
    FIELD(g->enemySwayVelX, f->enemySwayVelX);
    FIELD(g->enemySwayAccel, f->enemySwayAccel);
    FIELD(g->enemySwayMax, f->enemySwayMax);
    FIELD(g->enemySwayMin, f->enemySwayMin);
    FIELD(g->unusedF208, f->unusedF208);
    FIELD(g->unusedF20c, f->unusedF20c);
    FIELD(g->unusedF210, f->unusedF210);
    FIELD(g->unusedF214, f->unusedF214);
    FIELD(g->moneyMax, f->moneyMax);
    FIELD(g->moneyMaxShown, f->moneyMaxShown);
    FIELD(g->unusedF220, f->unusedF220);
    FIELD(g->unusedF224, f->unusedF224);
    FIELD(g->primaryEnemyCount, f->primaryEnemyCount);
    FIELD(g->secondaryEnemyCount, f->secondaryEnemyCount);
    FIELD(g->totalEnemies, f->totalEnemies);
    FIELD(g->spawnReserve, f->spawnReserve);
    FIELD(g->killed, f->killed);
    FIELD(g->escaped, f->escaped);
    FIELD(g->done, f->done);
    FIELD(g->doneTime, f->doneTime);
    FIELD(g->chainBonusValue, f->chainBonusValue);
    FIELD(g->started, f->started);
    FIELD(g->starVelX, f->starVelX);
    FIELD(g->starSpeed, f->starSpeed);
    FIELD(g->starVelZ, f->starVelZ);
    FIELD(g->savedRaceStarSpeed, f->savedRaceStarSpeed);
    FIELD(g->hyperspaceOutTimer, f->hyperspaceOutTimer);
    FIELD(g->hyperspaceMidTimer, f->hyperspaceMidTimer);
    FIELD(g->hyperspaceInTimer, f->hyperspaceInTimer);
    FIELD(g->hyperspaceInDuration, f->hyperspaceInDuration);
    FIELD(g->hyperspaceFade, f->hyperspaceFade);
    FIELD(g->savedHyperspaceFade, f->savedHyperspaceFade);
    FIELD(g->savedHyperspaceInDuration, f->savedHyperspaceInDuration);
    FIELD(g->savedHyperspaceInTimer, f->savedHyperspaceInTimer);
    FIELD(g->savedHyperspaceOutTimer, f->savedHyperspaceOutTimer);
    FIELD(g->savedStarSpeed, f->savedStarSpeed);
    FIELD(g->savedStarVelZ, f->savedStarVelZ);
    FIELD(g->savedScrollSpeedY, f->savedScrollSpeedY);
    FIELD(g->savedHyperspaceMidTimer, f->savedHyperspaceMidTimer);
    FIELD(g->scrollSpeedY, f->scrollSpeedY);
    FIELD(g->levelFinished, f->levelFinished);
    FIELD(g->levelTransitioning, f->levelTransitioning);
    FIELD(g->effectDuration, f->effectDuration);
    FIELD(g->levelWarpPending, f->levelWarpPending);
    FIELD(g->keyLatchFire, f->keyLatchFire);
    FIELD(g->keyLatchRocket, f->keyLatchRocket);
    FIELD(g->keyLatchPause, f->keyLatchPause);
    FIELD(g->keyLatchProfile, f->keyLatchProfile);
    FIELD(g->bonusTally, f->bonusTally);
    FIELD(g->bonusTallyTick, f->bonusTallyTick);
    FIELD(g->bonusTallyDelay, f->bonusTallyDelay);
    FIELD(g->perfectTextSpacing, f->perfectTextSpacing);
    FIELD(g->perfectTextSpacingVel, f->perfectTextSpacingVel);
    FIELD(g->perfectDone, f->perfectDone);
    FIELD(g->bonusResultsInitDone, f->bonusResultsInitDone);
    FIELD(g->bonusKilled, f->bonusKilled);
    FIELD(g->gemCounterPicks, f->gemCounterPicks);
    FIELD(g->gemCounterUnlocked, f->gemCounterUnlocked);
    FIELD(g->highScoreMilestone, f->highScoreMilestone);
    FIELD(g->blueMoneyPicks, f->blueMoneyPicks);
    FIELD(g->blueMoneyUnlocked, f->blueMoneyUnlocked);
    FIELD(g->multiplierPicks, f->multiplierPicks);
    FIELD(g->multiplierUnlocked, f->multiplierUnlocked);
    FIELD(g->weaponFloorAtOne, f->weaponFloorAtOne);
    FIELD(g->secretFlags, f->secretFlags);
    FIELD(g->secretCount, f->secretCount);
    FIELD(g->bonusRoundPoints, f->bonusRoundPoints);
    FIELD(g->perfectStreak, f->perfectStreak);
    FIELD(g->drunkStreak, f->drunkStreak);
    FIELD(g->rockets, f->rockets);
    FIELD(g->alienLock, f->alienLock);
    FIELD(g->tries, f->tries);
    FIELD(g->levelMilestoneHandled, f->levelMilestoneHandled);
    FIELD(g->secretBirdCounter, f->secretBirdCounter);
    FIELD(g->secretBirdTick, f->secretBirdTick);
    FIELD(g->raceDistance, f->raceDistance);
    FIELD(g->gameSpeedSetting, f->gameSpeedSetting);
    FIELD(g->maxRankReached, f->maxRankReached);
    FIELD(g->bulkLevelsCooldown, f->bulkLevelsCooldown);
    FIELD(g->nextShotSnd, f->nextShotSnd);
    FIELD(g->sessionPlayTime, f->sessionPlayTime);
}

static void EnemyToFile(EnemyFile *f, const Enemy *g)
{
    FIELD(f->unused00, g->unused00);
    FIELD(f->active, g->active);
    FIELD(f->x, g->x);
    FIELD(f->y, g->y);
    FIELD(f->homeX, g->homeX);
    FIELD(f->homeY, g->homeY);
    FIELD(f->velX, g->velX);
    FIELD(f->velY, g->velY);
    FIELD(f->accelX, g->accelX);
    FIELD(f->accelY, g->accelY);
    FIELD(f->speedScale, g->speedScale);
    FIELD(f->patternTimer, g->patternTimer);
    FIELD(f->hoverX, g->hoverX);
    FIELD(f->hoverY, g->hoverY);
    FIELD(f->speedX, g->speedX);
    FIELD(f->speedY, g->speedY);
    FIELD(f->offsetX, g->offsetX);
    FIELD(f->offsetY, g->offsetY);
    FIELD(f->oscillateMul, g->oscillateMul);
    FIELD(f->oscillateAccel, g->oscillateAccel);
    FIELD(f->oscillatePhase, g->oscillatePhase);
    FIELD(f->ownerPlayer, g->ownerPlayer);
    FIELD(f->beamOffsetX, g->beamOffsetX);
    FIELD(f->beamSide, g->beamSide);
    FIELD(f->homing, g->homing);
    FIELD(f->settled, g->settled);
    FIELD(f->altFireActive, g->altFireActive);
    FIELD(f->pairedEnemyIdx, g->pairedEnemyIdx);
    FIELD(f->savedFireDelay, g->savedFireDelay);
    FIELD(f->savedHp, g->savedHp);
    FIELD(f->unusedF7c, g->unusedF7c);
    FIELD(f->unusedF80, g->unusedF80);
    FIELD(f->debrisVelX, g->debrisVelX);
    FIELD(f->debrisVelY, g->debrisVelY);
    FIELD(f->attackStaggerTimer, g->attackStaggerTimer);
    FIELD(f->turnTimer, g->turnTimer);
    FIELD(f->dirStepTimer, g->dirStepTimer);
    FIELD(f->type, g->type);
    FIELD(f->patternId, g->patternId);
    FIELD(f->hazardType, g->hazardType);
    FIELD(f->srcX, g->srcX);
    FIELD(f->srcY, g->srcY);
    FIELD(f->facing, g->facing);
    FIELD(f->turnState, g->turnState);
    FIELD(f->zigzagTimer, g->zigzagTimer);
    FIELD(f->unusedAlpha, g->unusedAlpha);
    FIELD(f->zigzagVelX, g->zigzagVelX);
    FIELD(f->zigzagAccel, g->zigzagAccel);
    FIELD(f->descendVelY, g->descendVelY);
    FIELD(f->descendAccelY, g->descendAccelY);
    FIELD(f->descendVelX, g->descendVelX);
    FIELD(f->descendAccelX, g->descendAccelX);
    FIELD(f->turnTimer2, g->turnTimer2);
    FIELD(f->turnTimer2Max, g->turnTimer2Max);
    FIELD(f->animFrame, g->animFrame);
    FIELD(f->animDelay, g->animDelay);
    FIELD(f->animTimer, g->animTimer);
    FIELD(f->animReverse, g->animReverse);
    FIELD(f->useDirRemap, g->useDirRemap);
    FIELD(f->altFrameCounter, g->altFrameCounter);
    FIELD(f->shotGfxW, g->shotGfxW);
    FIELD(f->shotGfxH, g->shotGfxH);
    Rect16ToFile(&f->srcRect, &g->srcRect);
    FIELD(f->hitFlashTimer, g->hitFlashTimer);
    FIELD(f->hp, g->hp);
    FIELD(f->maxHp, g->maxHp);
    FIELD(f->score, g->score);
    FIELD(f->attackDelay, g->attackDelay);
    FIELD(f->attackDelayStep, g->attackDelayStep);
    FIELD(f->fireDelay, g->fireDelay);
    FIELD(f->fireDelayStep, g->fireDelayStep);
    FIELD(f->patternEndAction, g->patternEndAction);
    FrameSetToFile(&f->frameSet, &g->frameSet);
    Rect16ToFile(&f->hitRect, &g->hitRect);
    FIELD(f->groupIndex, g->groupIndex);
    FIELD(f->bonusGroupIndex, g->bonusGroupIndex);
    FIELD(f->patternStep, g->patternStep);
    FIELD(f->animSpeedDivisor, g->animSpeedDivisor);
    FIELD(f->unusedSpawnDelay, g->unusedSpawnDelay);
    FIELD(f->animStepTime, g->animStepTime);
    FIELD(f->animFrameCount, g->animFrameCount);
    FIELD(f->animPingPong, g->animPingPong);
    FIELD(f->forcedDir, g->forcedDir);
    FIELD(f->bossGunASlot, g->bossGunASlot);
    FIELD(f->bossGunBSlot, g->bossGunBSlot);
    FIELD(f->bossGunCSlot, g->bossGunCSlot);
    FIELD(f->bossGunAX, g->bossGunAX);
    FIELD(f->bossGunAY, g->bossGunAY);
    FIELD(f->bossGunBX, g->bossGunBX);
    FIELD(f->bossGunBY, g->bossGunBY);
    FIELD(f->bossGunCX, g->bossGunCX);
    FIELD(f->bossGunCY, g->bossGunCY);
    FIELD(f->deathExplosionGfx, g->deathExplosionGfx);
    FIELD(f->deathExplosionLife, g->deathExplosionLife);
    FIELD(f->deathExplosionR, g->deathExplosionR);
    FIELD(f->deathExplosionG, g->deathExplosionG);
    FIELD(f->deathExplosionB, g->deathExplosionB);
    FIELD(f->locked, g->locked);
    FIELD(f->fixedFireDelay, g->fixedFireDelay);
    FIELD(f->flashActive, g->flashActive);
    FIELD(f->flashTimer, g->flashTimer);
}

static void EnemyFromFile(Enemy *g, const EnemyFile *f)
{
    FIELD(g->unused00, f->unused00);
    FIELD(g->active, f->active);
    FIELD(g->x, f->x);
    FIELD(g->y, f->y);
    FIELD(g->homeX, f->homeX);
    FIELD(g->homeY, f->homeY);
    FIELD(g->velX, f->velX);
    FIELD(g->velY, f->velY);
    FIELD(g->accelX, f->accelX);
    FIELD(g->accelY, f->accelY);
    FIELD(g->speedScale, f->speedScale);
    FIELD(g->patternTimer, f->patternTimer);
    FIELD(g->hoverX, f->hoverX);
    FIELD(g->hoverY, f->hoverY);
    FIELD(g->speedX, f->speedX);
    FIELD(g->speedY, f->speedY);
    FIELD(g->offsetX, f->offsetX);
    FIELD(g->offsetY, f->offsetY);
    FIELD(g->oscillateMul, f->oscillateMul);
    FIELD(g->oscillateAccel, f->oscillateAccel);
    FIELD(g->oscillatePhase, f->oscillatePhase);
    FIELD(g->ownerPlayer, f->ownerPlayer);
    FIELD(g->beamOffsetX, f->beamOffsetX);
    FIELD(g->beamSide, f->beamSide);
    FIELD(g->homing, f->homing);
    FIELD(g->settled, f->settled);
    FIELD(g->altFireActive, f->altFireActive);
    FIELD(g->pairedEnemyIdx, f->pairedEnemyIdx);
    FIELD(g->savedFireDelay, f->savedFireDelay);
    FIELD(g->savedHp, f->savedHp);
    FIELD(g->unusedF7c, f->unusedF7c);
    FIELD(g->unusedF80, f->unusedF80);
    FIELD(g->debrisVelX, f->debrisVelX);
    FIELD(g->debrisVelY, f->debrisVelY);
    FIELD(g->attackStaggerTimer, f->attackStaggerTimer);
    FIELD(g->turnTimer, f->turnTimer);
    FIELD(g->dirStepTimer, f->dirStepTimer);
    FIELD(g->type, f->type);
    FIELD(g->patternId, f->patternId);
    FIELD(g->hazardType, f->hazardType);
    FIELD(g->srcX, f->srcX);
    FIELD(g->srcY, f->srcY);
    FIELD(g->facing, f->facing);
    FIELD(g->turnState, f->turnState);
    FIELD(g->zigzagTimer, f->zigzagTimer);
    FIELD(g->unusedAlpha, f->unusedAlpha);
    FIELD(g->zigzagVelX, f->zigzagVelX);
    FIELD(g->zigzagAccel, f->zigzagAccel);
    FIELD(g->descendVelY, f->descendVelY);
    FIELD(g->descendAccelY, f->descendAccelY);
    FIELD(g->descendVelX, f->descendVelX);
    FIELD(g->descendAccelX, f->descendAccelX);
    FIELD(g->turnTimer2, f->turnTimer2);
    FIELD(g->turnTimer2Max, f->turnTimer2Max);
    FIELD(g->animFrame, f->animFrame);
    FIELD(g->animDelay, f->animDelay);
    FIELD(g->animTimer, f->animTimer);
    FIELD(g->animReverse, f->animReverse);
    FIELD(g->useDirRemap, f->useDirRemap);
    FIELD(g->altFrameCounter, f->altFrameCounter);
    FIELD(g->shotGfxW, f->shotGfxW);
    FIELD(g->shotGfxH, f->shotGfxH);
    Rect16FromFile(&g->srcRect, &f->srcRect);
    FIELD(g->hitFlashTimer, f->hitFlashTimer);
    FIELD(g->hp, f->hp);
    FIELD(g->maxHp, f->maxHp);
    FIELD(g->score, f->score);
    FIELD(g->attackDelay, f->attackDelay);
    FIELD(g->attackDelayStep, f->attackDelayStep);
    FIELD(g->fireDelay, f->fireDelay);
    FIELD(g->fireDelayStep, f->fireDelayStep);
    FIELD(g->patternEndAction, f->patternEndAction);
    FrameSetFromFile(&g->frameSet, &f->frameSet);
    Rect16FromFile(&g->hitRect, &f->hitRect);
    FIELD(g->groupIndex, f->groupIndex);
    FIELD(g->bonusGroupIndex, f->bonusGroupIndex);
    FIELD(g->patternStep, f->patternStep);
    FIELD(g->animSpeedDivisor, f->animSpeedDivisor);
    FIELD(g->unusedSpawnDelay, f->unusedSpawnDelay);
    FIELD(g->animStepTime, f->animStepTime);
    FIELD(g->animFrameCount, f->animFrameCount);
    FIELD(g->animPingPong, f->animPingPong);
    FIELD(g->forcedDir, f->forcedDir);
    FIELD(g->bossGunASlot, f->bossGunASlot);
    FIELD(g->bossGunBSlot, f->bossGunBSlot);
    FIELD(g->bossGunCSlot, f->bossGunCSlot);
    FIELD(g->bossGunAX, f->bossGunAX);
    FIELD(g->bossGunAY, f->bossGunAY);
    FIELD(g->bossGunBX, f->bossGunBX);
    FIELD(g->bossGunBY, f->bossGunBY);
    FIELD(g->bossGunCX, f->bossGunCX);
    FIELD(g->bossGunCY, f->bossGunCY);
    FIELD(g->deathExplosionGfx, f->deathExplosionGfx);
    FIELD(g->deathExplosionLife, f->deathExplosionLife);
    FIELD(g->deathExplosionR, f->deathExplosionR);
    FIELD(g->deathExplosionG, f->deathExplosionG);
    FIELD(g->deathExplosionB, f->deathExplosionB);
    FIELD(g->locked, f->locked);
    FIELD(g->fixedFireDelay, f->fixedFireDelay);
    FIELD(g->flashActive, f->flashActive);
    FIELD(g->flashTimer, f->flashTimer);
}

static void SaveDataToFile(SaveDataFile *f, const SaveData *g)
{
    int i;

    FIELD(f->sig, g->sig);
    FIELD(f->saveId, g->saveId);
    for (i = 0; i < 4; i++)
        PlayerToFile(&f->players[i], &g->players[i]);
}

static void SaveDataFromFile(SaveData *g, const SaveDataFile *f)
{
    int i;

    FIELD(g->sig, f->sig);
    FIELD(g->saveId, f->saveId);
    for (i = 0; i < 4; i++)
        PlayerFromFile(&g->players[i], &f->players[i]);
}

// Copies the game state into the file image (dead zones stay as they are: zeroed by the caller).
static void GameToFile(SaveFile *f)
{
    int i;
    int j;

    SaveDataToFile(&f->save, &g_save);
    for (i = 0; i < 2; i++)
        for (j = 0; j < 150; j++)
            EnemyToFile(&f->enemies[i][j], &g_enemies[i][j]);
    for (i = 0; i < 4000; i++)
        LevelRecToFile(&f->levelRecs[i], &g_levelRecs[i]);
    FIELD(f->curLevelNum, g_curLevelNum);
    FIELD(f->levelDataLoaded, g_levelDataLoaded);
    FIELD(f->loadedLevel, g_loadedLevel);
    FIELD(f->warpLevelR, g_warpLevelR);
    FIELD(f->warpLevelL, g_warpLevelL);
    FIELD(f->marksBonusGiven, g_marksBonusGiven);
    FIELD(f->enemyAimAtPlayer, g_enemyAimAtPlayer);
    FIELD(f->fastEnemyBullets, g_fastEnemyBullets);
    FIELD(f->diffEnemyFireChance, g_diffEnemyFireChance);
    FIELD(f->diffShotFuseBase, g_diffShotFuseBase);
    FIELD(f->diffShotFuseRange, g_diffShotFuseRange);
    FIELD(f->hurryUpInterval, g_hurryUpInterval);
    FIELD(f->diffShotSpeedMin, g_diffShotSpeedMin);
    FIELD(f->diffShotSpeedMax, g_diffShotSpeedMax);
    FIELD(f->diffTurretTrackChance, g_diffTurretTrackChance);
    FIELD(f->diffBonusDropRoll, g_diffBonusDropRoll);
    FIELD(f->curPlayer, g_curPlayer);
    FIELD(f->shopCurPlayer, g_shopCurPlayer);
    FIELD(f->promoPlayer, g_promoPlayer);
    FIELD(f->bonusStagePlayer, g_bonusStagePlayer);
    FIELD(f->vsTurnPlayer, g_vsTurnPlayer);
    FIELD(f->saveVersion, g_saveVersion);
    FIELD(f->diffScoreBonus, g_diffScoreBonus);
    FIELD(f->defaultObjAlpha, g_defaultObjAlpha);
    FIELD(f->enemyBulletSpeed, g_enemyBulletSpeed);
    FIELD(f->playerStartSpeed, g_playerStartSpeed);
    FIELD(f->diffEnemyTimerMul, g_diffEnemyTimerMul);
    FIELD(f->diffEnemyHpBonus, g_diffEnemyHpBonus);
    FIELD(f->moneySuckerBaseHp, g_moneySuckerBaseHp);
    FIELD(f->diffHurryUpSpeedMax, g_diffHurryUpSpeedMax);
    FIELD(f->eliteHpBonus, g_eliteHpBonus);
    FIELD(f->hurryUpHpBonus, g_hurryUpHpBonus);
    FIELD(f->speedBase, g_speedBase);
    FIELD(f->speedStep, g_speedStep);
    FIELD(f->maxSpeedMul, g_maxSpeedMul);
    FIELD(f->speedMax, g_speedMax);
    FIELD(f->bonusDuration, g_bonusDuration);
    FIELD(f->bonusSpawnRampRate, g_bonusSpawnRampRate);
    FIELD(f->bonusThresholdBase, g_bonusThresholdBase);
    FIELD(f->bonusRareChance, g_bonusRareChance);
    FIELD(f->timeMax, g_timeMax);
    FIELD(f->speedMin, g_speedMin);
    FIELD(f->lastEliteSpawnLevel, g_lastEliteSpawnLevel);
    FIELD(f->fireDelayMin, g_fireDelayMin);
    FIELD(f->enemyFireRateMin, g_enemyFireRateMin);
    FIELD(f->fireDelayBiasA, g_fireDelayBiasA);
    FIELD(f->fireDelayBiasB, g_fireDelayBiasB);
    FIELD(f->resumeTimeOffset, g_resumeTimeOffset);
    FIELD(f->comboLevel, g_comboLevel);
    FIELD(f->comboStep, g_comboStep);
    FIELD(f->sessionScore, g_sessionScore);
    FIELD(f->hits, g_hits);
    FIELD(f->gameMode, g_gameMode);
    FIELD(f->pendingGameMode, g_pendingGameMode);
    FIELD(f->hofMode, g_hofMode);
    FIELD(f->savedDifficultySave, g_savedDifficultySave);
    FIELD(f->perfectCount, g_perfectCount);
    FIELD(f->killCount, g_killCount);
}

// Copies the file image into the game state; dead zones are ignored.
static void GameFromFile(const SaveFile *f)
{
    int i;
    int j;

    SaveDataFromFile(&g_save, &f->save);
    for (i = 0; i < 2; i++)
        for (j = 0; j < 150; j++)
            EnemyFromFile(&g_enemies[i][j], &f->enemies[i][j]);
    for (i = 0; i < 4000; i++)
        LevelRecFromFile(&g_levelRecs[i], &f->levelRecs[i]);
    FIELD(g_curLevelNum, f->curLevelNum);
    FIELD(g_levelDataLoaded, f->levelDataLoaded);
    FIELD(g_loadedLevel, f->loadedLevel);
    FIELD(g_warpLevelR, f->warpLevelR);
    FIELD(g_warpLevelL, f->warpLevelL);
    FIELD(g_marksBonusGiven, f->marksBonusGiven);
    FIELD(g_enemyAimAtPlayer, f->enemyAimAtPlayer);
    FIELD(g_fastEnemyBullets, f->fastEnemyBullets);
    FIELD(g_diffEnemyFireChance, f->diffEnemyFireChance);
    FIELD(g_diffShotFuseBase, f->diffShotFuseBase);
    FIELD(g_diffShotFuseRange, f->diffShotFuseRange);
    FIELD(g_hurryUpInterval, f->hurryUpInterval);
    FIELD(g_diffShotSpeedMin, f->diffShotSpeedMin);
    FIELD(g_diffShotSpeedMax, f->diffShotSpeedMax);
    FIELD(g_diffTurretTrackChance, f->diffTurretTrackChance);
    FIELD(g_diffBonusDropRoll, f->diffBonusDropRoll);
    FIELD(g_curPlayer, f->curPlayer);
    FIELD(g_shopCurPlayer, f->shopCurPlayer);
    FIELD(g_promoPlayer, f->promoPlayer);
    FIELD(g_bonusStagePlayer, f->bonusStagePlayer);
    FIELD(g_vsTurnPlayer, f->vsTurnPlayer);
    FIELD(g_saveVersion, f->saveVersion);
    FIELD(g_diffScoreBonus, f->diffScoreBonus);
    FIELD(g_defaultObjAlpha, f->defaultObjAlpha);
    FIELD(g_enemyBulletSpeed, f->enemyBulletSpeed);
    FIELD(g_playerStartSpeed, f->playerStartSpeed);
    FIELD(g_diffEnemyTimerMul, f->diffEnemyTimerMul);
    FIELD(g_diffEnemyHpBonus, f->diffEnemyHpBonus);
    FIELD(g_moneySuckerBaseHp, f->moneySuckerBaseHp);
    FIELD(g_diffHurryUpSpeedMax, f->diffHurryUpSpeedMax);
    FIELD(g_eliteHpBonus, f->eliteHpBonus);
    FIELD(g_hurryUpHpBonus, f->hurryUpHpBonus);
    FIELD(g_speedBase, f->speedBase);
    FIELD(g_speedStep, f->speedStep);
    FIELD(g_maxSpeedMul, f->maxSpeedMul);
    FIELD(g_speedMax, f->speedMax);
    FIELD(g_bonusDuration, f->bonusDuration);
    FIELD(g_bonusSpawnRampRate, f->bonusSpawnRampRate);
    FIELD(g_bonusThresholdBase, f->bonusThresholdBase);
    FIELD(g_bonusRareChance, f->bonusRareChance);
    FIELD(g_timeMax, f->timeMax);
    FIELD(g_speedMin, f->speedMin);
    FIELD(g_lastEliteSpawnLevel, f->lastEliteSpawnLevel);
    FIELD(g_fireDelayMin, f->fireDelayMin);
    FIELD(g_enemyFireRateMin, f->enemyFireRateMin);
    FIELD(g_fireDelayBiasA, f->fireDelayBiasA);
    FIELD(g_fireDelayBiasB, f->fireDelayBiasB);
    FIELD(g_resumeTimeOffset, f->resumeTimeOffset);
    FIELD(g_comboLevel, f->comboLevel);
    FIELD(g_comboStep, f->comboStep);
    FIELD(g_sessionScore, f->sessionScore);
    FIELD(g_hits, f->hits);
    FIELD(g_gameMode, f->gameMode);
    FIELD(g_pendingGameMode, f->pendingGameMode);
    FIELD(g_hofMode, f->hofMode);
    FIELD(g_savedDifficultySave, f->savedDifficultySave);
    FIELD(g_perfectCount, f->perfectCount);
    FIELD(g_killCount, f->killCount);
}

// zlib's compressBound(): the largest size compress() can produce for n bytes.
#define COMPRESS_BOUND(n) ((n) + ((n) >> 12) + ((n) >> 14) + ((n) >> 25) + 13)

// Writes the game state to `path` as a zlib-compressed SaveFile. Returns false if it could not.
bool SaveGameToFile(const char *path)
{
    SaveFile *f;
    unsigned char *buf;
    unsigned long cSize;
    int fd;
    bool ok = false;

    f = (SaveFile *)calloc(1, sizeof(SaveFile));
    cSize = COMPRESS_BOUND(sizeof(SaveFile));
    buf = (unsigned char *)malloc(cSize);
    if (f != 0 && buf != 0) {
        GameToFile(f);
        if (compress(buf, &cSize, (const unsigned char *)f, sizeof(SaveFile)) == 0) {
            fd = _open(path, _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
            if (fd != -1) {
                ok = _write(fd, buf, cSize) == (int)cSize;
                _close(fd);
            }
        }
    }
    free(buf);
    free(f);
    return ok;
}

// Reads and uncompresses `path`. Returns the SaveFile (free() it) if the file is a whole save
// with the "SDY" signature and the current save version, otherwise 0.
static SaveFile *ReadSaveFile(const char *path)
{
    SaveFile *f;
    unsigned char *buf;
    unsigned long uSize;
    int n = 0;
    int fd;

    f = (SaveFile *)malloc(sizeof(SaveFile));
    buf = (unsigned char *)malloc(sizeof(SaveFile));
    if (f != 0 && buf != 0) {
        fd = _open(path, _O_RDONLY | _O_BINARY, 0);
        if (fd != -1) {
            n = _read(fd, buf, sizeof(SaveFile));
            _close(fd);
        }
        uSize = sizeof(SaveFile);
        if (n > 0 && uncompress((unsigned char *)f, &uSize, buf, n) == 0 && uSize == sizeof(SaveFile)
            && f->save.sig[0] == 'S' && f->save.sig[1] == 'D' && f->save.sig[2] == 'Y'
            && f->saveVersion == g_saveMagic) {
            free(buf);
            return f;
        }
    }
    free(buf);
    free(f);
    return 0;
}

// Whether `path` holds a valid, current-version suspended game. Leaves the game state alone.
bool SaveFileValid(const char *path)
{
    SaveFile *f = ReadSaveFile(path);
    bool ok = f != 0;

    free(f);
    return ok;
}

// Loads the suspended game in `path` into the game state. Returns false (and changes nothing)
// if the file is missing or not a valid save. Pointers are not restored: see LoadSuspended.
bool LoadGameFromFile(const char *path)
{
    SaveFile *f = ReadSaveFile(path);

    if (f == 0)
        return false;
    GameFromFile(f);
    free(f);
    return true;
}
