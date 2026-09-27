// online.cpp: Online services: serial codes, score upload (with its replay-record checks),
// clipboard paste.
#include <stdio.h>
#include "globals.h"
#include "game.h"


// Counts g_cfg.serialCode[i] as valid if it's a digit or uppercase letter.
#define PWCHK(i) \
    if ((g_cfg.serialCode[i] >= '0' && g_cfg.serialCode[i] <= '9') || \
        (g_cfg.serialCode[i] >= 'A' && g_cfg.serialCode[i] <= 'Z')) \
        n++;

// True if all 15 characters of g_cfg.serialCode are digits or uppercase letters.
bool SerialValid()
{
    short n = 0;
    PWCHK(0) PWCHK(1) PWCHK(2) PWCHK(3) PWCHK(4)
    PWCHK(5) PWCHK(6) PWCHK(7) PWCHK(8) PWCHK(9)
    PWCHK(10) PWCHK(11) PWCHK(12) PWCHK(13) PWCHK(14)
    if (n == 15)
        return true;
    return false;
}

#undef PWCHK

// Asks the Warblade website whether g_cfg.serialCode is a valid serial. Clears the
// serial if the response's SERIAL: tag isn't "Y". `serial` is unused (the current
// serial is read from g_cfg instead).
bool CheckSerialOnline(char* serial)
{
    KWeb *http = new KWeb();
    char *resp = 0;
    bool ok = false;
    if (http) {
        sprintf(g_logBuf, "http://www.warblade.as/tcp_checkserial.asp?serial=%s", g_cfg.serialCode);
        resp = http->callURL(g_logBuf, true);
        if (!resp)
            resp = http->callURL(g_logBuf, false);
        if (resp) {
            if (FindTag(resp, 0x800, "SERIAL:", g_newsBuf, 0x400)) {
                if (g_newsBuf[0] == 'Y')
                    ok = true;
                else
                    g_cfg.serialCode[0] = 0;
            }
        }
        delete http;
    }
    return ok;
}

// These scan g_replayRecs[t][1..3999] (per-mode replay verification records) and fold
// them into a checksum-like number that's sent alongside an uploaded hiscore, as a
// crude tamper check on the server side. `+ 500`/`+ 700` are arbitrary offsets baked
// into the protocol.

// Counts "plausible" consecutive replay records (matching score, shots within 1 of the
// previous record, all verify flags clear, low frame bucket) for mode `t`.
int StatCount(int t)
{
    int n = 0;
    int i;
    for (i = 1; i < MAX_LEVEL_RECS; i++) {
        if (g_replayRecs[t][i].score != 0 && g_replayRecs[t][i].shots != 0 &&
            g_replayRecs[t][i].frameBucket != 0 &&
            g_replayRecs[t][i].score == g_replayRecs[t][i - 1].score &&
            g_replayRecs[t][i].shots - g_replayRecs[t][i - 1].shots < 2 &&
            g_replayRecs[t][i].verify5 == 0 && g_replayRecs[t][i].verify6 == 0 &&
            g_replayRecs[t][i].verify1 == 0 &&
            g_replayRecs[t][i].frameBucket < 3)
            n++;
    }
    return n + 500;
}

// Defines a NAME(t) that sums FIELD across all populated replay records for mode `t`
// and adds ADD.
#define STATSUM(NAME, FIELD, ADD) \
int NAME(int t) \
{ \
    int n = 0; \
    int i; \
    for (i = 1; i < MAX_LEVEL_RECS; i++) { \
        if (g_replayRecs[t][i].score != 0 && g_replayRecs[t][i].shots != 0 && \
            g_replayRecs[t][i].frameBucket != 0) \
            n += g_replayRecs[t][i].FIELD; \
    } \
    return n + ADD; \
}

STATSUM(StatSum13, livesGainedByte, 500)

STATSUM(StatSum14, deathsByte, 700)

STATSUM(StatSum15, armourAddedByte, 500)

STATSUM(StatSum16, verify3, 700)

#undef STATSUM

// Accessors used generically by the STD_TABLE macro below.
int EntryMonth(HiscoreEntry e)
{
    return e.month;
}

// Accessor used generically by the STD_TABLE macro below.
int EntryYear(HiscoreEntry e)
{
    return e.year;
}

// Finds this player's best new entry in each score table (one set this month/year and
// not already sent) and posts it to the Warblade website.

// Formats the stat fields for entry E (mode T) and posts the score-add request; on a
// server-side rejection (ADDED:NO) clears+resaves the serial, on success (ADDED:OK)
// sets `added`. Always marks the per-table search `found` so STD_TABLE stops scanning.
#define SEND_SCORE(E, T)                                                                        \
    sprintf(g_sA, "%d", StatCount(T));                                                   \
    sprintf(g_sB, "%d", StatSum13(T));                                                   \
    sprintf(g_sC, "%d", StatSum14(T));                                                   \
    sprintf(g_sD, "%d", StatSum15(T));                                                   \
    sprintf(g_sE, "%d", StatSum16(T));                                                   \
    sprintf(g_sDate, "%d/%d/%d", (E).month, (E).day, (E).year);                 \
    sprintf(g_logBuf,                                                                    \
            "http://www.warblade.as/addscore132.asp?sName=%s&sScore=%s&sLevel=%s&sRank=%s&pw=%s&gtr=%s&mm=%s&A=%s&B=%s&C=%s&D=%s&E=%s", \
            g_sName, g_sScore, g_sLevel, g_sRank, g_cfg.serialCode,      \
            g_sDate, g_sMode, g_sA, g_sB, g_sC, g_sD, \
            g_sE);                                                                       \
    resp = http->callURL(g_logBuf, true);                                        \
    if (resp == 0)                                                                              \
        resp = http->callURL(g_logBuf, false);                                   \
    if (resp != 0) {                                                                            \
        if (FindTag(resp, 0x800, "ADDED:NO", g_newsBuf, 0x400)) {                 \
            g_cfg.serialCode[0] = 0;                                                                 \
            MergeSettings(g_profileIndex);                                        \
            WriteSettings();                                                                        \
        }                                                                                       \
        if (FindTag(resp, 0x800, "ADDED:OK", g_newsBuf, 0x400))                   \
            added = true;                                                                       \
    }                                                                                           \
    found = true;

// Scans a 20-entry hiscore table TBL for the first entry dated this month/year whose
// score beats g_bestSent[K]; if found, formats it (mode MM, replay stats for mode T)
// and sends it via SEND_SCORE.
#define STD_TABLE(TBL, K, MM, T)                                                                \
    found = false;                                                                              \
    for (int i = 0; i < MAX_HISCORES; i++) {                                                              \
        if (!found) {                                                                           \
            if (TBL[i].score > 0 && EntryMonth(TBL[i]) == a && EntryYear(TBL[i]) == b &&      \
                TBL[i].score > g_bestSent[K]) {                                       \
                g_bestSent[K] = TBL[i].score;                                         \
                sprintf(g_sName, "%s", TBL[i].name);                                  \
                Int64ToStr(TBL[i].score, g_sScore);                           \
                sprintf(g_sLevel, "%d", TBL[i].level);                                \
                sprintf(g_sRank, "%s", g_rankNames[TBL[i].rank]);              \
                sprintf(g_sMode, "%d", MM);                                              \
                SEND_SCORE(TBL[i], T)                                                           \
            }                                                                                   \
        }                                                                                       \
    }

// Uploads at most one new score per table (4 standard difficulty tables plus the two
// special tables inlined below, since their layout differs enough that STD_TABLE
// doesn't fit) and returns whether any score was accepted (ADDED:OK).
bool UploadScores()
{
    KWeb *http = new KWeb;
    char *resp = 0;
    bool added = false;
    if (http) {
        DecompressHiscores();
        int a = CurMonth();
        int b = CurYear();
        bool found = false;
        STD_TABLE(g_hiscoreMagic.table[HOF_EASY], HOF_EASY, 1, 0)
        STD_TABLE(g_hiscoreMagic.table[HOF_NORMAL], HOF_NORMAL, 2, 1)
        STD_TABLE(g_hiscoreMagic.table[HOF_HARD], HOF_HARD, 3, 2)
        STD_TABLE(g_hiscoreMagic.table[HOF_ACE], HOF_ACE, 4, 3)

        // table[4]: level is always reported as 10000, mode 5, replay stats read from mode 0.
        found = false;
        for (int i = 0; i < MAX_HISCORES; i++) {
            if (!found) {
                if (g_hiscoreMagic.table[4][i].score > 0 && EntryMonth(g_hiscoreMagic.table[4][i]) == a &&
                    EntryYear(g_hiscoreMagic.table[4][i]) == b &&
                    g_hiscoreMagic.table[4][i].score > g_bestSent[5]) {
                    g_bestSent[5] = g_hiscoreMagic.table[4][i].score;
                    sprintf(g_sName, "%s", g_hiscoreMagic.table[4][i].name);
                    Int64ToStr(g_hiscoreMagic.table[4][i].score, g_sScore);
                    sprintf(g_sLevel, "%d", 10000);
                    sprintf(g_sRank, "%s", g_rankNames[g_hiscoreMagic.table[4][i].rank]);
                    sprintf(g_sMode, "%d", 5);
                    SEND_SCORE(g_hiscoreMagic.table[4][i], 0)
                }
            }
        }

        // table5: level always 10000, rank field sent as the literal 5000 (not a rank name), mode 6.
        found = false;
        for (int i = 0; i < MAX_HISCORES; i++) {
            if (!found) {
                if (g_hiscoreMagic.table5[i].score > 0 && EntryMonth(g_hiscoreMagic.table5[i]) == a &&
                    EntryYear(g_hiscoreMagic.table5[i]) == b &&
                    g_hiscoreMagic.table5[i].score > g_bestSent[4]) {
                    g_bestSent[4] = g_hiscoreMagic.table5[i].score;
                    sprintf(g_sName, "%s", g_hiscoreMagic.table5[i].name);
                    Int64ToStr(g_hiscoreMagic.table5[i].score, g_sScore);
                    sprintf(g_sLevel, "%d", 10000);
                    sprintf(g_sRank, "%d", 5000);
                    sprintf(g_sMode, "%d", 6);
                    SEND_SCORE(g_hiscoreMagic.table5[i], 4)
                }
            }
        }
        delete http;
        ClearHiscores();
    }
    return added;
}

#undef SEND_SCORE
#undef STD_TABLE

// Pastes a 15-character serial from the clipboard into g_serial. Fails (and leaves
// g_serial cleared) if there's no text on the clipboard or its length isn't exactly 15.
bool PasteKeyFromClipboard()
{
    bool ok;
    HANDLE hData;
    char *text;

    ok = false;
    if (OpenClipboard(g_window->getWindowHandle())) {
        if (IsClipboardFormatAvailable(CF_TEXT) || IsClipboardFormatAvailable(CF_OEMTEXT)) {
            hData = GetClipboardData(CF_TEXT);
            if (hData) {
                text = (char *)GlobalLock(hData);
                g_serial[0] = 0;
                if (StrLenPlat(text) == 15) {
                    CopyStrAt(g_serial, text, 0, 15);
                    ok = true;
                }
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }
    return ok;
}
