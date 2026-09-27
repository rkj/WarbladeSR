// net.cpp: Network: Winsock/HTTP transport, tag parsing, the version check, opening URLs.
#include <stdio.h>
#include "globals.h"
#include "game.h"


// Closes the game UI, flushes hiscores, then launches `url` in the default
// browser via "rundll32 url.dll,FileProtocolHandler" and minimizes the game window.
void OpenUrl(char *url)
{
    g_clickWin = -1;
    g_clickItem = -1;
    WinCloseAll();
    WriteHiscoreFile();
    ClearHiscores();
    sprintf(g_songPath, "url.dll,FileProtocolHandler %s", url);
    ShellExecuteA(0, "open", "rundll32.exe", g_songPath, 0, 3);
    g_window->minimize();
}

// Searches `buf` (`len` bytes) for the literal `tag`; if found, copies everything after
// it up to (but not including) the next newline or 0xA4 byte into `out`. Returns whether
// `tag` was found. `outLen` is unused (no bound is applied — NOTE: matches the original).
bool FindTag(char *buf, int len, char *tag, char *out, int outLen)
{
    bool found = false;
    int i;
    int j;
    int k;

    for (i = 0; i < len - StrLenPlat(tag); i++) {
        if (buf[i] == tag[0]) {
            found = true;

            for (j = 0; j < StrLenPlat(tag); j++) {
                if (buf[i + j] != tag[j]) {
                    found = false;
                    break;
                }
            }

            if (found) {
                k = 0;
                buf += StrLenPlat(tag) + i;

                do {
                    *out = *buf;
                    out++;
                    buf++;
                } while (*buf != 0 && *buf != '\n' && *buf != '\xa4');
                *out = 0;
                break;
            }
        }
    }
    return found;
}

// Resolves `host` to an IPv4 address (network byte order) via gethostbyname(), or 0 on failure.
unsigned long ResolveHost(char *host)
{
    hostent *he = gethostbyname(host);
    if (he == 0) {
        LogPrint("Network error: could not resolve hostname!\r\n");
        return 0;
    }
    if (he->h_addr_list != 0 && he->h_addr_list[0] != 0) {
        return *(unsigned long *)he->h_addr_list[0];
    }
    return 0;
}

// Fills in `addr` for connecting to `host`:`port`.
void MakeSockAddr(sockaddr_in *addr, char *host, unsigned short port)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = ResolveHost(host);
}

// Starts Winsock, resolves `host` and opens a TCP connection to it on port 80.
// Stores the socket in g_socket. Returns false and logs on any failure.
bool NetConnect(char *host)
{
    if (WSAStartup(2, &g_wsaData) == 0) {
        if (LOBYTE(g_wsaData.wVersion) < 2) {
            LogPrint("Network error: Required winsocket version not supported!\r\n");
            return false;
        }
    } else {
        LogPrint("Network error: WinSocket startup failed!!\r\n");
        return false;
    }
    MakeSockAddr(&g_sockAddr, host, 80);
    g_socket = socket(2, 1, 6);
    if (g_socket == -1) {
        LogPrint("Network error: could not create socket!\r\n");
        return false;
    }
    if (connect(g_socket, (sockaddr *)&g_sockAddr, 16) != 0) {
        LogPrint("Network error: could not connect!\r\n");
        return false;
    }
    return true;
}

// Reads from g_socket into `buf` (up to `size` bytes) until the peer closes the
// connection or the buffer fills, NUL-terminating as it goes. Returns bytes received.
int NetRecv(char *buf, int size)
{
    int n = 0;
    int total = 0;
    int left = size;

    while (1) {
        n = recv(g_socket, buf, left - 1, 0);
        if (n == 0) {
            break;
        } else if (n == -1) {
            LogPrint("Network error: socket error while receiving!\r\n");
            return 0;
        } else {
            total += n;
            left -= n;
            // NOTE: off-by-one — this writes the terminator one byte past `total`,
            // not at buf[total]; kept because it matches the original.
            buf[total + 1] = 0;
            if (left <= 0) {
                break;
            }
            buf += n;
        }
    }
    return total;
}

// Sends the whole NUL-terminated string `s` on g_socket.
bool NetSend(char *s)
{
    if (send(g_socket, s, StrLen(s), 0) == -1) {
        LogPrint("Network error: Can not write data !\r\n");
        return false;
    }
    return true;
}

// Closes g_socket and shuts down Winsock.
void NetClose()
{
    LogPrint("Closing TCP connection !\r\n");
    closesocket(g_socket);
    WSACleanup();
}

// Parses a version string like "1.23" into a float: the first digit is the integer
// part, every digit after it (decimal points are just skipped) is the next decimal place.
float ParseVersion(char *s)
{
    int i = 0;
    float v = 0.0f;
    float scale = 1.0f;
    while (s[i] != 0) {
        if (s[i] >= '0' && s[i] <= '9') {
            v = (s[i] - '0') * scale + v;
            scale = scale / 10.0;
            i++;
        } else {
            i++;
        }
    }
    return v;
}

// Fetches the version tag from the Warblade website and sets g_newVersion / opens the
// login window if a newer version than g_gameVersion is advertised. Retries the
// synchronous call if the asynchronous one returns nothing.
void CheckVersion()
{
    KWeb *http = new KWeb;
    char *data = 0;
    g_newVersion = 0;
    if (http != 0) {
        sprintf(g_logBuf, "http://www.warblade.as/tcp_checkversion.asp");
        data = http->callURL(g_logBuf, true);
        if (data == 0) {
            data = http->callURL(g_logBuf, false);
        }
        if (data != 0) {
            LogPrint("Checking for new version !\r\n");
            if (FindTag(data, 0x800, "WARBLADE VERSION:", g_newsBuf, 0x400)) {
                float ver = ParseVersion(g_newsBuf);
                if (ver > g_gameVersion) {
                    g_newVersion = 1;
                    g_loginWinOpen = 1;
                }
            }
        }
        delete http;
    }
}

// No-op hook, called before following a URL/link from the UI.
void BeforeOpenLink()
{
}
