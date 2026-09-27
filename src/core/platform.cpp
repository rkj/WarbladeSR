// platform.cpp: Win32/DirectDraw layer: message pump, surfaces and colour keys, memory status,
// clocks and timers, the log, cursor clipping, screenshots.
#include <stdio.h>
#include <io.h>
#include "globals.h"
#include "game.h"


// Creates the user's Warblade folder if it doesn't exist.
void MakeGameDir()
{
    char path[512];
    sprintf(path, "%s\\warblade", KMiscTools::getUserFolder());
    bool exists = GetFileAttributesA(path) != 0xffffffff;
    if (!exists)
        CreateDirectoryA(path, 0);
}

// Loads NViewLib.dll (an optional third-party NVIDIA stereoscopic-3D driver) and resolves its
// NViewLibLoad entry point, if present.
void NViewLibInit()
{
    g_nviewLib = LoadLibraryA("NViewLib.dll");
    if (g_nviewLib != 0)
        g_nviewLibLoad = GetProcAddress(g_nviewLib, "NViewLibLoad");
}

// Releases NViewLib.dll if it was loaded.
void NViewLibFree()
{
    if (g_nviewLib != 0) {
        FreeLibrary(g_nviewLib);
        g_nviewLib = 0;
    }
}

// Confines the mouse cursor to the window, in fullscreen only.
void ClipCursorOn()
{
    if (!g_windowed)
        KInput::clipPointer(true);
}

// Releases the cursor clip, in fullscreen only.
void ClipCursorOff()
{
    if (!g_windowed)
        ClipCursor(NULL);
}

// Always-true stub for a cursor-clip related callback.
int CursorClipStubA()
{
    return 1;
}

// Always-true stub for a cursor-clip related callback.
int CursorClipStubB()
{
    return 1;
}

// Current UTC month (1-12).
int CurMonth()
{
    SYSTEMTIME time;
    memset(&time, 0, sizeof(time));
    GetSystemTime(&time);
    return time.wMonth;
}

// Current UTC year.
int CurYear()
{
    SYSTEMTIME time;
    memset(&time, 0, sizeof(time));
    GetSystemTime(&time);
    return time.wYear;
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
    sprintf(filename, "%s\\warblade\\screenshots", KMiscTools::getUserFolder());
    if (GetFileAttributesA(filename) == -1) {
        if (!CreateDirectoryA(filename, 0))
            ok = false;
    }
    if (ok) {
        ok = false;
        do {
            sprintf(filename, "%s\\warblade\\screenshots\\ScreenShot%03d.jpg",
                    KMiscTools::getUserFolder(), count);
            g_fileModeFlag = 0x8000;
            fd = _open(filename, 0, 0);
            if (fd != -1) {
                count++;
                _close(fd);
            } else {
                ok = true;
            }
        } while (!ok);
        if (ok)
            g_window->saveBackBuffer(filename, 1, 800, 600);
    }
}

// Clips (x, y, w, h) to the clip rect and blits directly via DirectDraw's BltFast().
// Retries on DDERR_SURFACELOST (0x887601C2) after a successful RestoreSurfaces(), and on
// DDERR_WASSTILLDRAWING (0x8876021C); any other result (including success) returns.
void BltFastClipped(int x, int y, DDSurface *dst, DDSurface *srcSurf, int sx, int sy, int w, int h)
{
    long hr;
    Rect16 src;
    int dx;
    int dy;

    while (1) {
        dx = x;
        dy = y;
        src.x1 = sx;
        src.y1 = sy;
        if (dx >= g_clipRight || dy >= g_clipBottom ||
            dx + w <= g_clipLeft || dy + h <= g_clipTop)
            return;

        if (dx < g_clipLeft) {
            src.x1 = g_clipLeft - dx + src.x1;
            w = w - (g_clipLeft - dx);
            dx = g_clipLeft;
        } else if (dx + w >= g_clipRight) {
            w = g_clipRight - dx;
        }
        if (dy < g_clipTop) {
            src.y1 = g_clipTop - dy + src.y1;
            h = h - (g_clipTop - dy);
            dy = g_clipTop;
        } else if (dy + h >= g_clipBottom) {
            h = g_clipBottom - dy;
        }
        src.x2 = src.x1 + w;
        src.y2 = src.y1 + h;

        // Retry the blit if the surface was lost and could be restored; give up otherwise.
        hr = dst->BltFast(dx, dy, srcSurf, &src, g_bltFastFlags);
        if (hr == 0)
            return;
        if (hr == 0x887601C2 && RestoreSurfaces() == 0)
            return;
        if (hr != 0x8876021C)
            return;
    }
}

// Drains the Windows message queue without blocking so the app stays responsive.
void PumpMessages()
{
    MSG message;

    while (PeekMessageA(&message, 0, 0, 0x7D00, 1))  // 1 = PM_REMOVE
        DispatchMessageA(&message);
}

// DirectDraw surface-restore callback; always reports success.
int RestoreSurfaces()
{
    return 1;
}

// Returns the current UTC day-of-month combined with time-of-day, as a seconds count
// (day*86400 + hour*3600 + minute*60 + second). A coarse timestamp, not a true epoch.
int GetDaySeconds()
{
    int secs;
    memset(&g_sysTime, 0, sizeof(g_sysTime));
    GetSystemTime(&g_sysTime);
    secs = g_sysTime.wDay * 86400 + g_sysTime.wHour * 3600
         + g_sysTime.wMinute * 60 + g_sysTime.wSecond;
    return secs;
}

// Creates (or truncates) the debug log file and writes its header line.
void LogInit()
{
    char tit[] = "************ Warblade Debug information ************\r\n";
    int fh;
    int written;
    char path[512];
    g_fileModeFlag = 0x8000;
    sprintf(path, "%s\\warblade\\warblade.dbg", KMiscTools::getUserFolder());
    fh = _open(path, 0x302, 0x180);  // O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, mode 0600
    if (fh != -1) {
        written = _write(fh, tit, strlen(tit));
        if (written == -1)
            MessageBoxA(g_hwnd, "Could not open/create Warblade debug info",
                        "Warblade v1.31, (C) 1999-2008 Edgar M Vigdal", 0x10);
        _close(fh);
    }
}

// Appends s to the debug log file.
void LogPrint(const char *s)
{
    int fh;
    int written;
    char path[512];
    g_fileModeFlag = 0x8000;
    sprintf(path, "%s\\warblade\\warblade.dbg", KMiscTools::getUserFolder());
    fh = _open(path, 0xa, 0x180);  // O_WRONLY|O_APPEND, mode 0600
    if (fh != -1) {
        written = _write(fh, s, strlen(s));
        if (written == -1)
            MessageBoxA(g_hwnd, "Could not open/create WarBlade debug info",
                        "Warblade v1.31, (C) 1999-2008 Edgar M Vigdal", 0x10);
        _close(fh);
    }
}

// Returns available physical memory in bytes.
DWORD GetMemAvailPhys()
{
    MEMORYSTATUS MemoryStatus;
    memset(&MemoryStatus, sizeof(MemoryStatus), 0);
    GlobalMemoryStatus(&MemoryStatus);
    return MemoryStatus.dwAvailPhys;
}

// Returns total physical memory in bytes.
DWORD GetMemTotalPhys()
{
    MEMORYSTATUS MemoryStatus;
    memset(&MemoryStatus, sizeof(MemoryStatus), 0);
    MemoryStatus.dwLength = sizeof(MemoryStatus);
    GlobalMemoryStatus(&MemoryStatus);
    return MemoryStatus.dwTotalPhys;
}

// Returns available virtual address space in bytes.
DWORD GetMemAvailVirtual()
{
    MEMORYSTATUS MemoryStatus;
    memset(&MemoryStatus, sizeof(MemoryStatus), 0);
    MemoryStatus.dwLength = sizeof(MemoryStatus);
    GlobalMemoryStatus(&MemoryStatus);
    return MemoryStatus.dwAvailVirtual;
}

// Returns total virtual address space in bytes.
DWORD GetMemTotalVirtual()
{
    MEMORYSTATUS MemoryStatus;
    memset(&MemoryStatus, sizeof(MemoryStatus), 0);
    MemoryStatus.dwLength = sizeof(MemoryStatus);
    GlobalMemoryStatus(&MemoryStatus);
    return MemoryStatus.dwTotalVirtual;
}

// Returns the system memory load as a percentage (0-100).
DWORD GetMemMemoryLoad()
{
    MEMORYSTATUS MemoryStatus;
    memset(&MemoryStatus, sizeof(MemoryStatus), 0);
    MemoryStatus.dwLength = sizeof(MemoryStatus);
    GlobalMemoryStatus(&MemoryStatus);
    return MemoryStatus.dwMemoryLoad;
}

// Records the current system time into g_timeA.
void StampTimeA()
{
    GetSystemTimeAsFileTime((FILETIME *)&g_timeA);
}

// Records the current system time into g_timeMarkB.
void StampTimeB()
{
    GetSystemTimeAsFileTime(&g_timeMarkB);
}

// Records the current system time into g_timeMarkC.
void StampTimeC()
{
    GetSystemTimeAsFileTime((FILETIME *)&g_timeMarkC);
}

// Records the current system time into g_timeD.
void StampTimeD()
{
    GetSystemTimeAsFileTime(&g_timeD);
}

// Records the current system time into g_timeE, but only once g_timeD has been stamped.
void StampTimeE()
{
    if (g_timeD.dwHighDateTime != 0 && g_timeD.dwLowDateTime != 0)
        GetSystemTimeAsFileTime(&g_timeE);
}

// Marks the start of timer 1.
void TimerStart1()
{
    GetSystemTimeAsFileTime(&g_timerStart1);
}

// Marks the end of timer 1 and updates g_timerMin1 with the elapsed time if it is a
// new minimum.
void TimerStop1()
{
    LARGE_INTEGER fra;
    LARGE_INTEGER til;
    LARGE_INTEGER tid;
    GetSystemTimeAsFileTime(&g_timerEnd1);
    fra.LowPart = g_timerStart1.dwLowDateTime;
    fra.HighPart = g_timerStart1.dwHighDateTime;
    til.LowPart = g_timerEnd1.dwLowDateTime;
    til.HighPart = g_timerEnd1.dwHighDateTime;
    tid.QuadPart = til.QuadPart - fra.QuadPart;
    if (tid.QuadPart < g_timerMin1)
        g_timerMin1 = tid.QuadPart;
}

// Marks the start of timer 2.
void TimerStart2()
{
    GetSystemTimeAsFileTime(&g_timerStart2);
}

// Marks the end of timer 2 and updates g_timerMin2 with the elapsed time if it is a
// new minimum.
void TimerStop2()
{
    LARGE_INTEGER fra;
    LARGE_INTEGER til;
    LARGE_INTEGER tid;
    GetSystemTimeAsFileTime(&g_timerEnd2);
    fra.LowPart = g_timerStart2.dwLowDateTime;
    fra.HighPart = g_timerStart2.dwHighDateTime;
    til.LowPart = g_timerEnd2.dwLowDateTime;
    til.HighPart = g_timerEnd2.dwHighDateTime;
    tid.QuadPart = til.QuadPart - fra.QuadPart;
    if (tid.QuadPart < g_timerMin2)
        g_timerMin2 = tid.QuadPart;
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

// Determines the raw pixel value DirectDraw uses for rgb on surface pdds: paints the
// color into the surface's top-left pixel via a GDI DC, reads back the raw pixel bits,
// then restores the original pixel. Masks the result to the surface's bit depth.
DWORD DDColorMatch(IDirectDrawSurface7 *pdds, COLORREF rgb)
{
    COLORREF rgbT;
    HDC hdc;
    DWORD dw = CLR_INVALID;
    DDSURFACEDESC2 ddsd;
    HRESULT hres;

    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        rgbT = GetPixel(hdc, 0, 0);
        SetPixel(hdc, 0, 0, rgb);
        pdds->ReleaseDC(hdc);
    }
    ddsd.dwSize = sizeof(ddsd);
    while ((hres = pdds->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
        ;
    if (hres == DD_OK)
    {
        dw = *(DWORD *)ddsd.lpSurface;
        if (ddsd.ddpfPixelFormat.dwRGBBitCount < 32)
            dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount) - 1;
        pdds->Unlock(NULL);
    }
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        SetPixel(hdc, 0, 0, rgbT);
        pdds->ReleaseDC(hdc);
    }
    return dw;
}

// Sets pdds's source blit color key to the raw pixel value matching rgb.
HRESULT DDSetColorKey(IDirectDrawSurface7 *pdds, COLORREF rgb)
{
    DDCOLORKEY ddck;

    ddck.dwColorSpaceLowValue = DDColorMatch(pdds, rgb);
    ddck.dwColorSpaceHighValue = ddck.dwColorSpaceLowValue;
    return pdds->SetColorKey(DDCKEY_SRCBLT, &ddck);
}

// Reads total/free video memory from pdd's device caps into g_vidMemTotal/g_vidMemFree.
void GetVidMem(IDirectDraw7 *pdd)
{
    DDCAPS ddcaps;

    g_vidMemTotal = 0;
    g_vidMemFree = 0;
    memset(&ddcaps, 0, sizeof(ddcaps));
    ddcaps.dwSize = sizeof(ddcaps);
    pdd->GetCaps(&ddcaps, NULL);
    g_vidMemTotal = ddcaps.dwVidMemTotal;
    g_vidMemFree = ddcaps.dwVidMemFree;
}
