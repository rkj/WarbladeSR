// A stand-in for the game in the browser tests (tests/README.md). Built with the same
// Emscripten options as warblade.js (CMakeLists.txt), it lets the tests run the page's real
// file-system and IndexedDB code without the Warblade data.
//
// Like the game, it reads data/warblade.pac and keeps its files in /save/warblade: it counts
// its runs in a profile file, prints what it found, and keeps running until the page closes.
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <emscripten.h>

#define PROFILE "/save/warblade/profiles/profile000.acc"

static int s_settingsWrites;

// The page calls this when it's hidden or closed, and every few seconds (web/page.js).
EMSCRIPTEN_KEEPALIVE void WebSaveSettings(void)
{
    FILE *f = fopen("/save/warblade/WarBlade.inf", "w");
    if (f) {
        fprintf(f, "settings %d\n", ++s_settingsWrites);
        fclose(f);
    }
}

int main(void)
{
    char pac[64] = "";
    int runs = 0;
    FILE *f;

    if ((f = fopen("data/warblade.pac", "rb")) != NULL) {
        size_t n = fread(pac, 1, sizeof(pac) - 1, f);
        pac[n] = 0;
        pac[strcspn(pac, "\r\n")] = 0;
        fclose(f);
    }
    if ((f = fopen(PROFILE, "r")) != NULL) {
        if (fscanf(f, "%d", &runs) != 1)
            runs = 0;
        fclose(f);
    }
    runs++;
    mkdir("/save/warblade", 0755);
    mkdir("/save/warblade/profiles", 0755);
    if ((f = fopen(PROFILE, "w")) != NULL) {
        fprintf(f, "%d\n", runs);
        fclose(f);
    }
    printf("STUB pac=%s runs=%d\n", pac, runs);
    for (;;)
        emscripten_sleep(100);
}
