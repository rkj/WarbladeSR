// static_init.cpp: Dynamic initializers of game globals (score tables, prices, end colours).
#include "globals.h"
#include "game.h"


// ---- dynamic initializers of game globals ----
// Each global below is initialised by a compiler-generated `dynamic initializer` (??__E<name>@@YAXXZ,
// internal linkage) located at the address in its comment.

// Y coordinate of the ground line players stand/land on: fixed offset above the bottom of the screen.
// 0x774de0

#pragma bss_seg(".bss$ge11ccc")
int g_floorY = g_screenH - 50;

// Combo-hit score bonus, indexed by combo length (0-9): steeply increasing rewards for longer combos.
// 0x774e20
#pragma bss_seg(".bss$ge11ed0")
__int64 g_comboBonusTable[10] = {
    g_scoreUnit * 10, g_scoreUnit * 25, g_scoreUnit * 50, g_scoreUnit * 100,
    g_scoreUnit * 250, g_scoreUnit * 500, g_scoreUnit * 1000, g_scoreUnit * 2500,
    g_scoreUnit * 5000, g_scoreUnit * 10000
};

// Shop item prices, indexed by item id.
// 0x774f60
#pragma bss_seg(".bss$ge11cd8")
int g_prices[25] = {
    (int)g_valueObfuscationMult * 0, (int)g_valueObfuscationMult * 50, (int)g_valueObfuscationMult * 75, (int)g_valueObfuscationMult * 100,
    (int)g_valueObfuscationMult * 150, (int)g_valueObfuscationMult * 200, (int)g_valueObfuscationMult * 300, (int)g_valueObfuscationMult * 400,
    (int)g_valueObfuscationMult * 500, (int)g_valueObfuscationMult * 600, (int)g_valueObfuscationMult * 750, (int)g_valueObfuscationMult * 800,
    (int)g_valueObfuscationMult * 990, (int)g_valueObfuscationMult * 1000, (int)g_valueObfuscationMult * 1250, (int)g_valueObfuscationMult * 1500,
    (int)g_valueObfuscationMult * 2000, (int)g_valueObfuscationMult * 3000, (int)g_valueObfuscationMult * 5000, (int)g_valueObfuscationMult * 15000,
    (int)g_valueObfuscationMult * 30000, (int)g_valueObfuscationMult * 500000
};

// Kill-score award per enemy type, indexed by enemy type id.
// 0x775150
#pragma bss_seg(".bss$ge11d40")
__int64 g_enemyScoreTable[50] = {
    g_valueObfuscationMult, g_valueObfuscationMult, OBFUSCATE_VALUE(3), OBFUSCATE_VALUE(0),
    OBFUSCATE_VALUE(8), OBFUSCATE_VALUE(14), OBFUSCATE_VALUE(3), OBFUSCATE_VALUE(0),
    OBFUSCATE_VALUE(0), OBFUSCATE_VALUE(25000), OBFUSCATE_VALUE(0), OBFUSCATE_VALUE(10000000),
    OBFUSCATE_VALUE(100000), OBFUSCATE_VALUE(1000000), OBFUSCATE_VALUE(500000), OBFUSCATE_VALUE(10000),
    OBFUSCATE_VALUE(25000), OBFUSCATE_VALUE(50000), OBFUSCATE_VALUE(1000), OBFUSCATE_VALUE(100),
    OBFUSCATE_VALUE(5000), OBFUSCATE_VALUE(2500), OBFUSCATE_VALUE(10), OBFUSCATE_VALUE(99990),
    OBFUSCATE_VALUE(50), OBFUSCATE_VALUE(100), OBFUSCATE_VALUE(200), OBFUSCATE_VALUE(500),
    OBFUSCATE_VALUE(250), OBFUSCATE_VALUE(5000000), OBFUSCATE_VALUE(999990), OBFUSCATE_VALUE(2000),
    OBFUSCATE_VALUE(450000), OBFUSCATE_VALUE(250000000), OBFUSCATE_VALUE(2000000), OBFUSCATE_VALUE(20000000),
    OBFUSCATE_VALUE(2500000)
};

// End-of-level rank bonus, indexed by the player's rank for that level.
// 0x7757e0
#pragma bss_seg(".bss$ge11f20")
__int64 g_rankBonusTable[33] = {
    OBFUSCATE_VALUE(10000), OBFUSCATE_VALUE(20000), OBFUSCATE_VALUE(30000), OBFUSCATE_VALUE(40000),
    OBFUSCATE_VALUE(50000), OBFUSCATE_VALUE(60000), OBFUSCATE_VALUE(70000), OBFUSCATE_VALUE(80000),
    OBFUSCATE_VALUE(90000), OBFUSCATE_VALUE(100000), OBFUSCATE_VALUE(200000), OBFUSCATE_VALUE(300000),
    OBFUSCATE_VALUE(400000), OBFUSCATE_VALUE(500000), OBFUSCATE_VALUE(600000), OBFUSCATE_VALUE(700000),
    OBFUSCATE_VALUE(800000), OBFUSCATE_VALUE(1000000), OBFUSCATE_VALUE(2000000), OBFUSCATE_VALUE(3000000),
    OBFUSCATE_VALUE(4000000), OBFUSCATE_VALUE(5000000), OBFUSCATE_VALUE(10000000), OBFUSCATE_VALUE(10000000),
    OBFUSCATE_VALUE(10000000), OBFUSCATE_VALUE(10000000), OBFUSCATE_VALUE(10000000), OBFUSCATE_VALUE(10000000),
    OBFUSCATE_VALUE(10000000), OBFUSCATE_VALUE(10000000), OBFUSCATE_VALUE(10000000), OBFUSCATE_VALUE(10000000),
    OBFUSCATE_VALUE(50000000)
};

// Blink interval (ms) for the "saved" UI message; doubled temporarily elsewhere for emphasis, then
// reset back to g_blinkRate.
// 0x775df0
#pragma bss_seg(".bss$ge11cd0")
unsigned int g_saveMsgBlinkRate = g_blinkRate;

// Random end-sequence background colour (R/G/B, each rolled independently), 200-250 out of 255.
// 0x775e30
#pragma bss_seg(".bss$ge11cc4")
float g_endR = RandRange(200, 250);

// 0x775ea0
#pragma bss_seg(".bss$ge11cc0")
float g_endG = RandRange(200, 250);

// 0x775f10
#pragma bss_seg(".bss$ge11cc8")
float g_endB = RandRange(200, 250);
#pragma bss_seg()
