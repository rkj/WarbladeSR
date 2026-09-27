// static_init.c: Game globals the original initialised before main (C++ dynamic initializers:
// floor line, blink rate, end colours). C needs constant initializers, so the two that were
// computed from other globals' initial values are written out, and the random end colours are
// rolled by InitStaticGlobals() at the start of GameMain.
#include "globals.h"
#include "game.h"


// Y coordinate of the ground line players stand/land on: fixed offset above the bottom of the
// screen (g_screenH's initial 600 - 50).
#pragma bss_seg(".bss$ge11ccc")
int g_floorY = 600 - 50;

// Blink interval (ms) for the "saved" UI message; doubled temporarily elsewhere for emphasis,
// then reset back to g_blinkRate (whose initial value it copies).
#pragma bss_seg(".bss$ge11cd0")
unsigned int g_saveMsgBlinkRate = 450;

// Random end-sequence background colour (R/G/B, each rolled independently), 200-250 out of 255.
#pragma bss_seg(".bss$ge11cc4")
float g_endR;

#pragma bss_seg(".bss$ge11cc0")
float g_endG;

#pragma bss_seg(".bss$ge11cc8")
float g_endB;
#pragma bss_seg()

// Rolls the end colours, as the original's initializers did before main (in this order, on the
// random generator's initial state).
void InitStaticGlobals(void)
{
    g_endR = RandRange(200, 250);
    g_endG = RandRange(200, 250);
    g_endB = RandRange(200, 250);
}
