// sdl.c: The engine interface (include/sdlhelp.h) on SDL3 and SDL3_image: window, frame
// loop, time, files, the data archive, images and input. sdl_audio.c has the audio half.
//
// PTK's behaviour is kept where the game can see it (SDL_PLAN.md Step 4):
// - The game draws into an 800x600 back buffer (a render-target texture), which persists
//   between frames; SysFlip scales it into the window, letterboxed.
// - SysFlip caps the frame rate the way PTK did: at least 1000/fps whole milliseconds per
//   frame (16 ms, i.e. 62.5 fps, for 60), and it waits while the window is in the background.
//   With vsync on (the default) a 60 Hz display sets the pace instead.
// - On other displays SysFlip can interpolate: each tick's draws are recorded, matched to the
//   previous tick's and shown blended at the display's rate, while the game still ticks at
//   the rate above (see "Interpolation" below).
// - Blits, stretches and primitives follow KGraphicGL: blend factors per alpha mode, tint times
//   blend for the alpha, rotation (counter-clockwise, in degrees) about the destination centre
//   plus (centerX, centerY).
// - Key codes stay PTK's (they are stored in WarBlade.inf); letters, digits and punctuation
//   follow the keyboard layout, as Windows virtual keys did.
#include <string.h>
#ifndef _WIN32
#include <dirent.h>
#include <unistd.h>
#endif
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "sdlhelp.h"

// Audio slides that need the game thread (sdl_audio.c).
void AudioTick(void);

static SDL_Window   *s_window;
static SDL_Renderer *s_renderer;
static SDL_Texture  *s_canvas;       // the back buffer; always the render target between flips
static SDL_ScaleMode s_canvasScaleMode = SDL_SCALEMODE_PIXELART;
static int           s_width, s_height;
static bool          s_fullscreen;
static bool          s_quit;
static bool          s_focused = true;
static void        (*s_focusFn)(bool focused);
static int           s_fps = 60;         // SysSetMaxFps
static int           s_frameMs = 16;     // PTK: 1000 / fps, whole milliseconds; 0 = no cap
static Uint64        s_lastFlipNs;
static unsigned      s_frame;            // flips so far
static float         s_clearR, s_clearG, s_clearB, s_clearA = 1.0f;
static bool          s_vsync = true;
static int           s_interpMode = SYS_INTERP_AUTO;

// Interpolation state (see "Interpolation").
static SDL_Texture  *s_base;             // the back buffer as it was before this tick's draws
static SDL_Texture  *s_view;             // the picture at window resolution (RenderInterpolated)
static bool          s_recording;        // this tick's draws are being recorded
static Uint64        s_tickDueNs;        // when the tick being shown was due to start
static Uint64        s_nextFrameNs;      // without vsync: when the next interpolated frame is due

static void RecordFill(SDL_FRect rect, SDL_BlendMode blend, float r, float g, float b, float a);

// ---------------------------------------------------------------------------------------------
// Window, frame and events
// ---------------------------------------------------------------------------------------------

void SysInit(void)
{
    SDL_SetAppMetadata("Warblade", "1.34 SR1", "as.warblade.warblade");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD))
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
}

// The rectangle (window pixels) the back buffer is shown in: as large as fits, centred.
// SDL_GetRenderOutputSize, not SDL_GetCurrentRenderOutputSize: between flips the render target
// is the back buffer, whose size the latter would return.
static SDL_FRect OutputRect(void)
{
    SDL_FRect r;
    int pw = s_width, ph = s_height;
    float scale;

    SDL_GetRenderOutputSize(s_renderer, &pw, &ph);
    scale = SDL_min((float)pw / s_width, (float)ph / s_height);
    r.w = s_width * scale;
    r.h = s_height * scale;
    r.x = (pw - r.w) / 2;
    r.y = (ph - r.h) / 2;
    return r;
}

// Window (mouse) coordinates to back-buffer coordinates and back.
static void WindowToCanvas(float wx, float wy, float *cx, float *cy)
{
    int ww = 1, wh = 1, pw = 1, ph = 1;
    SDL_FRect r;

    SDL_GetWindowSize(s_window, &ww, &wh);
    SDL_GetWindowSizeInPixels(s_window, &pw, &ph);
    r = OutputRect();
    *cx = (wx * pw / ww - r.x) * s_width / r.w;
    *cy = (wy * ph / wh - r.y) * s_height / r.h;
}

static void CanvasToWindow(float cx, float cy, float *wx, float *wy)
{
    int ww = 1, wh = 1, pw = 1, ph = 1;
    SDL_FRect r;

    SDL_GetWindowSize(s_window, &ww, &wh);
    SDL_GetWindowSizeInPixels(s_window, &pw, &ph);
    r = OutputRect();
    *wx = (cx * r.w / s_width + r.x) * ww / pw;
    *wy = (cy * r.h / s_height + r.y) * wh / ph;
}

bool SysCreateWindow(int w, int h, bool windowed, const char *title, const char *renderDriver)
{
    SDL_WindowFlags flags;
    bool ok;

#ifdef __EMSCRIPTEN__
    // A page can only go fullscreen from a click or key press: start in the page (W and the
    // settings page still switch).
    windowed = true;
#endif
    flags = windowed ? SDL_WINDOW_RESIZABLE : SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN;
#ifdef __EMSCRIPTEN__
    // Keep CSS/input coordinates in logical pixels while presenting at the
    // display density. Otherwise thin bitmap strokes are lost in a low-resolution
    // canvas before a phone's browser enlarges it again.
    flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif

    SysDestroyWindow();
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, renderDriver);     // NULL clears it
    ok = SDL_CreateWindowAndRenderer(title, w, h, flags, &s_window, &s_renderer);
    SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
    if (!ok) {
        SDL_Log("SDL_CreateWindowAndRenderer (%s) failed: %s", renderDriver ? renderDriver : "auto",
                SDL_GetError());
        s_window = NULL;
        s_renderer = NULL;
        return false;
    }
    // Vsync presents on the vertical blank (no tearing); SysFlip's frame cap still sets the pace.
    SysSetVSync(s_vsync);
    s_canvas = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    s_base = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (s_canvas == NULL || s_base == NULL) {
        SDL_Log("SDL_CreateTexture (back buffer) failed: %s", SDL_GetError());
        SysDestroyWindow();
        return false;
    }
    SDL_SetTextureBlendMode(s_canvas, SDL_BLENDMODE_NONE);
    SDL_SetTextureBlendMode(s_base, SDL_BLENDMODE_NONE);
    // Pixel-art scaling keeps the 800x600 picture (and its bitmap fonts) sharp in large windows.
    s_canvasScaleMode = SDL_SCALEMODE_PIXELART;
    SDL_SetTextureScaleMode(s_canvas, s_canvasScaleMode);
    SDL_SetTextureScaleMode(s_base, SDL_SCALEMODE_PIXELART);
    s_recording = false;
    s_width = w;
    s_height = h;
    s_fullscreen = !windowed;
    s_quit = false;
    s_focused = true;
    SDL_SetRenderTarget(s_renderer, s_canvas);
    SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
    SDL_RenderClear(s_renderer);
    s_lastFlipNs = SDL_GetTicksNS();
    return true;
}

void SysDestroyWindow(void)
{
    s_recording = false;
    if (s_canvas) {
        SDL_DestroyTexture(s_canvas);
        s_canvas = NULL;
    }
    if (s_base) {
        SDL_DestroyTexture(s_base);
        s_base = NULL;
    }
    if (s_view) {
        SDL_DestroyTexture(s_view);
        s_view = NULL;
    }
    if (s_renderer) {
        SDL_DestroyRenderer(s_renderer);
        s_renderer = NULL;
    }
    if (s_window) {
        SDL_DestroyWindow(s_window);
        s_window = NULL;
    }
}

bool SysHasWindow(void)          { return s_window != NULL; }

const char *SysRendererName(void)
{
    const char *name = s_renderer ? SDL_GetRendererName(s_renderer) : NULL;
    return name ? name : "";
}
void SysTerminate(void)          { s_quit = true; }
bool SysQuitRequested(void)      { return s_quit; }
void SysMinimize(void)           { SDL_MinimizeWindow(s_window); }
bool SysHasFocus(void)           { return s_focused; }
void SysDisableScreenSaver(void) { SDL_DisableScreenSaver(); }

void SysSetFocusCallback(void (*fn)(bool focused))
{
    s_focusFn = fn;
}

void SysSetFullscreen(const bool fullscreen)
{
    if (s_fullscreen == fullscreen)
        return;

    SDL_SetWindowFullscreen(s_window, fullscreen);
    s_fullscreen = fullscreen;
}

static void PadAdded(SDL_JoystickID id);
static void PadRemoved(SDL_JoystickID id);

static void HandleEvent(const SDL_Event *e)
{
    switch (e->type) {
    case SDL_EVENT_GAMEPAD_ADDED:
        PadAdded(e->gdevice.which);
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        PadRemoved(e->gdevice.which);
        break;
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        s_quit = true;
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        s_focused = true;
        if (s_focusFn)
            s_focusFn(true);
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        s_focused = false;
        if (s_focusFn)
            s_focusFn(false);
        break;
    }
}

void SysProcessEvents(void)
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
        HandleEvent(&e);
    AudioTick();
}

// Like PTK, the game stands still while its window is in the background.
static void WaitForFocus(void)
{
    while (!s_focused && !s_quit) {
        SDL_Event e;
        if (SDL_WaitEventTimeout(&e, 100))
            HandleEvent(&e);
        AudioTick();
    }
}

// The refresh rate of the window's display in Hz, 0 if unknown.
static float RefreshRate(void)
{
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(s_window));
    return mode ? mode->refresh_rate : 0.0f;
}

// Whether frames are interpolated. Auto turns it off where it gains nothing: vsynced on a
// display whose refresh rate is the game's frame rate, where each tick gets one refresh.
static bool WantInterpolation(void)
{
    float hz;

#ifdef __EMSCRIPTEN__
    return false;   // the browser paces the page; frames are shown as they are made
#endif

    if (s_frameMs == 0 || s_base == NULL || s_interpMode == SYS_INTERP_OFF)
        return false;
    if (s_interpMode == SYS_INTERP_ON)
        return true;
    hz = RefreshRate();
    return !(s_vsync && hz > 0 && SDL_fabsf(hz - s_fps) < 0.5f);
}

static void FlipInterpolated(void);
static void StartRecording(bool fresh);

// Shows the back buffer as it is, then waits for the frame cap.
static void FlipPlain(void)
{
    SDL_FRect dst;
    Uint64 frameNs;
    Uint64 now;

    // Show the back buffer, letterboxed, and go back to drawing into it.
    SDL_SetRenderTarget(s_renderer, NULL);
    SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
    SDL_RenderClear(s_renderer);
    dst = OutputRect();
    // Average neighbouring pixels when shrinking, preserving coverage of thin
    // text strokes. Pixel-art magnification still keeps the original look.
    SDL_ScaleMode scaleMode = SDL_SCALEMODE_PIXELART;
#ifdef __EMSCRIPTEN__
    // WebGL supports filtered render-target minification. Keep the standalone
    // software renderer's presentation path unchanged; its linear target copy
    // can produce a black surface with the pinned SDL version.
    if (dst.w < s_width || dst.h < s_height)
        scaleMode = SDL_SCALEMODE_LINEAR;
#endif
    if (scaleMode != s_canvasScaleMode) {
        SDL_SetTextureScaleMode(s_canvas, scaleMode);
        s_canvasScaleMode = scaleMode;
    }
    SDL_RenderTexture(s_renderer, s_canvas, NULL, &dst);
    SDL_RenderPresent(s_renderer);
    SDL_SetRenderTarget(s_renderer, s_canvas);

    SysProcessEvents();

    // Frame cap: at least s_frameMs since the end of the previous flip.
    frameNs = (Uint64)s_frameMs * SDL_NS_PER_MS;
    now = SDL_GetTicksNS();
    if (frameNs && now - s_lastFlipNs < frameNs)
        SDL_DelayPrecise(frameNs - (now - s_lastFlipNs));
#ifdef __EMSCRIPTEN__
    else
        emscripten_sleep(0);   // the browser shows the frame (and runs events) only once we yield
#endif

    WaitForFocus();
    s_lastFlipNs = SDL_GetTicksNS();
    s_tickDueNs = s_lastFlipNs;         // the next tick starts now
}

void SysFlip(void)
{
    bool wasRecording = s_recording;

    s_frame++;
    // Interpolation needs this tick's draws, so it starts (or resumes) on the next tick.
    if (wasRecording && WantInterpolation())
        FlipInterpolated();
    else
        FlipPlain();

    s_recording = WantInterpolation();
    if (s_recording)
        StartRecording(!wasRecording);
}

void SysSetMaxFps(int fps)
{
    s_fps = fps;
    s_frameMs = fps > 0 ? 1000 / fps : 0;
}

void SysSetVSync(bool on)
{
    s_vsync = on;
    if (s_renderer && !SDL_SetRenderVSync(s_renderer, on ? 1 : SDL_RENDERER_VSYNC_DISABLED))
        SDL_Log("SDL_SetRenderVSync failed: %s", SDL_GetError());
}

void SysSetInterpolation(int mode)
{
    s_interpMode = mode >= 0 && mode < SYS_INTERP_COUNT ? mode : SYS_INTERP_AUTO;
}

bool SysInterpolating(void)
{
    return WantInterpolation();
}

void SysSetClearColor(float r, float g, float b, float a)
{
    s_clearR = r;
    s_clearG = g;
    s_clearB = b;
    s_clearA = a;
}

// The game only ever sets the identity view; `clear` clears the back buffer.
void SysSetWorldView(float x, float y, float rotation, float zoom, bool clear)
{
    if (x != 0 || y != 0 || rotation != 0 || zoom != 1.0f)
        SDL_Log("SysSetWorldView: only the identity view is supported (%g %g %g %g)", x, y, rotation, zoom);
    if (clear) {
        SDL_FRect all = { 0, 0, (float)s_width, (float)s_height };
        SDL_SetRenderDrawBlendMode(s_renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColorFloat(s_renderer, s_clearR, s_clearG, s_clearB, 1.0f);
        SDL_RenderClear(s_renderer);
        RecordFill(all, SDL_BLENDMODE_NONE, s_clearR, s_clearG, s_clearB, 1.0f);
    }
}

void SysSetIcon(const char *icoFile)
{
    SDL_Surface *icon = IMG_Load(SysPath(icoFile));
    if (icon) {
        SDL_SetWindowIcon(s_window, icon);
        SDL_DestroySurface(icon);
    }
}

bool SysScreenshot(const char *file, int w, int h)
{
    SDL_Surface *shot = SDL_RenderReadPixels(s_renderer, NULL);
    bool ok;

    if (shot == NULL)
        return false;
    if (shot->w != w || shot->h != h) {
        SDL_Surface *scaled = SDL_ScaleSurface(shot, w, h, SDL_SCALEMODE_LINEAR);
        SDL_DestroySurface(shot);
        shot = scaled;
        if (shot == NULL)
            return false;
    }
    ok = IMG_SaveJPG(shot, SysPath(file), 90);
    SDL_DestroySurface(shot);
    return ok;
}

static SDL_Rect UsableDesktop(void)
{
    SDL_Rect r = { 0, 0, 0, 0 };
    SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &r);
    return r;
}

int SysDesktopWidth(void)  { return UsableDesktop().w; }
int SysDesktopHeight(void) { return UsableDesktop().h; }

void SysMessageBox(const char *title, const char *text)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, text, s_window);
}

// ---------------------------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------------------------

// The game subtracts up to a few minutes from this clock, so it must not start near 0
// (timeGetTime counted from boot).
#define MILLIS_OFFSET 86400000u

unsigned SysMillis(void)
{
    return (unsigned)SDL_GetTicks() + MILLIS_OFFSET;
}

long long SysPerfCounter(void) { return (long long)SDL_GetPerformanceCounter(); }
long long SysPerfFreq(void)    { return (long long)SDL_GetPerformanceFrequency(); }

// FILETIME counts 100 ns units from 1601-01-01; SDL_Time counts ns from 1970-01-01.
#define FILETIME_1970 116444736000000000LL

long long SysFileTimeNow(void)
{
    SDL_Time t = 0;
    SDL_GetCurrentTime(&t);
    return t / 100 + FILETIME_1970;
}

static void ToSysDate(SDL_Time t, bool local, SysDate *out)
{
    SDL_DateTime dt;

    memset(out, 0, sizeof(*out));
    if (!SDL_TimeToDateTime(t, &dt, local))
        return;
    out->year = (unsigned short)dt.year;
    out->month = (unsigned short)dt.month;
    out->dayOfWeek = (unsigned short)dt.day_of_week;
    out->day = (unsigned short)dt.day;
    out->hour = (unsigned short)dt.hour;
    out->minute = (unsigned short)dt.minute;
    out->second = (unsigned short)dt.second;
    out->milliseconds = (unsigned short)(dt.nanosecond / 1000000);
}

// Worked out here rather than through SDL_Time: the game also converts durations (FILETIMEs
// near 1601), which are outside SDL_Time's range of about 1678-2262.
void SysFileTimeToDate(long long ft, SysDate *out)
{
    long long secs, days, z, era, doe, yoe, doy, mp, y;
    long long rem;

    memset(out, 0, sizeof(*out));
    if (ft < 0)
        return;
    secs = ft / 10000000;
    days = secs / 86400;                    // since 1601-01-01, a Monday
    rem = secs % 86400;
    out->dayOfWeek = (unsigned short)((days + 1) % 7);
    out->hour = (unsigned short)(rem / 3600);
    out->minute = (unsigned short)(rem / 60 % 60);
    out->second = (unsigned short)(rem % 60);
    out->milliseconds = (unsigned short)(ft / 10000 % 1000);

    // Civil date from a day count (H. Hinnant's days_from_civil, inverted), from 0000-03-01.
    z = days + 584694;                      // 1601-01-01 is day 584694 after 0000-03-01
    era = z / 146097;
    doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    out->day = (unsigned short)(doy - (153 * mp + 2) / 5 + 1);
    out->month = (unsigned short)(mp < 10 ? mp + 3 : mp - 9);
    out->year = (unsigned short)(out->month <= 2 ? y + 1 : y);
}

void SysUtcDate(SysDate *out)
{
    SDL_Time t = 0;
    SDL_GetCurrentTime(&t);
    ToSysDate(t, false, out);
}

void SysLocalDate(SysDate *out)
{
    SDL_Time t = 0;
    SDL_GetCurrentTime(&t);
    ToSysDate(t, true, out);
}

// ---------------------------------------------------------------------------------------------
// Files, folders, programs
// ---------------------------------------------------------------------------------------------

static void CopyPath(char *dst, size_t size, const char *src)
{
    size_t n;
    SDL_strlcpy(dst, src ? src : "", size);
    n = strlen(dst);
    while (n > 0 && (dst[n - 1] == '\\' || dst[n - 1] == '/'))
        dst[--n] = 0;
}

const char *SysUserFolder(void)
{
    static char folder[1024];
    if (folder[0] == 0) {
        const char *docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
#ifdef __EMSCRIPTEN__
        // The page keeps this folder in memory; durable saves belong to the server account.
        docs = "/save";
#elif !defined(_WIN32)
        // Not every Linux home has a Documents folder.
        if (docs == NULL || !SDL_GetPathInfo(docs, NULL))
            docs = SDL_GetUserFolder(SDL_FOLDER_HOME);
#endif
        CopyPath(folder, sizeof(folder), docs ? docs : SDL_GetBasePath());
    }
    return folder;
}

const char *SysAppPath(const char *rel)
{
    static char path[1024];
    const char *base = SDL_GetBasePath();
    SDL_snprintf(path, sizeof(path), "%s%s", base ? base : "", rel);
    return path;
}

#ifdef _WIN32
const char *SysPath(const char *path)
{
    return path;
}
#else
// Finds the entry of folder `dir` named `name` ignoring case, into `out`; false if none.
static bool FindNoCase(const char *dir, const char *name, char *out, size_t size)
{
    DIR *d = opendir(dir[0] ? dir : ".");
    struct dirent *e;
    bool found = false;

    if (d == NULL)
        return false;
    while ((e = readdir(d)) != NULL) {
        if (SDL_strcasecmp(e->d_name, name) == 0) {
            SDL_strlcpy(out, e->d_name, size);
            found = true;
            break;
        }
    }
    closedir(d);
    return found;
}

const char *SysPath(const char *path)
{
    static char bufs[4][1024];
    static int next;
    char *out = bufs[next];
    const size_t size = sizeof(bufs[0]);
    char name[256];
    char real[256];
    size_t len;
    const char *p = path;

    next = (next + 1) % 4;
    out[0] = 0;
    if (*p == '/' || *p == '\\')
        SDL_strlcpy(out, "/", size);
    while (*p) {
        size_t n;
        while (*p == '/' || *p == '\\')
            p++;
        for (n = 0; p[n] && p[n] != '/' && p[n] != '\\'; n++)
            ;
        if (n == 0)
            break;
        SDL_strlcpy(name, p, n + 1 < sizeof(name) ? n + 1 : sizeof(name));
        p += n;

        // `out` is the folder so far ("" for the current one); add this component to it,
        // spelled as on disk if only a different case exists.
        len = strlen(out);
        if (len > 0 && out[len - 1] != '/' && len + 1 < size)
            out[len++] = '/';
        out[len] = 0;
        SDL_strlcat(out, name, size);
        if (access(out, F_OK) != 0) {
            out[len] = 0;
            if (FindNoCase(out, name, real, sizeof(real)))
                SDL_strlcat(out, real, size);
            else
                SDL_strlcat(out, name, size);
        }
    }
    return out;
}
#endif

bool SysFileExists(const char *path)
{
    return SDL_GetPathInfo(SysPath(path), NULL);
}

bool SysMakeDir(const char *path)
{
    return SDL_CreateDirectory(SysPath(path));
}

void SysOpenUrl(const char *url)
{
    SDL_OpenURL(url);
}

// The original brought an already running jukebox to the front instead; a second instance
// starts here.
void SysOpenJukebox(void)
{
    char path[1024];
    const char *args[2];
    SDL_Process *proc;

    SDL_strlcpy(path, SysAppPath("Jukebox.exe"), sizeof(path));
    args[0] = path;
    args[1] = NULL;
    proc = SDL_CreateProcess(args, false);
    if (proc)
        SDL_DestroyProcess(proc);   // only the handle: the jukebox keeps running
    else
        SDL_Log("Could not start %s: %s", path, SDL_GetError());
}

// The data archive: a ustar file, indexed once. Names are looked up case-insensitively, as
// PTK's KResource did ("alien_10.tga" is "Alien_10.tga" in the archive).
typedef struct PacEntry {
    char name[256];
    Sint64 offset;
    Sint64 size;
} PacEntry;

static SDL_IOStream *s_pac;
static PacEntry     *s_pacEntries;
static int           s_pacCount;

static Sint64 OctalField(const unsigned char *p, int n)
{
    Sint64 v = 0;
    int i;
    for (i = 0; i < n && p[i] >= '0' && p[i] <= '7'; i++)
        v = v * 8 + (p[i] - '0');
    return v;
}

void PacAddArchive(const char *path)
{
    unsigned char hdr[512];
    Sint64 pos = 0;
    int cap = 0;

    s_pac = SDL_IOFromFile(SysPath(path), "rb");
    if (s_pac == NULL) {
        SDL_Log("Could not open %s: %s", path, SDL_GetError());
        return;
    }
    while (SDL_ReadIO(s_pac, hdr, 512) == 512 && hdr[0] != 0) {
        Sint64 size = OctalField(hdr + 124, 12);
        char type = (char)hdr[156];
        if (type == '0' || type == 0) {
            PacEntry *e;
            if (s_pacCount == cap) {
                cap = cap ? cap * 2 : 1024;
                s_pacEntries = (PacEntry *)SDL_realloc(s_pacEntries, cap * sizeof(PacEntry));
            }
            e = &s_pacEntries[s_pacCount++];
            if (memcmp(hdr + 257, "ustar", 5) == 0 && hdr[345] != 0)
                SDL_snprintf(e->name, sizeof(e->name), "%.155s/%.100s", (const char *)hdr + 345, (const char *)hdr);
            else
                SDL_snprintf(e->name, sizeof(e->name), "%.100s", (const char *)hdr);
            e->offset = pos + 512;
            e->size = size;
        }
        pos += 512 + (size + 511) / 512 * 512;
        if (SDL_SeekIO(s_pac, pos, SDL_IO_SEEK_SET) < 0)
            break;
    }
}

static const PacEntry *PacFind(const char *name)
{
    int i;
    for (i = 0; i < s_pacCount; i++) {
        if (SDL_strcasecmp(s_pacEntries[i].name, name) == 0)
            return &s_pacEntries[i];
    }
    return NULL;
}

// Reads a whole file, from the archive or else from the disk. SDL_free the result.
static void *PacLoad(const char *name, size_t *size)
{
    const PacEntry *e = PacFind(name);
    void *data;

    if (e == NULL)
        return SDL_LoadFile(SysPath(name), size);
    data = SDL_malloc((size_t)e->size + 1);
    if (data == NULL)
        return NULL;
    if (SDL_SeekIO(s_pac, e->offset, SDL_IO_SEEK_SET) < 0 ||
        SDL_ReadIO(s_pac, data, (size_t)e->size) != (size_t)e->size) {
        SDL_free(data);
        return NULL;
    }
    ((char *)data)[e->size] = 0;
    *size = (size_t)e->size;
    return data;
}

bool PacExists(const char *name)
{
    return PacFind(name) != NULL || SDL_GetPathInfo(SysPath(name), NULL);
}

// A file shorter than `size` fills only its own length of `buf`.
bool PacRead(const char *name, void *buf, unsigned size)
{
    size_t n = 0;
    void *data = PacLoad(name, &n);
    if (data == NULL)
        return false;
    memcpy(buf, data, n < size ? n : size);
    SDL_free(data);
    return true;
}

// ---------------------------------------------------------------------------------------------
// Images
// ---------------------------------------------------------------------------------------------

struct Image {
    SDL_Texture *tex;           // NULL after ImgFreePicture
    float w, h;
    bool freed;                 // ImgFree'd; the struct itself goes a few frames later
    unsigned freedFrame;
    Image *nextFreed;
    bool tinted;                // setBlitColor with anything but (1, 1, 1, 1)
    float r, g, b, a;
    short mode;                 // PTK alpha mode
    bool linear;                // setTextureQuality
};

static void RecordBlit(Image *img, SDL_FRect src, SDL_FRect dst, float angle, SDL_FPoint pivot,
                       SDL_FlipMode flip, float alpha);

// Freed images wait here until no render queue can still hold them (see ImgIsFreed).
static Image *s_freedImages;

static void ReleaseFreedImages(bool all)
{
    Image **p = &s_freedImages;
    while (*p) {
        Image *img = *p;
        if (all || s_frame - img->freedFrame > 2) {
            *p = img->nextFreed;
            SDL_free(img);
        } else {
            p = &img->nextFreed;
        }
    }
}

// KGraphicGL::setAlphaMode's blend factors (src, dst). The back buffer's alpha is kept at 1,
// so mode 0's DST_ALPHA is 1.
static SDL_BlendMode BlendModeFor(short mode)
{
    SDL_BlendFactor src = SDL_BLENDFACTOR_SRC_ALPHA;
    SDL_BlendFactor dst = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

    switch (mode) {
    case 0: src = SDL_BLENDFACTOR_SRC_ALPHA;           dst = SDL_BLENDFACTOR_DST_ALPHA; break;
    case 1: src = SDL_BLENDFACTOR_SRC_ALPHA;           dst = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA; break;
    case 2: src = SDL_BLENDFACTOR_ONE;                 dst = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA; break;
    case 3: src = SDL_BLENDFACTOR_ZERO;                dst = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA; break;
    case 4: src = SDL_BLENDFACTOR_SRC_ALPHA;           dst = SDL_BLENDFACTOR_ONE; break;
    case 5: src = SDL_BLENDFACTOR_ONE;                 dst = SDL_BLENDFACTOR_ZERO; break;
    case 6: src = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA; dst = SDL_BLENDFACTOR_ONE; break;
    }
    return SDL_ComposeCustomBlendMode(src, dst, SDL_BLENDOPERATION_ADD,
                                      SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
}

Image *ImgLoad(const char *file, bool hiDef, bool hasAlpha)
{
    size_t size = 0;
    void *data;
    const char *ext;
    SDL_Surface *surf;
    SDL_Texture *tex;
    Image *img;

    (void)hiDef;
    (void)hasAlpha;
    data = PacLoad(file, &size);
    if (data == NULL)
        return NULL;
    // TGA has no magic number, so the type comes from the extension.
    ext = strrchr(file, '.');
    surf = IMG_LoadTyped_IO(SDL_IOFromConstMem(data, size), true, ext ? ext + 1 : NULL);
    SDL_free(data);
    if (surf == NULL) {
        SDL_Log("Could not decode %s: %s", file, SDL_GetError());
        return NULL;
    }
    tex = SDL_CreateTextureFromSurface(s_renderer, surf);
    SDL_DestroySurface(surf);
    if (tex == NULL) {
        SDL_Log("Could not create a texture for %s: %s", file, SDL_GetError());
        return NULL;
    }
    img = (Image *)SDL_calloc(1, sizeof(Image));
    img->tex = tex;
    img->w = (float)tex->w;
    img->h = (float)tex->h;
    img->r = img->g = img->b = img->a = 1.0f;
    img->mode = 1;      // KGraphicGL's default
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    return img;
}

void ImgFreePicture(Image *img)
{
    if (img->tex) {
        SDL_DestroyTexture(img->tex);
        img->tex = NULL;
    }
}

void ImgFree(Image *img)
{
    if (img == NULL || img->freed)
        return;
    ImgFreePicture(img);
    img->freed = true;
    img->freedFrame = s_frame;
    img->nextFreed = s_freedImages;
    s_freedImages = img;
    ReleaseFreedImages(false);
}

bool  ImgIsFreed(const Image *img) { return img->freed; }
float ImgWidth(Image *img)         { return img->w; }
float ImgHeight(Image *img)        { return img->h; }

// Applied per blit (Draw), as the interpolated frames draw with their own scale modes.
void ImgSetTextureQuality(Image *img, bool quality)
{
    img->linear = quality;
}

void ImgSetAlphaMode(Image *img, short mode)
{
    if (mode >= 0 && mode <= 6)
        img->mode = mode;
}

// Components above 1 are clamped, as PTK did.
void ImgSetBlitColor(Image *img, float r, float g, float b, float a)
{
    img->r = SDL_min(r, 1.0f);
    img->g = SDL_min(g, 1.0f);
    img->b = SDL_min(b, 1.0f);
    img->a = SDL_min(a, 1.0f);
    img->tinted = !(r == 1.0f && g == 1.0f && b == 1.0f && a == 1.0f);
}

// Draws the source rect (sx1,sy1)-(sx2,sy2) into dst, rotated by `angle` degrees
// counter-clockwise about dst's corner + (px, py), with alpha `alpha`. A source rect given
// backwards (x2 < x1) is drawn mirrored, as in PTK.
static void Draw(Image *img, float sx1, float sy1, float sx2, float sy2, SDL_FRect dst,
                 float angle, float px, float py, bool flipX, bool flipY, float alpha)
{
    SDL_FRect src;
    SDL_FPoint pivot;
    int flip = SDL_FLIP_NONE;

    if (img == NULL || img->tex == NULL)
        return;
    if (sx2 < sx1) {
        float t = sx1; sx1 = sx2; sx2 = t;
        flipX = !flipX;
    }
    if (sy2 < sy1) {
        float t = sy1; sy1 = sy2; sy2 = t;
        flipY = !flipY;
    }
    if (dst.w < 0) {
        dst.x += dst.w;
        dst.w = -dst.w;
        px = dst.w - px;
        flipX = !flipX;
    }
    if (dst.h < 0) {
        dst.y += dst.h;
        dst.h = -dst.h;
        py = dst.h - py;
        flipY = !flipY;
    }
    src.x = sx1;
    src.y = sy1;
    src.w = sx2 - sx1;
    src.h = sy2 - sy1;
    pivot.x = px;
    pivot.y = py;
    if (flipX)
        flip |= SDL_FLIP_HORIZONTAL;
    if (flipY)
        flip |= SDL_FLIP_VERTICAL;

    alpha = SDL_clamp(alpha, 0.0f, 1.0f);
    if (img->tinted)
        SDL_SetTextureColorModFloat(img->tex,…6557 tokens truncated…else {
        SDL_SetWindowMouseRect(s_window, NULL);
    }
}

// Joysticks, reported like winmm's joyGetPosEx: axes 0-65535 (centre 32767), buttons a bit mask.
#define MAX_JOYS 2
static SDL_Joystick *s_joys[MAX_JOYS];

bool JoyEnable(char joy)
{
    int count = 0;
    SDL_JoystickID *ids;

    if (joy < 0 || joy >= MAX_JOYS)
        return false;
    if (s_joys[(int)joy])
        return true;
    ids = SDL_GetJoysticks(&count);
    if (ids && joy < count)
        s_joys[(int)joy] = SDL_OpenJoystick(ids[(int)joy]);
    SDL_free(ids);
    return s_joys[(int)joy] != NULL;
}

static long JoyAxis(char joy, int axis)
{
    SDL_Joystick *j = joy >= 0 && joy < MAX_JOYS ? s_joys[(int)joy] : NULL;
    if (j == NULL)
        return 0x7fff;
    if (axis >= SDL_GetNumJoystickAxes(j))
        return 0x7fff;
    RefreshInput();
    return (long)SDL_GetJoystickAxis(j, axis) + 32768;
}

long JoyX(char joy, char hat) { return JoyAxis(joy, hat == 0 ? 0 : 2); }
long JoyY(char joy, char hat) { return JoyAxis(joy, hat == 0 ? 1 : 3); }

bool JoyButton(char joy, long mask)
{
    SDL_Joystick *j = joy >= 0 && joy < MAX_JOYS ? s_joys[(int)joy] : NULL;
    unsigned long buttons = 0;
    int i, n;

    if (j == NULL)
        return false;
    RefreshInput();
    n = SDL_min(SDL_GetNumJoystickButtons(j), 32);
    for (i = 0; i < n; i++) {
        if (SDL_GetJoystickButton(j, i))
            buttons |= 1ul << i;
    }
    return (buttons & (unsigned long)mask) != 0;
}

// ---------------------------------------------------------------------------------------------
// Gamepads (sdlhelp.h): opened as they connect (SDL reports the ones present at start too).
// ---------------------------------------------------------------------------------------------

#define MAX_PADS 4
#define PAD_STICK_DEAD 12000        // of 32767: how far the stick has to go to count as a direction
#define PAD_TRIGGER_DOWN 16000      // of 32767: how far a trigger has to go to count as pressed

static SDL_Gamepad *s_pads[MAX_PADS];
static unsigned s_virtualPad;

static void PadAdded(SDL_JoystickID id)
{
    int i;
    for (i = 0; i < MAX_PADS; i++) {
        if (s_pads[i] && SDL_GetGamepadID(s_pads[i]) == id)
            return;
    }
    for (i = 0; i < MAX_PADS; i++) {
        if (s_pads[i] == NULL) {
            s_pads[i] = SDL_OpenGamepad(id);
            if (s_pads[i] == NULL)
                SDL_Log("SDL_OpenGamepad failed: %s", SDL_GetError());
            else
                SDL_Log("Gamepad %d: %s", i, SDL_GetGamepadName(s_pads[i]));
            return;
        }
    }
}

static void PadRemoved(SDL_JoystickID id)
{
    int i;
    for (i = 0; i < MAX_PADS; i++) {
        if (s_pads[i] && SDL_GetGamepadID(s_pads[i]) == id) {
            SDL_CloseGamepad(s_pads[i]);
            s_pads[i] = NULL;
        }
    }
}

unsigned SysPadState(int pad)
{
    SDL_Gamepad *g;
    unsigned bits;
    int x, y;

    if (pad < 0 || pad >= MAX_PADS)
        return 0;
    RefreshInput();
    bits = pad == 0 ? s_virtualPad : 0;
    g = s_pads[pad];
    if (g == NULL)
        return bits;

    x = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTX);
    y = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTY);
    if (x < -PAD_STICK_DEAD || SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
        bits |= PAD_LEFT;
    if (x > PAD_STICK_DEAD || SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
        bits |= PAD_RIGHT;
    if (y < -PAD_STICK_DEAD || SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_UP))
        bits |= PAD_UP;
    if (y > PAD_STICK_DEAD || SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
        bits |= PAD_DOWN;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_SOUTH) ||
        SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > PAD_TRIGGER_DOWN)
        bits |= PAD_FIRE;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_EAST) || SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_WEST) ||
        SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > PAD_TRIGGER_DOWN)
        bits |= PAD_ROCKET;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_START))
        bits |= PAD_PAUSE;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_BACK))
        bits |= PAD_PROFILE;
    return bits;
}

// Called by the browser page's touch controls (web/index.html).
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void SysSetVirtualPad(unsigned bits)
{
    s_virtualPad = bits;
}
