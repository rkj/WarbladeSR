// stubs.cpp: Context-free no-op and constant callbacks used as function-pointer defaults.
#include "globals.h"
#include "game.h"


// Always returns true; used as a stub predicate/callback.
bool ReturnTrueNet()
{
    return true;
}

// No-op; used as a placeholder callback.
void DoNothing()
{
}

// No-op placeholder used as the default state/view-transition callback (screen change hook
// with nothing extra to do).
void EmptyPostTransitionHook()
{
}

// No-op placeholder used as the default view-change callback.
void EmptyViewChangeHook()
{
}

// Draw calls made during update are appended to one of these queues (as small structs,
// not run immediately) so the whole frame's drawing happens together during the flush
// pass; each queue has a matching FlushX() below that empties it into real KGraphic calls.

// Always returns 0; used as a stub predicate/callback.
int EmptyEscGateCheck()
{
    int result = 0;
    return result;
}

// Constant-zero stub, used as a callback/function-pointer default.
int ReturnZero()
{
    return 0;
}

// No-op stub, used as a callback/function-pointer default.
void NoOpEffects()
{
}
