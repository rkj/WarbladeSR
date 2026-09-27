// strutil.cpp: String and memory helpers, number formatting.
#include "globals.h"
#include "game.h"


// Length of a NUL-terminated string.
int StrLen(char *s)
{
    int n = 0;
    while (*s != 0) {
        s++;
        n++;
    }
    return n;
}

// Simple polynomial hash (h = h*31 + c) of a NUL-terminated string.
int StrHash(char *s)
{
    int h = 0;
    int i;
    for (i = 0; i < StrLen(s); i++)
        h = h * 31 + s[i];
    return h;
}

// ASCII lowercase of `c`; non-letters pass through unchanged.
char ToLower(char c)
{
    if (c >= 'A' && c <= 'Z')
        c = c + 32;
    return c;
}

// Lowercases up to `max` characters of `src` into `dst` (NUL-terminated). No-op if
// `src` is already empty.
void StrToLowerN(const char* src, char* dst, int max)
{
    int i = 0;
    if (src[i] == 0)
        return;
    while (i < max && src[i] != 0) {
        dst[i] = ToLower(src[i]);
        i++;
    }
    dst[i] = 0;
}

// Concatenates `a`+`b`+`c` into the shared `g_concatBuf` and returns it. NOTE: does nothing
// (returns the buffer's previous contents) when `b` is NULL.
char *Concat3(const char *a, const char *b, const char *c)
{
    const char *p;
    int n;
    if (b) {
        n = 0;
        for (p = a; *p; p++, n++)
            g_concatBuf[n] = *p;
        for (p = b; *p; p++, n++)
            g_concatBuf[n] = *p;
        for (p = c; *p; p++, n++)
            g_concatBuf[n] = *p;
        g_concatBuf[n] = 0;
    }
    return g_concatBuf;
}

// Replaces every '\' in `s` with '/', in place.
void BackslashToSlash(char *s)
{
    int len = strlen(s);
    int i;
    if (len < 1)
        return;
    for (i = 0; i < len; i++) {
        if (*s == '\\')
            *s = '/';
        s++;
    }
}

// Lowercases `s` in place and returns it.
char *StrLower(char *s)
{
    int len = strlen(s);
    int i;
    for (i = 0; i < len; i++)
        s[i] = tolower(s[i]);
    return s;
}

// Wrapper around strstr(): returns a pointer to the first occurrence of `sub` in `s`, or NULL.
char *StrContains(char *s, const char *sub)
{
    return (char *)strstr((const char *)s, sub);
}

// memcpy wrapper that also returns the byte count copied.
int CopyBytes(void *src, void *dst, int size)
{
    memcpy(dst, src, size);
    return size;
}

// Parses a decimal integer starting at *p, skipping leading and trailing whitespace/'x'
// separators. Returns def (and leaves *p unmoved past the skipped separators) if no
// digit is found.
int ParseInt(char **p, int def)
{
    int n = 0;

    while (**p == ' ' || **p == '\r' || **p == '\n' || **p == '\t' || **p == 'x')
        (*p)++;
    if (**p < '0' || **p > '9')
        return def;
    while (**p >= '0' && **p <= '9') {
        n = n * 10 + **p - '0';
        (*p)++;
    }
    while (**p == ' ' || **p == '\r' || **p == '\n' || **p == '\t' || **p == 'x')
        (*p)++;
    return n;
}

// Formats v as a decimal string with '.' thousands separators into out (e.g. 1234567
// -> "1.234.567"). Uses g_numBuf as scratch space. Returns the number of characters
// written, including separators.
int Int64ToStrGrouped(__int64 v, char *out)
{
    int count = 0;
    int grp = 0;
    __int64 mod = 10;
    __int64 div = 1;
    __int64 r;
    __int64 d;
    char *p = g_numBuf;

    do {
        r = v % mod;
        d = r / div;
        *p = d + '0';
        p++;
        grp++;
        v -= r;
        if (grp == 3 && v != 0) {
            *p = '.';
            p++;
            grp = 0;
            count++;
        }
        mod *= 10;
        div *= 10;
        count++;
    } while (v > 0);
    p--;
    for (grp = 0; grp < count; grp++, out++, p--)
        *out = *p;
    *out = 0;
    return count;
}

// Formats v as a plain decimal string (no separators) into out. Uses g_numBuf as
// scratch space. Returns the number of digits written.
int Int64ToStr(__int64 v, char *out)
{
    int count = 0;
    int grp = 0;
    __int64 mod = 10;
    __int64 div = 1;
    __int64 r;
    __int64 d;
    char *p = g_numBuf;

    do {
        r = v % mod;
        d = r / div;
        *p = d + '0';
        p++;
        v -= r;
        mod *= 10;
        div *= 10;
        count++;
    } while (v > 0);
    p--;
    for (grp = 0; grp < count; grp++, out++, p--)
        *out = *p;
    *out = 0;
    return count;
}

// Copies n bytes from src into dst+off. Does not NUL-terminate.
void CopyBytesAt(char *dst, char *src, int off, int n)
{
    int i;

    dst += off;
    for (i = 0; i < n; i++, dst++, src++)
        *dst = *src;
}

// Copies n bytes from src into dst+off and NUL-terminates the result.
void CopyStrAt(char *dst, char *src, int off, int n)
{
    int i;

    dst += off;
    for (i = 0; i < n; i++, dst++, src++)
        *dst = *src;
    *dst = 0;
}

// strlen() reimplementation.
int StrLenPlat(char *s)
{
    int n;

    for (n = 0; *s; s++, n++)
        ;
    return n;
}

// Uppercases s in place.
void StrUpper(char *s)
{
    while (*s) {
        *s = toupper(*s);
        s++;
    }
}
