// PTK engine API used by Warblade: declarations only.
//
// PTK (Phelios, http://ptk.phelios.com) is third-party code linked into warblade.exe at
// 0x627A10-0x6FBB0C. It is not decompiled; this header declares the part the game uses, with
// the real PTK names from docs/ptk/ (2009 web docs + the 2006 Mac class dump). It is also the
// spec for the PTK replacement.
//
// Rules for this file:
//  - Names come from the docs. Where docs and binary disagree, the binary wins (noted inline).
//  - Non-virtual functions carry no address suffix; their address is in symbols.txt, keyed by
//    decorated name. Update those entries after changing a signature.
//  - Virtual methods are called through the vtable, so only slot order and signature matter.
//    Every slot is declared, in order, as in the binary's OpenGL vtable. Unused slots whose
//    identity is not certain are named slot_XX (their offset).
//  - Addresses are the real function addresses in warblade.exe (not incremental-link thunks).
#pragma once

// Win32 handle/callback types without <windows.h> (identical to its STRICT definitions, so
// files that do include <windows.h> still compile).
struct HWND__;
typedef HWND__ *HWND;

// ---------------------------------------------------------------------------------------------
// KPTK: factory (kwindow.html, tuto1.html)
// ---------------------------------------------------------------------------------------------

// The game passes 1 for its DirectX attempt and 2 for its OpenGL fallback (see the log strings
// in shard_5a15b0). This PTK build only tests renderer != 0, so both create the OpenGL window
// (KWindowGL, 0xEC bytes); 0 would create the 0x400-byte other renderer. Values are the game's.
enum Erenderer {
    K_DIRECTX = 1,
    K_OPENGL  = 2,
};

class KWindow;
class KGraphic;

class KPTK {
public:
    static KWindow  *createKWindow(Erenderer renderer, bool);  // 0x627a10 (2nd arg: always false in the game; undocumented)
    static KGraphic *createKGraphic(void);                     // 0x627af0
};

// ---------------------------------------------------------------------------------------------
// KWindow (kwindow.html). vtable: KWindowGL 0x786D8C, 33 slots.
// Differences from the Mac 2006 layout: one destructor slot instead of two (MSVC), so
// createGameWindow moves up to +0x10; a new slot at +0x14; setQuit moved up to +0x34.
// ---------------------------------------------------------------------------------------------

// Windows form of the event callback (kwindow.html). Return false to stop the system handling it.
typedef bool (*ptkWindowCallBack)(HWND hWnd, unsigned int messg, unsigned int wParam, long lParam);

class KWindow {
public:
    virtual void setDefaultWorldView(void);                                                  // +0x00 0x62a450
    virtual void setWorldView(float translateX, float translateY, float rotation, float zoom,
                              bool clearworld);                                              // +0x04 0x62a480
    virtual void setClearColor(float r, float g, float b, float alpha);                      // +0x08 0x62afb0
    virtual ~KWindow();                                                                      // +0x0c 0x62a2f0 (scalar deleting)
    virtual bool createGameWindow(short width, short height, short depth, bool windowed,
                                  const char *windowTitle);                                 // +0x10 0x62a7d0
    virtual void slot_14(long);                                                              // +0x14 0x62a3f0 undocumented: stores its arg at this+4
    virtual void terminate(void);                                                            // +0x18 0x62be70
    virtual void toggleFullScreen(bool fullscreen);                                          // +0x1c 0x62b740
    virtual bool hasFocus(void);                                                             // +0x20 0x62afe0
    virtual void setTitle(const char *title);                                                // +0x24 0x62af80
    virtual void flipBackBuffer(bool waitifBackWindow = true, bool restoreView = true);      // +0x28 0x62b390 (docs show 1 arg; binary pops 2)
    virtual void processEvents(void);                                                        // +0x2c 0x62be90
    virtual bool isQuit(void);                                                               // +0x30 0x62b680
    virtual void setQuit(bool qstate);                                                       // +0x34 0x62af40
    virtual HWND getWindowHandle(void);                                                      // +0x38 0x62af60
    virtual void setMaxFrameRate(long desiredFrameRate);                                     // +0x3c 0x62b020
    virtual bool getRectangleTexCap(void);                                                   // +0x40 0x62ae10
    virtual bool getAccelerationCap(void);                                                   // +0x44 0x62ae60
    virtual void setRectangleTexCap(bool);                                                   // +0x48 0x62ae40
    virtual void setGamma(float gamma);                                                      // +0x4c 0x62adf0
    virtual short getWindowWidth(void);                                                      // +0x50 0x62a410
    virtual short getWindowHeight(void);                                                     // +0x54 0x62a430
    virtual void restore(void);                                                              // +0x58 0x62bf90
    virtual void minimize(void);                                                             // +0x5c 0x62bf70
    virtual void slot_60(bool);                                                              // +0x60 0x62a680 undocumented: sets flipBackBuffer's default wait flag
    virtual bool saveBackBuffer(const char *fileName, long imageFormat = 0, long resizeW = 0,
                                long resizeH = 0);                                           // +0x64 0x62b090
    virtual void slot_68(long, long, long, long);                                            // +0x68 0x62bbf0 undocumented: glScissor helper
    virtual void setCallback(ptkWindowCallBack callbackFunc);                                // +0x6c 0x62a790
    virtual void setPTKCallback(ptkWindowCallBack callbackFunc);                             // +0x70 0x62a7b0 (real type: bool (*)(KEvent *))
    virtual void slot_74(void *);                                                            // +0x74 0x62bc40 probably enumerateDisplays(ptkEnumDisplayCallBack)
    virtual void showWindow(bool show);                                                      // +0x78 0x62bb80 undocumented: ShowWindow(SW_SHOW / SW_HIDE)
    virtual bool slot_7c(void);                                                              // +0x7c 0x62bbc0 undocumented: IsWindowVisible
    virtual bool slot_80(void);                                                              // +0x80 0x62c5d0 undocumented: probably isFullScreen
};

// ---------------------------------------------------------------------------------------------
// KGraphic (kgraphic.html). vtable: KGraphicGL 0x786F7C, 40 slots.
// Relative to Mac 2006: +0x0c from loadPicture to blitAlphaRectFxF, +0x10 from setAlphaMode on,
// and 4 new slots at the end. Param counts checked against each GL method's `ret N`.
// ---------------------------------------------------------------------------------------------

class KGraphic {
public:
    virtual ~KGraphic();                                                                     // +0x00 0x62ffb0
    virtual void slot_04(void);                                                              // +0x04 0x631850
    virtual void slot_08(void);                                                              // +0x08 0x6318d0 load family (loadPictureWithMask?)
    virtual void slot_0c(void);                                                              // +0x0c 0x631910 load family (loadPictureFromPtr?)
    virtual void slot_10(void);                                                              // +0x10 0x630a00 texture build/upload
    virtual void slot_14(void);                                                              // +0x14 0x631890 load family
    virtual void slot_18(void);                                                              // +0x18 0x6311a0 texture build/upload
    virtual bool loadPicture(const char *filename, bool hiDef, bool hasAlpha);               // +0x1c 0x631950
    virtual void freePicture(void);                                                          // +0x20 0x6329b0
    virtual float getWidth(void);                                                            // +0x24 0x630570
    virtual float getHeight(void);                                                           // +0x28 0x630590
    virtual void setTextureQuality(bool quality);                                            // +0x2c 0x636470
    virtual void drawRect(float x1, float y1, float x2, float y2,
                          float r, float g, float b, float blend);                           // +0x30 0x635590
    virtual void drawLine(float x1, float y1, float x2, float y2,
                          float r, float g, float b, float blend);                           // +0x34 0x635890
    virtual void plotPixel(float x, float y, float r, float g, float b, float blend);        // +0x38 0x6359c0
    virtual void blitRect(float x1, float y1, float x2, float y2, short destX, short destY,
                          bool flipx, bool flipy);                                           // +0x3c 0x6330f0
    virtual void blitRectFx(float x1, float y1, float x2, float y2, short destX, short destY,
                            float angle, float zoom, bool flipx, bool flipy,
                            float centerX, float centerY);                                   // +0x40 0x633410
    virtual void blitAlphaRect(float x1, float y1, float x2, float y2, short destX, short destY,
                               bool flipx, bool flipy);                                      // +0x44 0x635220
    virtual void blitAlphaRectFx(float x1, float y1, float x2, float y2, short destX, short destY,
                                 float angle, float zoom, float blend, bool flipx, bool flipy,
                                 float centerX, float centerY);                              // +0x48 0x635be0
    virtual void blitTiledRect(short destX, short destY, short width, short height,
                               float tileFactor, float angle);                               // +0x4c 0x632ad0
    virtual bool blitArbitraryQuad(float sx1, float sy1, float sx2, float sy2,
                                   float sx3, float sy3, float sx4, float sy4,
                                   float dx1, float dy1, float dx2, float dy2,
                                   float dx3, float dy3, float dx4, float dy4);              // +0x50 0x632b60
    virtual void blitRectF(float x1, float y1, float x2, float y2, float destX, float destY,
                           bool flipx, bool flipy);                                          // +0x54 0x632cc0
    virtual void blitRectFxF(float x1, float y1, float x2, float y2, float destX, float destY,
                             float angle, float zoom, bool flipx, bool flipy,
                             float centerX, float centerY);                                  // +0x58 0x632db0
    virtual void blitAlphaRectF(float x1, float y1, float x2, float y2, float destX, float destY,
                                bool flipx, bool flipy);                                     // +0x5c 0x634fb0
    virtual void blitAlphaRectFxF(float x1, float y1, float x2, float y2, float destX, float destY,
                                  float angle, float zoom, float blend, bool flipx, bool flipy,
                                  float centerX, float centerY);                             // +0x60 0x635ab0
    virtual void slot_64(void);                                                              // +0x64 0x6353c0 new since 2006 (blitRect2d?)
    // Blend (src, dst) per mode, from the GL code: 0 SRC_ALPHA/ONE_MINUS_SRC_ALPHA (normal),
    // 1 SRC_ALPHA/ONE_MINUS_SRC_COLOR, 2 ONE/ONE_MINUS_SRC_COLOR, 3 ZERO/ONE_MINUS_SRC_COLOR,
    // 4 SRC_ALPHA/ONE (additive), 5 ONE/ZERO, 6 ONE_MINUS_SRC_COLOR/ONE; > 6 no change.
    virtual void setAlphaMode(short alphaMode);                                              // +0x68 0x6353f0
    virtual void stretchAlphaRect(float sx1, float sy1, float sx2, float sy2,
                                  float dx1, float dy1, float dx2, float dy2,
                                  float blend, float angle, bool flipx, bool flipy,
                                  float centerX, float centerY);                             // +0x6c 0x633170 (docs show 8 args; binary pops 14)
    virtual bool grabBackBuffer(void *callback);                                             // +0x70 0x635f30 (docs: no args; binary pops 1; game passes 0)
    virtual bool grabFrontBuffer(void);                                                      // +0x74 0x6367a0
    virtual void setColorKey(bool activate, unsigned char r, unsigned char g, unsigned char b); // +0x78 0x630050
    virtual void allowTextureWrap(bool wrapping);                                            // +0x7c 0x636500
    virtual void setBlitColor(float r, float g, float b, float a);                           // +0x80 0x636620
    virtual bool makePictureFromArray(unsigned long *buffer, long width, long height,
                                      bool hidef);                                           // +0x84 0x6305b0
    virtual void pollArray(void);                                                            // +0x88 0x630910
    virtual void selectTexture(void);                                                        // +0x8c 0x636c60
    virtual void slot_90(void);                                                              // +0x90 0x636ba0 new since 2006
    virtual void slot_94(void);                                                              // +0x94 0x636c80 new since 2006
    virtual void slot_98(void);                                                              // +0x98 0x633f70 new since 2006
    virtual void slot_9c(void);                                                              // +0x9c 0x636c30 new since 2006

    // First data member (Mac 2006: _userPixelPtr). The game's render queues read it to skip
    // objects already freed (0xFEEEFEEE is the debug heap's fill for freed memory).
    unsigned int _field04;                                                                   // +0x04
};

// No KSound: the game plays everything through BASS. The objects its sound cache (shard_557330)
// "stops" and deletes are KGraphics; the "stop" at vtable +0x20 is KGraphic::freePicture.

// ---------------------------------------------------------------------------------------------
// KInput (kinput.html, keytable.html): all static.
// ---------------------------------------------------------------------------------------------

// Key codes: the row index in PTK's key table at 0x7D3E40 ({int key; WORD vk; WORD pad} x 100).
// 0-92 are exactly the documented order; 93-99 exist only in this build (names are ours).
enum EKeyboardLayout {
    K_VK_LEFT = 0, K_VK_UP, K_VK_DOWN, K_VK_RIGHT, K_VK_SPACE,
    K_VK_L_SHIFT, K_VK_R_SHIFT, K_VK_RETURN, K_VK_R_CONTROL, K_VK_L_CONTROL,
    K_VK_F1, K_VK_F2, K_VK_F3, K_VK_F4, K_VK_F5, K_VK_F6,
    K_VK_F7, K_VK_F8, K_VK_F9, K_VK_F10, K_VK_F11, K_VK_F12,
    K_VK_BACK, K_VK_TAB, K_VK_ESCAPE,                                        // 22-24
    K_VK_A, K_VK_B, K_VK_C, K_VK_D, K_VK_E, K_VK_F, K_VK_G, K_VK_H, K_VK_I,  // 25-
    K_VK_J, K_VK_K, K_VK_L, K_VK_M, K_VK_N, K_VK_O, K_VK_P, K_VK_Q, K_VK_R,
    K_VK_S, K_VK_T, K_VK_U, K_VK_V, K_VK_W, K_VK_X, K_VK_Y, K_VK_Z,          // -50
    K_VK_0, K_VK_1, K_VK_2, K_VK_3, K_VK_4, K_VK_5, K_VK_6, K_VK_7, K_VK_8, K_VK_9,   // 51-60
    K_VK_NUM0, K_VK_NUM1, K_VK_NUM2, K_VK_NUM3, K_VK_NUM4,
    K_VK_NUM5, K_VK_NUM6, K_VK_NUM7, K_VK_NUM8, K_VK_NUM9,                   // 61-70
    K_VK_MULTIPLY, K_VK_ADD, K_VK_SUBTRACT, K_VK_DECIMAL, K_VK_DIVIDE,       // 71-75
    K_VK_CLEAR, K_VK_MENU, K_VK_LWIN, K_VK_RWIN, K_VK_NUMLOCK, K_VK_SCROLL,  // 76-81
    K_VK_OEM_1, K_VK_OEM_PLUS, K_VK_OEM_COMMA, K_VK_OEM_MINUS, K_VK_OEM_PERIOD,
    K_VK_OEM_2, K_VK_OEM_3, K_VK_OEM_4, K_VK_OEM_5, K_VK_OEM_6, K_VK_OEM_7,  // 82-92
    K_VK_END, K_VK_HOME, K_VK_DELETE, K_VK_INSERT,                           // 93-96 (undocumented)
    K_VK_PRINTSCREEN, K_VK_PAGEUP, K_VK_PAGEDOWN,                            // 97-99 (undocumented)
    K_VK_ERROR,                                                              // 100: table terminator / "no key"
};

class KInput {
public:
    static bool  isPressed(EKeyboardLayout keyCode);      // 0x629500 GetAsyncKeyState(table[key].vk) & 0x8000
    static short getMouseX(void);                         // 0x629580 client coords
    static short getMouseY(void);                         // 0x6295d0
    static bool  getLeftButtonState(void);                // 0x629620
    static bool  getRightButtonState(void);               // 0x629640
    static void  showPointer(void);                       // 0x6296c0
    static void  hidePointer(void);                       // 0x6296f0
    static void  clipPointer(bool clipToWindow);          // 0x629720 (this build only implements false = release)
    static bool  joyEnable(char joyID);                   // 0x629750
    static long  joyY(char joyID, char hatID);            // 0x6298f0 hat 0: dwYpos, else dwRpos; 0x7FFF if not enabled
    static long  joyX(char joyID, char hatID);            // 0x629970 hat 0: dwXpos, else dwZpos
    static bool  joyButtonN(char joyID, long buttonID);   // 0x629bc0 (dwButtons & buttonID) != 0; buttonID is a bit mask
};

// ---------------------------------------------------------------------------------------------
// KMiscTools (kmisctools.html): all static.
// ---------------------------------------------------------------------------------------------

class KMiscTools {
public:
    static void          initMiscTools(void);                             // 0x628760 (createKWindow also calls it)
    static char         *makeFilePath(const char *filename);              // 0x628870 app folder + name, '/' -> '\'; static buffer
    static char         *getUserFolder(void);                             // 0x6289d0 CSIDL_PERSONAL ("My Documents")
    static void          messageBox(const char *title, const char *text); // 0x628a10
    static unsigned long getMilliseconds(void);                           // 0x628cd0
    static long          flipLong(long v);                                // 0x628f60 identity on x86
};

// ---------------------------------------------------------------------------------------------
// KResource / KResourceArchive (kresource.html). Archives are tar files (data\warblade.pac).
// ---------------------------------------------------------------------------------------------

enum KResourceMode {
    K_RES_READ = 1000,
};

enum KResourceWhence {
    K_RES_BEGIN   = 0,
    K_RES_CURRENT = 1,     // name not in the docs (only BEGIN and END appear)
    K_RES_END     = 2,
};

enum KResourceResult {
    K_RES_OK = 0,          // errors seen in the binary: 100, 101, 102, 104, 106 (names unknown)
};

class KResource {
public:
    KResource();                                                    // 0x627ee0
    ~KResource();                                                   // 0x627f70
    KResourceResult open(const char *filename, KResourceMode mode); // 0x627fe0
    KResourceResult close(void);                                    // 0x628290
    KResourceResult seek(KResourceWhence whence, long offset);      // 0x6282d0
    KResourceResult read(void *buffer, unsigned long size);         // 0x6283f0
private:
    // 32 bytes. +0x00 is a vtable-like pointer (0x786D7C) set by the constructor; the game never
    // uses it, so it is not declared as virtual. +0x0C FILE *, +0x10 in-archive flag,
    // +0x14 open mode, +0x18 offset in archive, +0x1C size.
    char _opaque[32];
};

class KResourceArchive {
public:
    static void addArchive(const char *filename);                   // 0x628470
};

// ---------------------------------------------------------------------------------------------
// KWeb (kweb.html): WinINet URL fetch.
// ---------------------------------------------------------------------------------------------

class KWeb {
public:
    KWeb();                                                         // 0x629d20
    ~KWeb();                                                        // 0x629d40 frees the buffer callURL returned
    // Returns a NUL-terminated heap buffer owned by the object, or NULL. The flag is passed on
    // to InternetOpenA's flags ("currently unused" per the docs).
    char *callURL(const char *urlStr, bool asynchronousFlags);     // 0x629d70
private:
    void         *_hInternet;   // +0x00
    void         *_hUrl;        // +0x04
    char         *_buffer;      // +0x08
    unsigned long _bufferSize;  // +0x0C (getBufferSize(), 0x629e20, not used by the game)
};

// ---------------------------------------------------------------------------------------------
// zlib 1.2.1, statically linked with PTK (not PTK API). Declared as in zlib.h.
// ---------------------------------------------------------------------------------------------

extern "C" {
int compress(unsigned char *dest, unsigned long *destLen,
             const unsigned char *source, unsigned long sourceLen);    // 0x6292c0
int uncompress(unsigned char *dest, unsigned long *destLen,
               const unsigned char *source, unsigned long sourceLen);  // 0x629c40
}
