// resources.cpp: Loading graphics (KGraphic) and .hma masks.
#include <stdio.h>
#include "globals.h"
#include "game.h"


// Creates a KGraphic, loads `name` (lowercased into g_gfxPath) as its picture with the
// given alpha-mask/quality flags, and enables texture quality + alpha blending. Returns
// null and logs an error if creation or loading fails.
// NOTE: the "could not create" error is formatted into g_msgBuf but g_staleErrBuf (a
// leftover/different buffer) is what actually gets logged — matches the original.
KGraphic *LoadGraphic(char *name, bool maskAlpha, bool hiQuality)
{
    KGraphic *gfx = KPTK::createKGraphic();
    if (gfx == 0) {
        sprintf(g_msgBuf, "ERROR: Could not create %s graphics\n", name);
        LogPrint(g_staleErrBuf);
        return 0;
    }
    StrToLowerN(name, g_gfxPath, 1024);
    if (!gfx->loadPicture(g_gfxPath, maskAlpha, hiQuality)) {
        sprintf(g_msgBuf, "ERROR: Could not load %s file\n", name);
        LogPrint(g_msgBuf);
        return 0;
    }
    gfx->setTextureQuality(true);
    gfx->setAlphaMode(true);
    return gfx;
}

// Same as LoadGraphic() but leaves alpha blending mode untouched (used for graphics
// that don't need setAlphaMode()).
KGraphic *LoadGraphic2(char *name, bool maskAlpha, bool hiQuality)
{
    KGraphic *gfx = KPTK::createKGraphic();
    if (gfx == 0) {
        sprintf(g_msgBuf, "ERROR: Could not create %s graphics\n", name);
        LogPrint(g_staleErrBuf);
        return 0;
    }
    StrToLowerN(name, g_gfxPath, 1024);
    if (!gfx->loadPicture(g_gfxPath, maskAlpha, hiQuality)) {
        sprintf(g_msgBuf, "ERROR: Could not load %s file\n", name);
        LogPrint(g_msgBuf);
        return 0;
    }
    gfx->setTextureQuality(true);
    return gfx;
}

// Loads a raw `w`x`h` (1 byte/pixel) image from `<name>.hma` via KResource. Returns a
// malloc'd buffer the caller owns, or NULL if the file can't be opened.
void *LoadHma(const char *name, int w, int h)
{
    char levelname[512];
    int unused = 0;
    KResource resData;
    int err;
    void *buf;

    buf = calloc(w * h, 1);
    if (buf) {
        StrToLowerN(name, levelname, 512);
        sprintf(levelname, "%s.hma", levelname);
        err = resData.open(levelname, K_RES_READ);
        if (err == 0) {
            resData.seek(K_RES_BEGIN, 0);
            resData.read(buf, w * h);
            resData.close();
            return buf;
        } else {
            free(buf);
            return NULL;
        }
    }
}
