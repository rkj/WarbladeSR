// news.cpp: The online news feed: fetching it, the wait screen, the ticker.
#include <stdio.h>
#include "globals.h"
#include "game.h"


// Looks for `tag` in the HTTP response `resp` (searched up to `maxlen` bytes); if found,
// parses the following 3-digit news id into g_newsIds[idx] and copies the rest of the
// tagged line into dst.
#define NEWS_FETCH(maxlen, tag, idx, dst)                                                   \
    if (FindTag(resp, maxlen, tag, g_newsBuf, 0x400)) {                       \
        g_newsIds[idx] = (g_newsBuf[0] - '0') * 100 + (g_newsBuf[1] - '0') * 10 + g_newsBuf[2] - '0'; \
        for (i##idx = 0; i##idx < StrLenPlat(g_newsBuf) - 4; i##idx++) {         \
            dst[i##idx] = g_newsBuf[i##idx + 4];                                     \
            dst[i##idx + 1] = 0;                                                            \
        }                                                                                   \
    }

// Updates the last-4-seen-news-ids ring in g_cfg with news item idx: bumps its seen count if
// it's already tracked, otherwise pushes it in as the newest entry (evicting the oldest).
#define NEWS_TRACK(idx)                                                                     \
    if (g_newsIds[idx] != -1 && !g_newsFail) {                                \
        found = false;                                                                      \
        if (g_cfg.newsIds[0] == g_newsIds[idx]) {                         \
            g_cfg.newsCounts[0]++;                                               \
            g_newsA[idx] = g_cfg.newsCounts[0];                        \
            found = true;                                                                   \
        }                                                                                   \
        if (g_cfg.newsIds[1] == g_newsIds[idx]) {                         \
            g_cfg.newsCounts[1]++;                                               \
            g_newsA[idx] = g_cfg.newsCounts[1];                        \
            found = true;                                                                   \
        }                                                                                   \
        if (g_cfg.newsIds[2] == g_newsIds[idx]) {                         \
            g_cfg.newsCounts[2]++;                                               \
            g_newsA[idx] = g_cfg.newsCounts[2];                        \
            found = true;                                                                   \
        }                                                                                   \
        if (g_cfg.newsIds[3] == g_newsIds[idx]) {                         \
            g_cfg.newsCounts[3]++;                                               \
            g_newsA[idx] = g_cfg.newsCounts[3];                        \
            found = true;                                                                   \
        }                                                                                   \
        if (!found) {                                                                       \
            g_cfg.newsIds[3] = g_cfg.newsIds[2];                      \
            g_cfg.newsCounts[3] = g_cfg.newsCounts[2];                \
            g_cfg.newsIds[2] = g_cfg.newsIds[1];                      \
            g_cfg.newsCounts[2] = g_cfg.newsCounts[1];                \
            g_cfg.newsIds[1] = g_cfg.newsIds[0];                      \
            g_cfg.newsCounts[1] = g_cfg.newsCounts[0];                \
            g_cfg.newsIds[0] = g_newsIds[idx];                            \
            g_cfg.newsCounts[0] = 0;                                             \
            g_newsA[idx] = 0;                                                     \
        }                                                                                   \
    }

// Fetch the news page and update the seen-news history.
void CheckNews()
{
    KWeb *http = new KWeb();
    char *resp = 0;
    bool found;
    int i0, i1, i2, i3;

    g_newsIds[0] = 0;
    g_newsIds[1] = 0;
    g_newsIds[2] = 0;
    g_newsIds[3] = 0;
    g_newsA[0] = -1;
    g_newsA[1] = -1;
    g_newsA[2] = -1;
    g_newsA[3] = -1;
    g_newsB[0] = 0;
    g_newsB[1] = 0;
    g_newsB[2] = 0;
    g_newsB[3] = 0;

    if (http != 0) {
        if (g_cfg.netMode == 2) {  // 2 = online mode (news fetch enabled)
            g_newsFail = false;
            sprintf(g_logBuf, "http://www.warblade.as/tcp_checknews.asp");
            resp = http->callURL(g_logBuf, true);
            if (resp == 0)
                resp = http->callURL(g_logBuf, false);
            if (resp != 0) {
                LogPrint("Checking for news!\r\n");
                NEWS_FETCH(0x800, "NEWS1:", 0, g_news1)   // ticker item: small buffer
                NEWS_FETCH(0x4000, "NEWS2:", 1, g_news2)  // full news items: larger buffers
                NEWS_FETCH(0x4000, "NEWS3:", 2, g_news3)
                NEWS_FETCH(0x4000, "NEWS4:", 3, g_news4)
            } else {
                g_newsFail = true;
                g_tickerIdx = 0;
                sprintf(g_news1, "NEWS IS NOT AVAILABLE AT THIS TIME - NEWS SERVER MAY BE DOWN");
            }

            NEWS_TRACK(3)
            NEWS_TRACK(2)
            NEWS_TRACK(1)
            NEWS_TRACK(0)
            if (g_newsIds[0] + g_newsIds[1] + g_newsIds[2] + g_newsIds[3] > 0 &&
                !g_newsFail) {
                MergeSettings(g_profileIndex);
                WriteSettings();
            }
            if (g_newsFail) {
                g_newsIds[0] = 1;  // NOTE: nonzero id 1 with no matching news text; just
                g_newsA[0] = 0;    // enough to make the "server may be down" ticker show
            } else {
                g_newsReady = true;
            }
        }
        delete http;
    }
}

#undef NEWS_FETCH
#undef NEWS_TRACK

// If ticker slot `idx` is the active one, scrolls `buf` left across the bottom of the screen until it's
// fully off-screen, then advances the ticker to `next` and resets the scroll position; skips straight to
// `next` if the slot has run out its allowance (g_newsA/g_newsB) or has no text.
#define NEWS_STEP(idx, buf, next)                                                          \
    if (g_tickerIdx == idx) {                                                                 \
        if (g_newsA[idx] < 8 && g_newsB[idx] < 5 && StrLenPlat(buf) > 0) { \
            DrawNewsText(buf, (int)g_newsX, g_screenH - 30, font);    \
            g_newsX = g_newsX - g_newsSpeed;                          \
            if ((int)g_newsX < 0 - StrLenPlat(buf) * 8) {                         \
                g_newsB[idx]++;                                                     \
                g_tickerIdx = next;                                                           \
                g_newsX = (float)g_screenW;                                  \
            }                                                                              \
        } else {                                                                           \
            g_tickerIdx = next;                                                               \
            g_newsX = (float)g_screenW;                                      \
        }                                                                                  \
    }

// Draws whichever of the 4 news-ticker slots (g_news1..4) is currently scrolling, cycling round-robin
// through them (0 -> 1 -> 2 -> 3 -> 0). Skipped entirely if no news items are loaded (g_newsIds all zero).
void NewsTicker()
{
    if (g_newsIds[0] + g_newsIds[1] + g_newsIds[2] + g_newsIds[3] > 0) {
        int font = g_font;
        // Occasionally swap in the small font (8) instead of the large ticker font (12).
        if (font == 12 && Rand7f() < 64)
            font = 8;
        NEWS_STEP(0, g_news1, 1)
        NEWS_STEP(1, g_news2, 2)
        NEWS_STEP(2, g_news3, 3)
        NEWS_STEP(3, g_news4, 0)
    }
}

#undef NEWS_STEP

extern "C" __declspec(dllimport) int __stdcall GetSystemMetrics(int nIndex);

// Shows the "CHECKING FOR NEWSFEED! PLEASE WAIT..." screen with scattered "N E W S"
// background text while the news feed request is in flight.
void ShowNewsWait()
{
    int count;
    int i;
    int j;
    SetViewHud();
    count = RandRange(40, 50);
    for (i = 0; i < count; i++)
        DrawNewsText("N E W S", RandRange(0, 800), RandRange(0, 250), 8);
    for (j = 0; j < count; j++)
        DrawNewsText("N E W S", RandRange(0, 800), RandRange(350, 600), 8);
    DrawMenuText("CHECKING FOR NEWSFEED!  PLEASE WAIT...", POS_CENTERED, g_screenH / 2, 0);
    FlushBlit(g_screen);
    FlipBuffer(0);
}
