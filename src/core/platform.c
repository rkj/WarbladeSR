// platform.c: Game-side platform helpers: the debug log, screenshots, timers, cursor
// clipping.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include "globals.h"
#include "game.h"


// Creates the user's Warblade folder if it doesn't exist.
void MakeGameDir()
{
    char path[512];
    sprintf(path, "%s\\warblade", SysUserFolder());
    if (!SysFileExists(path))
        SysMakeDir(path);
}

// Confines the mouse cursor to the window, in fullscreen only.
void ClipCursorOn()
{
    if (!g_windowed)
        ClipPointer(true);
}

// Releases the cursor clip, in fullscreen only.
void ClipCursorOff()
{
    if (!g_windowed)
        ClipPointer(false);
}

// Current UTC month (1-12).
int CurMonth()
{
    SysDate date;
    memset(&date, 0, sizeof(date));
    SysUtcDate(&date);
    return date.month;
}

// Current UTC year.
int CurYear()
{
    SysDate date;
    memset(&date, 0, sizeof(date));
    SysUtcDate(&date);
    return date.year;
}

// Saves the current back buffer as a JPEG under .../warblade/screenshots, bound to F7.
// Creates the screenshots folder if needed and picks the first unused "ScreenShotNNN.jpg" name.
void TakeScreenshot()
{
    int count;
    int fd;
    char filename[512];
    bool ok;

    count = 1;
    ok = true;
    sprintf(filename, "%s\\warblade\\screenshots", SysUserFolder());
    if (!SysFileExists(filename)) {
        if (!SysMakeDir(filename))
            ok = false;
    }
    if (ok) {
        ok = false;
        do {
            sprintf(filename, "%s\\warblade\\screenshots\\ScreenShot%03d.jpg",
                    SysUserFolder(), count);
            _set_fmode(_O_BINARY);
            fd = _open(filename, 0, 0);
            if (fd != -1) {
                count++;
                _close(fd);
            } else {
                ok = true;
            }
        } while (!ok);
        if (ok)
            SysScreenshot(filename, 800, 600);
    }
}

// Returns the current UTC day-of-month combined with time-of-day, as a seconds count
// (day*86400 + hour*3600 + minute*60 + second). A coarse timestamp, not a true epoch.
int GetDaySeconds()
{
    int secs;
    memset(&g_sysTime, 0, sizeof(g_sysTime));
    SysUtcDate(&g_sysTime);
    secs = g_sysTime.day * 86400 + g_sysTime.hour * 3600
         + g_sysTime.minute * 60 + g_sysTime.second;
    return secs;
}

// Creates (or truncates) the debug log file and writes its header line.
void LogInit()
{
    char tit[] = "************ Warblade Debug information ************\r\n";
    int fh;
    int written;
    char path[512];
    _set_fmode(_O_BINARY);
    sprintf(path, "%s\\warblade\\warblade.dbg", SysUserFolder());
    fh = _open(path, 0x302, 0x180);  // O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, mode 0600
    if (fh != -1) {
        written = _write(fh, tit, (unsigned int)strlen(tit));
        if (written == -1)
            SysMessageBox("Warblade v1.31, (C) 1999-2008 Edgar M Vigdal",
                          "Could not open/create Warblade debug info");
        _close(fh);
    }
}

// Appends s to the debug log file.
void LogPrint(const char *s)
{
    int fh;
    int written;
    char path[512];
    _set_fmode(_O_BINARY);
    sprintf(path, "%s\\warblade\\warblade.dbg", SysUserFolder());
    fh = _open(path, 0xa, 0x180);  // O_WRONLY|O_APPEND, mode 0600
    if (fh != -1) {
        written = _write(fh, s, (unsigned int)strlen(s));
        if (written == -1)
            SysMessageBox("Warblade v1.31, (C) 1999-2008 Edgar M Vigdal",
                          "Could not open/create WarBlade debug info");
        _close(fh);
    }
}

// Records the current system time into g_timeA.
void StampTimeA()
{
    g_timeA = SysFileTimeNow();
}

// Records the current system time into g_timeMarkB.
void StampTimeB()
{
    g_timeMarkB = SysFileTimeNow();
}

// Records the current system time into g_timeMarkC.
void StampTimeC()
{
    g_timeMarkC = SysFileTimeNow();
}

// Records the current system time into g_timeD.
void StampTimeD()
{
    g_timeD = SysFileTimeNow();
}

// Records the current system time into g_timeE, but only once g_timeD has been stamped.
void StampTimeE()
{
    // NOTE: the original checked both halves of the 64-bit timestamp for non-zero; any
    // real date has a non-zero high half, so a plain non-zero check is equivalent (it is
    // really just "has StampTimeD ever run").
    if (g_timeD != 0)
        g_timeE = SysFileTimeNow();
}

// Marks the start of timer 1.
void TimerStart1()
{
    g_timerStart1 = SysFileTimeNow();
}

// Marks the end of timer 1 and updates g_timerMin1 with the elapsed time if it is a
// new minimum.
void TimerStop1()
{
    __int64 elapsed;
    g_timerEnd1 = SysFileTimeNow();
    elapsed = g_timerEnd1 - g_timerStart1;
    if (elapsed < g_timerMin1)
        g_timerMin1 = elapsed;
}

// Marks the start of timer 2.
void TimerStart2()
{
    g_timerStart2 = SysFileTimeNow();
}

// Marks the end of timer 2 and updates g_timerMin2 with the elapsed time if it is a
// new minimum.
void TimerStop2()
{
    __int64 elapsed;
    g_timerEnd2 = SysFileTimeNow();
    elapsed = g_timerEnd2 - g_timerStart2;
    if (elapsed < g_timerMin2)
        g_timerMin2 = elapsed;
}

// Resets timer 1's recorded minimum to a large sentinel value.
void TimerReset1()
{
    g_timerMin1 = 999999999;
}

// Resets timer 2's recorded minimum to a large sentinel value.
void TimerReset2()
{
    g_timerMin2 = 999999999;
}
