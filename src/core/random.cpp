// random.cpp: Random numbers: the seeded xorshift generator, ranges, random ids.
#include "globals.h"
#include "game.h"


// Seeds the CRT RNG and the xorshift generator's state (g_rngX/Y/Z) from it.
// NOTE: g_rngT and g_rngW (also used by XorShift) are left at whatever they held before.
void SeedRand(unsigned int seed)
{
    srand(seed);
    g_rngX = rand();
    g_rngY = rand() * g_rngX;
    g_rngZ = rand() * g_rngX + g_rngY;
}

// Returns a random int in [lo, hi), or 0 if the range is empty.
int RandRange(int lo, int hi)
{
    if (hi - lo == 0)
        return 0;
    return XorShift() % (hi - lo) + lo;
}

// Advances the xorshift RNG state (g_rngT/X/Y/Z/W) and returns the next 32-bit value.
unsigned int XorShift()
{
    g_rngT -= (g_rngX << 11) ^ g_rngX;
    g_rngX = g_rngY;
    g_rngY = g_rngZ;
    g_rngZ = g_rngW;
    g_rngW = (g_rngW >> 19) ^ g_rngW ^ ((g_rngT >> 8) ^ g_rngT);
    return g_rngW;
}

// Returns a random float in [lo, hi) using XorShift(); the constant is 1/2^32.
float RandFloat(float lo, float hi)
{
    return (hi - lo) * XorShift() * 2.3283064365386963e-10 + lo;
}

// Generates a pseudo-random 64-bit save id (0 < id <= 9999999999999) by repeatedly
// perturbing an accumulator seeded from the mouse position and local time.
__int64 MakeRandomId()
{
    int n = 1024;
    __int64 v;
    SYSTEMTIME sy;
    int i;
    v = g_mouseX * g_mouseY * Rand1ff();
    GetLocalTime(&sy);
    v += sy.wMilliseconds * sy.wSecond - sy.wMinute;
    n += sy.wMilliseconds;
    do {
        for (i = 0; i < n; i++) {
            v += RandRange(0, 40000);
            if (Rand7f() < 1)
                v *= RandRange(0, 30000);
            v -= RandRange(0, 40000);
            if (Randff() < 3)
                v <<= 4;
            if (Rand1ff() < 5)
                v >>= 4;
        }
    } while (v < 0 || v == 0 || v > 9999999999999LL);
    return v;
}

// Returns a random value in [0, 0x7f].
int Rand7f()
{
    return XorShift() & 0x7f;
}

// Returns a random value in [0, 0xff].
int Randff()
{
    return XorShift() & 0xff;
}

// Returns a random value in [0, 0x1ff].
int Rand1ff()
{
    return XorShift() & 0x1ff;
}
