// music.cpp: Music: the playlist, BASS streams and modules, volume, and every track switch (title,
// game, boss, stages, shop, hiscore, end).
#include <stdio.h>
#include <io.h>
#include "globals.h"
#include "game.h"
#include "bass.h"

// g_songPath's buffer size: a playlist.m3u line longer than this is rejected.
enum { PLAYLIST_LINE_MAX = 0x400 };


// Frees the loaded playlist.m3u buffer, if any, and clears g_playlistBuf.
void FreePlaylist()
{
    if (g_playlistBuf != 0) {
        free(g_playlistBuf);
        g_playlistBuf = 0;
    }
}

// Reads the user's playlist.m3u (up to 1 MB) from <user folder>\warblade\playlist.m3u
// into g_playlistBuf. Frees the buffer if the file can't be opened.
void LoadPlaylist()
{
    int fd;
    char path[512];
    g_playlistBuf = (char *)malloc(0x100000);          // 1 MB playlist buffer
    if (g_playlistBuf == 0)
        return;
    g_fileModeFlag = 0x8000;                            // _fmode = _O_BINARY (default mode for _open())
    sprintf(path, "%s\\warblade\\playlist.m3u", KMiscTools::getUserFolder());
    fd = _open(path, 0, 0);
    if (fd != -1) {
        g_playlistSize = _read(fd, g_playlistBuf, 0x100000);
        _close(fd);
    } else {
        FreePlaylist();
    }
}

// Splits the loaded playlist buffer into lines: g_playlistLines[i] points at the start of
// line i (right after each '\n'), up to MAX_PLAYLIST_LINES lines. Returns the line count, or 0 if no
// playlist is loaded.
int ParsePlaylist()
{
    if (g_playlistBuf == 0)
        return 0;
    char *p = g_playlistBuf;
    int pos = 0;
    int count = 0;
    int i;
    for (i = 0; i < MAX_PLAYLIST_LINES; i++)
        g_playlistLines[i] = 0;
    do {
        if (*p == '\n' && count < MAX_PLAYLIST_LINES) {
            g_playlistLines[count] = p + 1;
            count++;
        }
        p++;
        pos++;
    } while (pos < g_playlistSize);
    return count;
}

// Picks the name of the next song to play and logs "OPENING SONG <name>", without
// actually opening it (StartMusic() does that). g_musicMode selects the source: 1 is the
// custom song list (g_songNames, shuffled or sequential via g_cfg.shuffle); every other
// mode uses one of the fixed g_str* name globals (T/I/B/N/M/P/D/S/H/E). Always returns true.
bool OpenNextSong()
{
    bool ret = true;
    char *name = 0;

    if (g_musicMode == MUSIC_TITLE)
        name = g_strT;

    if (g_musicPos == 0) {
        if (g_cfg.shuffle) {
            g_curSong = RandRange(0, g_songCount);
            if (g_musicMode == MUSIC_CUSTOM)
                name = g_songNames[g_curSong];
        } else {
            g_songIndex++;
            if (g_songIndex >= g_songCount)
                g_songIndex = 0;
            g_curSong = g_songIndex;
            if (g_musicMode == MUSIC_CUSTOM)
                name = g_songNames[g_curSong];
        }
    } else if (g_musicMode == MUSIC_CUSTOM) {
        name = g_songNames[g_curSong];
    }

    if (g_musicMode == MUSIC_TIMETRIAL)
        name = g_strI;
    if (g_musicMode == MUSIC_BOSS)
        name = g_strB;
    if (g_musicMode == MUSIC_MEMORY)
        name = g_strN;
    if (g_musicMode == MUSIC_METEOR)
        name = g_strM;
    if (g_musicMode == MUSIC_PROMOTED)
        name = g_strP;
    if (g_musicMode == MUSIC_GEM_DROP)
        name = g_strD;
    if (g_musicMode == MUSIC_SHOP)
        name = g_strS;
    if (g_musicMode == MUSIC_HISCORE)
        name = g_strH;
    if (g_musicMode == MUSIC_END)
        name = g_strE;

    g_musicPlaying = 0;
    g_playlistIdx = -1;
    sprintf(g_logBuf, "OPENING SONG %s\r\n", name);
    LogPrint(g_logBuf);
    return ret;
}

// Picks a random non-empty playlist line, copies its path into g_songPath, and if it
// ends in .mp3 or .ogg streams and plays it via BASS (fading volume in over 8 ms). Tries
// up to 10 random lines before giving up. Always returns 0 (the play/stream handle ends
// up in g_musicStream, not in the return value).
int PlayRandomPlaylistSong()
{
    int result = 0;
    char *p = 0;
    char *found = 0;
    int unused = 0;
    int idx = 0;
    BASS_QWORD len = 0;
    bool played = false;
    bool ok = false;
    int tries = 0;
    do {
        do {
            idx = RandRange(0, g_playlistCount);
        } while (g_playlistLines[idx] == 0);
        p = g_playlistLines[idx];

        // Copy the line into g_songPath, stopping at the newline.
        int i = 0;
        bool done = false;
        do {
            g_songPath[i] = *p;
            if (g_songPath[i] == '\n' || g_songPath[i] == '\r') {
                g_songPath[i] = 0;
                done = true;
            }
            i++;
            if (i >= PLAYLIST_LINE_MAX)   // g_songPath is 1024 bytes; bail out if the line is too long
                return 0;
            p++;
        } while (!done);

        found = StrContains(StrLower(g_songPath), ".mp3");
        if (found == 0)
            found = StrContains(StrLower(g_songPath), ".ogg");

        // Only .mp3/.ogg lines are playable; skip anything else and try another random line.
        if (found != 0) {
            sprintf(g_logBuf, "Trying to open mp3:%s\n", g_songPath);
            LogPrint(g_logBuf);
            g_musicStream = BASS_StreamCreateFile(0, g_songPath, 0, 0, BASS_STREAM_PRESCAN | BASS_SAMPLE_LOOP);

            if (g_musicStream != 0) {
                played = BASS_ChannelPlay(g_musicStream, 1) ? true : false;
                if (played) {
                    BASS_ChannelSetAttribute(g_musicStream, BASS_ATTRIB_VOL, 0.0f);
                    BASS_ChannelSlideAttribute(g_musicStream, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0, 8);
                    ok = true;
                    len = BASS_ChannelGetLength(g_musicStream, BASS_POS_BYTE);
                    g_songLengthMs = (BASS_DWORD)(BASS_ChannelBytes2Seconds(g_musicStream, len) * 1000.0);
                    if (g_songLengthMs == -1) {
                        g_songLengthMs = 1800000;
                        g_songEndTime = g_time + 100;
                    } else {
                        g_songEndTime = g_time + g_songLengthMs;
                    }
                }
            }
        }
        tries++;
    } while (tries < 10 && !ok);
    return result;
}

// Stops a BASS channel if the handle is non-zero.
void StopStream(BASS_DWORD h)
{
    if (h != 0)
        BASS_ChannelStop(h);
}

// Stops the current playlist stream, if any, and starts a new random one (only used
// when a playlist.m3u is loaded; see StartMusic()).
void PlayNextMusic()
{
    if (g_playlistBuf != 0) {
        if (g_curStream != 0)
            StopStream(g_curStream);
        g_curStream = PlayRandomPlaylistSong();
    }
}

// Re-applies g_cfg.musicVolume to whichever music channel (module or stream) is
// currently active.
void ApplyMusicVolume()
{
    if (g_soundEnabled != 0) {
        if (g_musicHandle != 0)
            BASS_ChannelSetAttribute(g_musicHandle, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0);
        if (g_musicStream != 0)
            BASS_ChannelSetAttribute(g_musicStream, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0);
    }
}

// Rebuilds g_musVolTable[0..255] as a linear scale of `vol`/255 (used by SoundPlayVoice()).
void SetMusicVolTable(int vol)
{
    float scale = vol / 255.0;
    int i;
    for (i = 0; i < 256; i++)
        g_musVolTable[i] = (int)(i * scale);
}

// Stops whatever is currently playing and starts the next track, picking the source in
// priority order: a loaded playlist.m3u when g_cfg.musicFormat == MUSIC_FMT_PLAYLIST (via PlayNextMusic()),
// else the user's custom song list when g_customSongs is set (BASS_MusicLoad, falling
// back to BASS_StreamCreateFile), else data\music\<g_songName>.mp3 when
// g_cfg.musicFormat == MUSIC_FMT_MP3, else data\music\<g_songName>.mus (a BASS module) otherwise.
// Volume fades in over 8 ms (or a much quieter fade when g_state == STATE_PAUSED: 0.05x the
// normal target). A saved g_musicPos seeks into the custom song once
// it starts playing, then is cleared.
void StartMusic()
{
    char *name;
    bool played;
    BASS_QWORD len;
    if (g_soundEnabled != 0) {
        g_songLengthMs = 0;
        g_songEndTime = 0;
        // ---- external playlist ----
        if (g_playlistBuf != 0 && g_cfg.musicFormat == MUSIC_FMT_PLAYLIST) {
            if (g_musicHandle != 0) {
                BASS_ChannelStop(g_musicHandle);
                BASS_MusicFree(g_musicHandle);
                g_musicHandle = 0;
            }
            if (g_musicStream != 0) {
                BASS_ChannelStop(g_musicStream);
                BASS_StreamFree(g_musicStream);
                g_musicStream = 0;
            }
            PlayNextMusic();

        } else {
            if (g_curStream != 0) {
                StopStream(g_curStream);
                g_curStream = 0;
            }

            // ---- custom song list (pick, start, seek, fade) ----
            if (g_customSongs) {
                // Pick the next song name from the custom song list (or a fixed g_str* name).
                name = 0;
                if (g_musicMode == MUSIC_TITLE)
                    name = g_strT;
                if (g_musicPos == 0) {
                    if (g_cfg.shuffle) {
                        g_curSong = RandRange(0, g_songCount);
                        if (g_musicMode == MUSIC_CUSTOM)
                            name = g_songNames[g_curSong];
                    } else {
                        g_songIndex++;
                        if (g_songIndex >= g_songCount)
                            g_songIndex = 0;
                        g_curSong = g_songIndex;
                        if (g_musicMode == MUSIC_CUSTOM)
                            name = g_songNames[g_curSong];
                    }
                } else if (g_musicMode == MUSIC_CUSTOM) {
                    name = g_songNames[g_curSong];
                }

                if (g_musicMode == MUSIC_TIMETRIAL)
                    name = g_strI;
                if (g_musicMode == MUSIC_BOSS)
                    name = g_strB;
                if (g_musicMode == MUSIC_MEMORY)
                    name = g_strN;
                if (g_musicMode == MUSIC_METEOR)
                    name = g_strM;
                if (g_musicMode == MUSIC_PROMOTED)
                    name = g_strP;
                if (g_musicMode == MUSIC_GEM_DROP)
                    name = g_strD;
                if (g_musicMode == MUSIC_SHOP)
                    name = g_strS;
                if (g_musicMode == MUSIC_HISCORE)
                    name = g_strH;
                if (g_musicMode == MUSIC_END)
                    name = g_strE;

                // Stop whatever was playing before, then open and start the picked song.
                if (g_musicHandle != 0) {
                    BASS_ChannelStop(g_musicHandle);
                    BASS_MusicFree(g_musicHandle);
                    g_musicHandle = 0;
                }
                if (g_musicStream != 0) {
                    BASS_ChannelStop(g_musicStream);
                    BASS_StreamFree(g_musicStream);
                    g_musicStream = 0;
                }
                g_musicPlaying = 0;
                sprintf(g_logBuf, "OPENING SONG %s\r\n", name);
                LogPrint(g_logBuf);
                g_musicStream = 0;
                g_musicHandle = 0;
                g_musicPlaying = 0;
                g_musicHandle = BASS_MusicLoad(0, name, 0, 0, BASS_MUSIC_PRESCAN | BASS_MUSIC_LOOP, 0);
                if (g_musicHandle == 0)
                    g_musicStream = BASS_StreamCreateFile(0, name, 0, 0, BASS_STREAM_PRESCAN | BASS_SAMPLE_LOOP);

                if (g_musicHandle != 0) {
                    played = BASS_ChannelPlay(g_musicHandle, 1) ? true : false;
                    if (played) {
                        BASS_ChannelSetAttribute(g_musicHandle, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicHandle, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0, 8);
                        len = BASS_ChannelGetLength(g_musicHandle, BASS_POS_BYTE);
                        g_songLengthMs = (BASS_DWORD)(BASS_ChannelBytes2Seconds(g_musicHandle, len) * 1000.0);
                        g_songEndTime = g_time + g_songLengthMs;
                        g_musicPlaying = 1;
                    }
                }

                if (g_musicStream != 0) {
                    played = BASS_ChannelPlay(g_musicStream, 1) ? true : false;
                    if (played) {
                        BASS_ChannelSetAttribute(g_musicStream, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicStream, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0, 8);
                        len = BASS_ChannelGetLength(g_musicStream, BASS_POS_BYTE);
                        g_songLengthMs = (BASS_DWORD)(BASS_ChannelBytes2Seconds(g_musicStream, len) * 1000.0);
                        g_songEndTime = g_time + g_songLengthMs;
                        g_musicPlaying = 1;
                    }
                }

                // A saved position (from a previous session) seeks into the custom song once it starts.
                if (g_musicStream != 0 && g_musicPos != 0 && g_musicMode == MUSIC_CUSTOM) {
                    BASS_ChannelSetPosition(g_musicStream, g_musicPos, BASS_POS_BYTE);
                    len = BASS_ChannelGetLength(g_musicStream, BASS_POS_BYTE);
                    g_songLengthMs = (BASS_DWORD)(BASS_ChannelBytes2Seconds(g_musicStream, len) * 1000.0);
                    g_songEndTime = g_time + g_songLengthMs;
                    g_musicPos = 0;
                }

                // Re-apply the fade-in target volume: much quieter while g_state == STATE_PAUSED.
                if (g_musicHandle != 0) {
                    if (g_state == STATE_PAUSED) {
                        BASS_ChannelSetAttribute(g_musicHandle, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicHandle, BASS_ATTRIB_VOL,
                                                      g_cfg.musicVolume * (double)0.05f / 255.0, 8);
                    } else {
                        BASS_ChannelSetAttribute(g_musicHandle, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicHandle, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0, 8);
                    }
                }
                if (g_musicStream != 0) {
                    if (g_state == STATE_PAUSED) {
                        BASS_ChannelSetAttribute(g_musicStream, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicStream, BASS_ATTRIB_VOL,
                                                      g_cfg.musicVolume * (double)0.05f / 255.0, 8);
                    } else {
                        BASS_ChannelSetAttribute(g_musicStream, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicStream, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0, 8);
                    }
                }

            // ---- fixed track, .mp3 ----
            } else if (g_cfg.musicFormat == MUSIC_FMT_MP3) {
                if (g_musicHandle != 0) {
                    BASS_ChannelStop(g_musicHandle);
                    BASS_MusicFree(g_musicHandle);
                    g_musicHandle = 0;
                }
                if (g_musicStream != 0) {
                    BASS_ChannelStop(g_musicStream);
                    BASS_StreamFree(g_musicStream);
                    g_musicStream = 0;
                }
                g_musicPlaying = 0;
                g_musicStream = BASS_StreamCreateFile(0,
                    Concat3(KMiscTools::makeFilePath("data\\music\\"), g_songName, ".mp3"), 0, 0, BASS_STREAM_PRESCAN | BASS_SAMPLE_LOOP);

                if (g_musicStream != 0) {
                    played = BASS_ChannelPlay(g_musicStream, 1) ? true : false;
                    if (played) {
                        BASS_ChannelSetAttribute(g_musicStream, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicStream, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0, 8);
                        len = BASS_ChannelGetLength(g_musicStream, BASS_POS_BYTE);
                        g_songLengthMs = (BASS_DWORD)(BASS_ChannelBytes2Seconds(g_musicStream, len) * 1000.0);
                        g_songEndTime = g_time + g_songLengthMs;
                        g_musicPlaying = 1;
                    }
                }

                if (g_musicStream != 0 && g_musicPos != 0 && g_musicMode == MUSIC_CUSTOM) {
                    BASS_ChannelSetPosition(g_musicStream, g_musicPos, BASS_POS_BYTE);
                    len = BASS_ChannelGetLength(g_musicStream, BASS_POS_BYTE);
                    g_songLengthMs = (BASS_DWORD)(BASS_ChannelBytes2Seconds(g_musicStream, len) * 1000.0);
                    g_songEndTime = g_time + g_songLengthMs;
                    g_musicPos = 0;
                }
                if (g_musicStream != 0) {
                    if (g_state == STATE_PAUSED) {
                        BASS_ChannelSetAttribute(g_musicStream, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicStream, BASS_ATTRIB_VOL,
                                                      g_cfg.musicVolume * (double)0.05f / 255.0, 8);
                    } else {
                        BASS_ChannelSetAttribute(g_musicStream, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicStream, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0, 8);
                    }
                }

            // ---- fixed track, .mus module ----
            } else {
                if (g_musicHandle != 0) {
                    BASS_ChannelStop(g_musicHandle);
                    BASS_MusicFree(g_musicHandle);
                    g_musicHandle = 0;
                }
                if (g_musicStream != 0) {
                    BASS_ChannelStop(g_musicStream);
                    BASS_StreamFree(g_musicStream);
                    g_musicStream = 0;
                }
                g_musicPlaying = 0;
                g_musicHandle = BASS_MusicLoad(0,
                    Concat3(KMiscTools::makeFilePath("data\\music\\"), g_songName, ".mus"), 0, 0, BASS_MUSIC_PRESCAN | BASS_MUSIC_LOOP, 0);

                if (g_musicHandle != 0) {
                    played = BASS_ChannelPlay(g_musicHandle, 1) ? true : false;
                    if (played) {
                        BASS_ChannelSetAttribute(g_musicHandle, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicHandle, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0, 8);
                        len = BASS_ChannelGetLength(g_musicHandle, BASS_POS_BYTE);
                        g_songLengthMs = (BASS_DWORD)(BASS_ChannelBytes2Seconds(g_musicHandle, len) * 1000.0);
                        g_songEndTime = g_time + g_songLengthMs;
                        g_musicPlaying = 1;
                    }
                }

                if (g_musicHandle != 0 && g_musicPos != 0 && g_musicMode == MUSIC_CUSTOM) {
                    BASS_ChannelSetPosition(g_musicHandle, g_musicPos, BASS_POS_BYTE);
                    len = BASS_ChannelGetLength(g_musicHandle, BASS_POS_BYTE);
                    g_songLengthMs = (BASS_DWORD)(BASS_ChannelBytes2Seconds(g_musicHandle, len) * 1000.0);
                    g_songEndTime = g_time + g_songLengthMs;
                    g_musicPos = 0;
                }
                if (g_musicHandle != 0) {
                    if (g_state == STATE_PAUSED) {
                        BASS_ChannelSetAttribute(g_musicHandle, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicHandle, BASS_ATTRIB_VOL,
                                                      g_cfg.musicVolume * (double)0.05f / 255.0, 8);
                    } else {
                        BASS_ChannelSetAttribute(g_musicHandle, BASS_ATTRIB_VOL, 0.0f);
                        BASS_ChannelSlideAttribute(g_musicHandle, BASS_ATTRIB_VOL, g_cfg.musicVolume / 255.0, 8);
                    }
                }
            }
        }
    }
}

// Refreshes g_musicPos from the currently playing BASS music/stream handle. Called once per
// frame while music is enabled.
void UpdateMusicPos()
{
    if (g_soundEnabled != 0 && g_musicMode == 1) {
        if (g_musicHandle != 0)
            g_musicPos = BASS_ChannelGetPosition(g_musicHandle, 0);
        if (g_musicStream != 0)
            g_musicPos = BASS_ChannelGetPosition(g_musicStream, 0);
    }
}

// Cycles the music format setting (off -> module/tracker -> streamed, or skips streamed if no playlist is
// loaded) and restarts playback; bound to the music-format option in the config menu.
void CycleMusicFormat()
{
    if (g_cfg.musicFormat == 0) {
        g_cfg.musicFormat = 1;
        LoadProfile();
    } else if (g_cfg.musicFormat == 1) {
        if (g_playlistBuf != 0)
            g_cfg.musicFormat = 2;
        else
            g_cfg.musicFormat = 0;
    } else {
        g_cfg.musicFormat = 0;
    }
    StartMusic();
}

// Starts the main gameplay track ("warblade"), or the time-trial track instead when in time-trial
// mode. No-op when an external playlist (music format 2) is active.
void PlayGameMusic()
{
    if (g_cfg.musicFormat == MUSIC_FMT_PLAYLIST && g_playlistCount != 0)
        return;
    g_songName = "warblade";
    g_musicMode = MUSIC_CUSTOM;
    if (g_gameMode == MODE_TIME_TRIAL) {
        g_songName = "timetrial";
        g_musicMode = MUSIC_TIMETRIAL;
    }
    StartMusic();
}

// Starts the title-screen music track. No-op when an external playlist is active.
void PlayTitleMusic()
{
    if (g_cfg.musicFormat == MUSIC_FMT_PLAYLIST && g_playlistCount != 0)
        return;
    g_songName = "title";
    g_musicMode = MUSIC_TITLE;
    StartMusic();
}

// Unused level-end/music callback slot; intentionally empty.
void EmptyMusicStubA()
{
}

// Starts the time-trial music track. No-op when an external playlist is active.
void PlayTimeTrialMusic()
{
    if (g_cfg.musicFormat == MUSIC_FMT_PLAYLIST && g_playlistCount != 0)
        return;
    g_songName = "timetrial";
    g_musicMode = MUSIC_TIMETRIAL;
    StartMusic();
}

// Unused level-end/music callback slot; intentionally empty.
void EmptyMusicStubB()
{
}

// Common body of the Play<X>Music() wrappers that flush the current playback position
// first: bail out if an external playlist is active, else switch to track `songName`/`mode`.
#define PLAY_MUSIC_TRACK(songName, mode) \
    if (g_cfg.musicFormat == MUSIC_FMT_PLAYLIST && g_playlistCount != 0) \
        return;                                                          \
    UpdateMusicPos();                                                    \
    g_songName = songName;                                               \
    g_musicMode = mode;                                                  \
    StartMusic();

// Starts the boss music track, first flushing the current playback position. No-op when an
// external playlist is active.
void PlayBossMusic()
{
    PLAY_MUSIC_TRACK("boss", MUSIC_BOSS);
}

// Unused level-end/music callback slot; intentionally empty.
void EmptyMusicStubC()
{
}

// Starts the memory (card-grid) bonus level music track, first flushing the current playback
// position. No-op when an external playlist is active.
void PlayMemoryStationMusic()
{
    PLAY_MUSIC_TRACK("memory", MUSIC_MEMORY);
}

// Starts the hi-score-entry music track. No-op when an external playlist is active.
void PlayHiscoreMusic()
{
    if (g_cfg.musicFormat == MUSIC_FMT_PLAYLIST && g_playlistCount != 0)
        return;
    g_songName = "hiscore";
    g_musicMode = MUSIC_HISCORE;
    StartMusic();
}

// Switches to the title music if some other track is currently playing; does nothing if the
// title track is already playing.
void EnsureTitleMusic()
{
    if (g_musicMode != 0)
        PlayTitleMusic();
}

// Starts the end-game music track, first flushing the current playback position. No-op when an
// external playlist is active.
void PlayEndMusic()
{
    PLAY_MUSIC_TRACK("end", MUSIC_END);
}

// Unused level-end/music callback slot; intentionally empty.
void EmptyMusicStubD()
{
}

// Starts the shop music track, first flushing the current playback position. No-op when an
// external playlist is active.
void PlayShopMusic()
{
    PLAY_MUSIC_TRACK("shop", MUSIC_SHOP);
}

// Unused level-end/music callback slot; intentionally empty.
void EmptyMusicStubE()
{
}

// Starts the meteor bonus level music track, first flushing the current playback position. No-op
// when an external playlist is active.
void PlayMeteorStormMusic()
{
    PLAY_MUSIC_TRACK("meteor", MUSIC_METEOR);
}

// Starts the Gem Drop bonus level music track, first flushing the current playback position.
// No-op when an external playlist is active.
void PlayGemDropMusic()
{
    PLAY_MUSIC_TRACK("gems", MUSIC_GEM_DROP);
}

#undef PLAY_MUSIC_TRACK
