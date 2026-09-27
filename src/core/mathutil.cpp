// mathutil.cpp: Maths helpers: sin/cos tables, CRT wrappers (sin, cos, sqrt, fabs), screen clamps,
// the score sanity clamp.
#include <math.h>
#include "globals.h"
#include "game.h"


// Fills g_sinTable/g_cosTable with sin/cos for 0..359.9 degrees in 0.1-degree steps
// (3600 entries), used elsewhere as a fast lookup instead of calling sin/cos.
void InitTrigTables()
{
    double a = 0;
    int i;
    for (i = 0; i < 3600; i++) {
        g_sinTable[i] = sin(a * 0.017453292519968373);
        g_cosTable[i] = cos(a * 0.017453292519968373);
        a += 0.1;
    }
}

// Sanitizes a timestamp t against g_scoreBefore: replaces a negative t with
// g_scoreBefore, and rejects (-1) a t more than 250,000,000 (ms) ahead of it.
__int64 ClampScore(__int64 t)
{
    if (t < 0)
        t = g_scoreBefore;
    else if (t - g_scoreBefore > 250000000)
        t = -1;
    return t;
}

// Absolute value used by window code; just forwards to FabsF.
float FabsWindow(float x)
{
    return FabsF(x);
}

// Absolute value via the CRT double fabs(), rounded back to float.
float FabsF(float x)
{
    return (float)fabs((double)x);
}

// Thin wrapper around Cos(), kept as a separate call site for callers that were compiled
// against it.
float Cos2(float a)
{
    return Cos(a);
}

// Cosine of `a` (radians), via the double-precision CRT function.
float Cos(float a)
{
    return (float)cos((double)a);
}

// Thin wrapper around Sin(), kept as a separate call site for callers that were compiled
// against it.
float Sin2(float a)
{
    return Sin(a);
}

// Sine of `a` (radians), via the double-precision CRT function.
float Sin(float a)
{
    return (float)sin((double)a);
}

// Thin wrapper around Sqrtf(), kept as a separate call site for callers that were
// compiled against it.
float Sqrt(float x)
{
    return Sqrtf(x);
}

// Square root of `x`, via the double-precision CRT function.
float Sqrtf(float x)
{
    return (float)sqrt((double)x);
}

// Thin wrapper around FabsF, used only by UpdateExplosionDebris's speed check below.
float FabsExplosion(float x)
{
    return FabsF(x);
}

// Precomputes sine/cosine lookup tables at 0.1-degree steps (3600 entries covering
// a full circle).
void BuildSinCos()
{
    double a = 0;
    int i;

    for (i = 0; i < 3600; i++) {
        g_sinTableFine[i] = sin(a * 0.017453292519968373);
        g_cosTableFine[i] = cos(a * 0.017453292519968373);
        a += 0.1;
    }
}

// Clamps x to the visible screen width [0, g_screenW-1].
int ClampX(int x)
{
    if (x < 0)
        x = 0;
    if (x > (int)(g_screenW - 1))
        x = g_screenW - 1;
    return x;
}

// Clamps y to the visible screen height [0, g_screenH-1].
int ClampY(int y)
{
    if (y < 0)
        y = 0;
    if (y > (int)(g_screenH - 1))
        y = g_screenH - 1;
    return y;
}
