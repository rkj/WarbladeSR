// sound.c: Sound effects: samples, the delayed voice queue, pending slots, loading voices,
// volume and pan tables, UI sounds.
#include <stdio.h>
#include <io.h>
#include "globals.h"
#include "game.h"
#include "sdlhelp.h"


// True if voice pack `voice` has a `wv.id` file on disk.
bool VoiceExists(int voice)
{
    int fd = 0;
    // NOTE: both branches build the same path; kept as-is for the byte match.
    if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) && g_playerUpdateFn != StateDemo)
        sprintf(g_pathBuf, SysAppPath("data\\samples\\voices\\%d\\wv.id"), voice);
    else
        sprintf(g_pathBuf, SysAppPath("data\\samples\\voices\\%d\\wv.id"), voice);
    bool ok = false;
    fd = _open(g_pathBuf, 0, 0);
    if (fd != -1) {
        ok = true;
        _close(fd);
    }
    return ok;
}

// Builds the path to `name` inside the active voice pack (profile's voice if a profile is
// active and not in demo playback, else the config's voice) into `g_pathBuf`.
char *VoicePath(const char *name)
{
    if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) && g_playerUpdateFn != StateDemo)
        sprintf(g_pathBuf, SysAppPath("data\\samples\\voices\\%d\\%s"),
                GetProfileVoiceIndex(g_profileIndex), name);
    else
        sprintf(g_pathBuf, SysAppPath("data\\samples\\voices\\%d\\%s"), g_cfg.voice, name);
    return g_pathBuf;
}

// Builds the path to sample `name` under `data\samples\` into `g_pathBuf`.
char *SamplePath(const char *name)
{
    // NOTE: both branches build the same path; kept as-is for the byte match.
    if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) && g_playerUpdateFn != StateDemo)
        sprintf(g_pathBuf, SysAppPath("data\\samples\\%s"), name);
    else
        sprintf(g_pathBuf, SysAppPath("data\\samples\\%s"), name);
    return g_pathBuf;
}

// Fills the 10-entry g_sampleHandle/g_sampleRate/g_sampleVol table used to play a
// randomly-picked alien shot sound at a fixed pitch and volume (see wherever that table
// is read for the actual random pick). Values are hand-tuned by the original developer.
void InitSampleTable()
{
    g_sampleHandle[0] = g_sfxSingleShot;
    g_sampleRate[0] = 44100;
    g_sampleVol[0] = 255;

    g_sampleHandle[1] = g_sfxAlienShoot2;
    g_sampleRate[1] = 25000;
    g_sampleVol[1] = 180;

    g_sampleHandle[2] = g_sfxAlienShoot16;
    g_sampleRate[2] = 22000;
    g_sampleVol[2] = 220;

    g_sampleHandle[3] = g_sfxAlienShoot12Alt;
    g_sampleRate[3] = 32000;
    g_sampleVol[3] = 150;

    g_sampleHandle[4] = g_sfxAlienShoot3;
    g_sampleRate[4] = 32000;
    g_sampleVol[4] = 128;

    g_sampleHandle[5] = g_sfxAlienShoot9;
    g_sampleRate[5] = 17500;
    g_sampleVol[5] = 128;

    g_sampleHandle[6] = g_sfxAlienShoot17;
    g_sampleRate[6] = 28000;
    g_sampleVol[6] = 128;

    g_sampleHandle[7] = g_sfxLaser2;
    g_sampleRate[7] = 24000;
    g_sampleVol[7] = 120;

    g_sampleHandle[8] = g_sfxAlienShoot15;
    g_sampleRate[8] = 44100;
    g_sampleVol[8] = 255;

    g_sampleHandle[9] = g_sfxAlienShoot4;
    g_sampleRate[9] = 20000;
    g_sampleVol[9] = 200;
}

// Rebuilds g_sfxVolTable[0..255] as a linear scale of `vol`/255, then re-applies the
// scaled volume (at index 200, or 255 for the ship hum) to whichever looping sfx
// channels are currently playing.
void SetSfxVolume(int vol)
{
    float scale = vol / 255.0;
    int i;
    for (i = 0; i < 256; i++)
        g_sfxVolTable[i] = (int)(i * scale);
    if (g_sfxScopeHum != 0)
        ChanSet(g_sfxScopeHum, AUDIO_VOL, g_sfxVolTable[200] / 255.0);
    if (g_sfxShieldHum != 0)
        ChanSet(g_sfxShieldHum, AUDIO_VOL, g_sfxVolTable[200] / 255.0);
    if (g_sfxMothership != 0)
        ChanSet(g_sfxMothership, AUDIO_VOL, g_sfxVolTable[200] / 255.0);
    if (g_sfxGuardLoop != 0)
        ChanSet(g_sfxGuardLoop, AUDIO_VOL, g_sfxVolTable[200] / 255.0);
    if (g_sfxBossLoop != 0)
        ChanSet(g_sfxBossLoop, AUDIO_VOL, g_sfxVolTable[200] / 255.0);
    if (g_sfxShipHumLoop != 0)
        ChanSet(g_sfxShipHumLoop, AUDIO_VOL, g_sfxVolTable[255] / 255.0);
}

// Builds two per-pixel lookup tables sized to the screen: g_panTable[0..screenW-1] maps
// x to a 0..255 stereo pan value (used to pan sound effects by their on-screen x
// position), and g_rampB[0..screenH-1] maps y to a 100..255 brightness ramp that flattens
// out to 255 for the bottom 100 rows.
void BuildRampTables()
{
    int i;
    float xf;
    int j;
    float yf;

    for (i = 0; i < (int)g_screenW; i++) {
        xf = (float)i;
        g_panTable[i] = 255.0 / g_screenW * xf;
    }
    for (j = 0; j < (int)g_screenH; j++) {
        yf = (float)j;
        if (j < (int)g_screenH - 100)
            g_rampB[j] = (int)(yf * (155.0 / (g_screenH - 100)) + 100.0);
        else
            g_rampB[j] = 255;
    }
}

//
// ChanSet/ChanSlide use AUDIO_FREQ (Hz), AUDIO_VOL (0..1) and AUDIO_PAN (-1..1) throughout
// this file. `vol` parameters are indices into g_sfxVolTable/g_musVolTable (0..255),
// converted to a 0..1 volume by /255.0.

// Clears the delayed announcer-voice queue and the six pending-sound slots.
void SoundResetQueue()
{
    g_soundQueueCount = 0;
    g_pend0 = 0;
    g_pend1 = 0;
    g_pend2 = 0;
    g_pend3 = 0;
    g_pend4 = 0;
    g_pend5 = 0;
}

// Returns the playback length of a loaded sample, in milliseconds.
int SampleLengthMs(AudioHandle sample)
{
    return (int)(ChanLength(sample) * 1000.0);
}

// Queues a sample to play after the ones already queued (announcer voices), `delay` ms
// after the previous one ends. `vol` > 0 queues it even when the queue is busy.
// NOTE: the `vol == 0` and `vol > 0` branches below duplicate the same enqueue logic;
// kept as-is for the byte match.
void SoundQueueAdd(AudioHandle sample, int delay, int vol)
{
    if (g_soundEnabled && sample && g_cfg.sfxOn && g_soundQueueCount < MAX_SOUND_QUEUE) {
        if (g_soundQueueCount == 0 && vol == 0 && g_time > g_soundQueueNext) {
            g_soundQueue[g_soundQueueCount].sample = sample;
            g_soundQueue[g_soundQueueCount].length = SampleLengthMs(sample);
            g_soundQueue[g_soundQueueCount].vol = vol;
            if (g_soundQueueCount == 0) {
                g_soundQueue[g_soundQueueCount].time = g_time;
                g_soundQueueNext = g_time + g_soundQueue[g_soundQueueCount].length + delay;
            } else {
                g_soundQueue[g_soundQueueCount].time = g_soundQueueNext;
                g_soundQueueNext = g_soundQueue[g_soundQueueCount].time
                    + g_soundQueue[g_soundQueueCount].length + delay;
            }
            g_soundQueueCount++;
        }

        if (vol > 0) {
            g_soundQueue[g_soundQueueCount].sample = sample;
            g_soundQueue[g_soundQueueCount].length = SampleLengthMs(sample);
            g_soundQueue[g_soundQueueCount].vol = vol;
            if (g_soundQueueCount == 0) {
                g_soundQueue[g_soundQueueCount].time = g_time;
                g_soundQueueNext = g_time + g_soundQueue[g_soundQueueCount].length + delay;
            } else {
                g_soundQueue[g_soundQueueCount].time = g_soundQueueNext;
                g_soundQueueNext = g_soundQueue[g_soundQueueCount].time
                    + g_soundQueue[g_soundQueueCount].length + delay;
            }
            g_soundQueueCount++;
        }
    }
}

// Stops a channel if sound is enabled and it's non-zero. Returns 0, so callers can
// write `handle = SoundStop(handle)` to clear the handle in one line.
int SoundStop(AudioHandle ch)
{
    if (g_soundEnabled && ch)
        ChanStop(ch);
    return 0;
}

// Starts pending slot g_pend##n (set up elsewhere) if it holds a sample: applies its
// frequency/volume/pan override, plays it, then clears the slot.
#define PLAY_PENDING(n)                                                                        \
    if (g_soundEnabled && g_pend##n) {                                                         \
        ch = SampleGetVoice(g_pend##n);                                                        \
        if (ch) {                                                                              \
            if (g_pend##n##Freq != -1)                                                         \
                ChanSet(ch, AUDIO_FREQ, (float)g_pend##n##Freq);                                \
            if (g_pend##n##Vol != -1)                                                          \
                ChanSet(ch, AUDIO_VOL, 0.0f);                                                   \
            ChanSet(ch, AUDIO_PAN, g_pend##n##Pan);                                             \
            ChanSlide(ch, AUDIO_VOL,                                                            \
                      (float)(g_sfxVolTable[g_pend##n##Vol] / 255.0), 8);                       \
            ChanPlay(ch, true);                                                                 \
            g_pend##n = 0;                                                                      \
        }                                                                                        \
    }

// Starts the six pending sound slots (g_pend0..g_pend5) set up elsewhere, applying each
// slot's frequency/volume/pan override, then clears the slot. Called once per frame.
void SoundPlayPending()
{
    AudioHandle ch;
    PLAY_PENDING(0);
    PLAY_PENDING(1);
    PLAY_PENDING(2);
    PLAY_PENDING(3);
    PLAY_PENDING(4);
    PLAY_PENDING(5);
}

#undef PLAY_PENDING

// Plays a sound effect on a fresh channel, fading volume in over 8 ms. `freq`/`vol` of
// -1 leave the default; `unused1`/`unused2` are not read.
void SoundPlay(AudioHandle sample, int freq, int vol, float pan, int unused1, int unused2)
{
    AudioHandle ch;
    int err;
    if (g_soundEnabled && sample) {
        ch = SampleGetVoice(sample);
        if (ch) {
            if (freq != -1)
                ChanSet(ch, AUDIO_FREQ, (float)freq);
            if (vol != -1)
                ChanSet(ch, AUDIO_VOL, 0.0f);
            ChanSet(ch, AUDIO_PAN, pan);
            ChanSlide(ch, AUDIO_VOL, (float)(g_sfxVolTable[vol] / 255.0), 8);
            if (!ChanPlay(ch, true)) {
                err = AudioError();
                if (err) {
                    sprintf(g_logBuf, "SOUNDSYSTEM : error code %d\n", err);
                    LogPrint(g_logBuf);
                }
            }
        }
    }
}

// Plays a sound effect at its final volume immediately, without the fade-in of SoundPlay().
void SoundPlayNoFade(AudioHandle sample, int freq, int vol, float pan, int unused1, int unused2)
{
    AudioHandle ch;
    int err;
    if (g_soundEnabled && sample) {
        ch = SampleGetVoice(sample);
        if (ch) {
            if (freq != -1)
                ChanSet(ch, AUDIO_FREQ, (float)freq);
            if (vol != -1)
                ChanSet(ch, AUDIO_VOL, (float)(g_sfxVolTable[vol] / 255.0));
            ChanSet(ch, AUDIO_PAN, pan);
            if (!ChanPlay(ch, true)) {
                err = AudioError();
                if (err) {
                    sprintf(g_logBuf, "SOUNDSYSTEM : error code %d\n", err);
                    LogPrint(g_logBuf);
                }
            }
        }
    }
}

// Like SoundPlay(), but also starts an extra attribute slide (e.g. a pitch or pan
// sweep) on the new channel: `attrib` slides to `value` over `time` ms.
void SoundPlaySlide(AudioHandle sample, int freq, int vol, float pan, int unused1, int unused2,
                     enum AudioAttrib attrib, float value, int time)
{
    AudioHandle ch;
    int err;
    if (g_soundEnabled && sample) {
        ch = SampleGetVoice(sample);
        if (ch) {
            if (freq != -1)
                ChanSet(ch, AUDIO_FREQ, (float)freq);
            if (vol != -1)
                ChanSet(ch, AUDIO_VOL, 0.0f);
            ChanSet(ch, AUDIO_PAN, pan);
            ChanSlide(ch, AUDIO_VOL, (float)(g_sfxVolTable[vol] / 255.0), 8);
            ChanSlide(ch, attrib, value, time);
            if (!ChanPlay(ch, true)) {
                err = AudioError();
                if (err) {
                    sprintf(g_logBuf, "SOUNDSYSTEM : error code %d\n", err);
                    LogPrint(g_logBuf);
                }
            }
        }
    }
}

// Identical to SoundPlay(); a second copy used by another call site.
void SoundPlay2(AudioHandle sample, int freq, int vol, float pan, int unused1, int unused2)
{
    AudioHandle ch;
    int err;
    if (g_soundEnabled && sample) {
        ch = SampleGetVoice(sample);
        if (ch) {
            if (freq != -1)
                ChanSet(ch, AUDIO_FREQ, (float)freq);
            if (vol != -1)
                ChanSet(ch, AUDIO_VOL, 0.0f);
            ChanSet(ch, AUDIO_PAN, pan);
            ChanSlide(ch, AUDIO_VOL, (float)(g_sfxVolTable[vol] / 255.0), 8);
            if (!ChanPlay(ch, true)) {
                err = AudioError();
                if (err) {
                    sprintf(g_logBuf, "SOUNDSYSTEM : error code %d\n", err);
                    LogPrint(g_logBuf);
                }
            }
        }
    }
}

// Plays a voice/announcer sample, fading in over 10 ms using the music volume table
// (g_musVolTable) rather than the sfx one. No error is logged on failure.
void SoundPlayVoice(AudioHandle sample, int freq, int vol, float pan, int unused1, int unused2)
{
    AudioHandle ch;
    if (g_soundEnabled && sample) {
        ch = SampleGetVoice(sample);
        if (ch) {
            if (freq != -1)
                ChanSet(ch, AUDIO_FREQ, (float)freq);
            if (vol != -1)
                ChanSet(ch, AUDIO_VOL, 0.0f);
            ChanSet(ch, AUDIO_PAN, pan);
            ChanSlide(ch, AUDIO_VOL, (float)(g_musVolTable[vol] / 255.0), 10);
            ChanPlay(ch, true);
        }
    }
}

// Stops `prev` (if any), plays `sample` like SoundPlay(), and returns the new channel
// handle so the caller can track/stop it later (e.g. a looping hum).
AudioHandle SoundPlayChannel(AudioHandle prev, AudioHandle sample, int freq, int vol, float pan, int unused)
{
    AudioHandle ch = 0;
    if (g_soundEnabled && sample) {
        if (prev)
            ChanStop(prev);
        ch = SampleGetVoice(sample);
        if (ch) {
            if (freq != -1)
                ChanSet(ch, AUDIO_FREQ, (float)freq);
            if (vol != -1)
                ChanSet(ch, AUDIO_VOL, 0.0f);
            ChanSet(ch, AUDIO_PAN, pan);
            ChanSlide(ch, AUDIO_VOL, (float)(g_sfxVolTable[vol] / 255.0), 8);
            ChanPlay(ch, true);
        }
    }
    return ch;
}

// Called once per frame: when the head of the delayed sound queue is due, plays it and
// shifts the remaining entries down by one slot.
void SoundQueueUpdate()
{
    int i;
    if (g_soundQueueCount > 0 && g_time > g_soundQueue[0].time) {
        if (g_soundEnabled && g_soundQueue[0].sample && g_cfg.sfxOn)
            SoundPlayVoice(g_soundQueue[0].sample, -1, SFX_VOL_FULL, 0.0f, 255, 0);
        for (i = 0; i < MAX_SOUND_QUEUE - 1; i++)
            g_soundQueue[i] = g_soundQueue[i + 1];
        g_soundQueueCount--;
    }
}

// Stops every currently-playing channel: the shield/scope hums and every active voice
// in the 4x150 g_samples channel-tracking grid.
void SoundStopAll()
{
    int i;
    int j;
    g_chanShieldHum = SoundStop(g_chanShieldHum);
    g_chanScopeHum = SoundStop(g_chanScopeHum);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 150; j++) {
            if (g_samples[i][j])
                g_samples[i][j] = SoundStop(g_samples[i][j]);
        }
    }
}

// Stops and restarts audio output (e.g. after a device change).
void SoundRestart()
{
    if (g_soundEnabled) {
        AudioStop();
        AudioStart();
    }
}

// Pauses audio output (e.g. when the game window loses focus).
void SoundPause()
{
    if (g_soundEnabled) {
        AudioPause();
        g_soundPaused = 1;
    }
}

// Resumes audio output after SoundPause().
void SoundResume()
{
    if (g_soundEnabled) {
        AudioStart();
        g_soundPaused = 0;
    }
}

// Loads a one-shot sample from data\samples\<name>, trying .wav, then .mp3, then .ogg.
// `max` is the max simultaneous playbacks. Logs and returns 0 on failure.
// Tries loading `name`.wav, then .mp3, then .ogg from data\samples\ into `sample`, stopping
// at the first that loads (SampleLoad returns 0 on failure).
#define TRY_LOAD_SAMPLE_CASCADE(flags, max)                                                \
    sprintf(g_pathBuf, SysAppPath("data\\samples\\%s.wav"), name);                        \
    sample = SampleLoad(g_pathBuf, max, flags);                                           \
    if (sample == 0) {                                                                     \
        sprintf(g_pathBuf, SysAppPath("data\\samples\\%s.mp3"), name);                    \
        sample = SampleLoad(g_pathBuf, max, flags);                                       \
        if (sample == 0) {                                                                 \
            sprintf(g_pathBuf, SysAppPath("data\\samples\\%s.ogg"), name);                \
            sample = SampleLoad(g_pathBuf, max, flags);                                   \
        }                                                                                    \
    }

AudioHandle LoadSample(const char *name, AudioHandle max)
{
    AudioHandle sample;
    int flags;

    flags = 0;                                   // SampleLoad flags (a busy sample steals its quietest voice)
    TRY_LOAD_SAMPLE_CASCADE(flags, max);
    if (sample == 0) {
        sprintf(g_pathBuf, "SAMPLE LOAD ERROR: %s\r\n", name);
        LogPrint(g_pathBuf);
    }
    return sample;
}

// Like LoadSample(), but loads the sample with the looping flag set (AUDIO_SAMPLE_LOOP
// added to LoadSample()'s flags) for background hums/loops.
AudioHandle LoadSampleLoop(const char *name, AudioHandle max)
{
    AudioHandle sample;
    int flags;

    flags = AUDIO_SAMPLE_LOOP;                      // SampleLoad flags: LoadSample()'s flags plus the loop bit
    TRY_LOAD_SAMPLE_CASCADE(flags, max);
    if (sample == 0) {
        sprintf(g_pathBuf, "SAMPLE LOAD ERROR: %s\r\n", name);
        LogPrint(g_pathBuf);
    }
    return sample;
}

#undef TRY_LOAD_SAMPLE_CASCADE

// Frees a sample handle if it is loaded, and zeroes it.
#define FREE_SAMPLE(h) if (h) { SampleFree(h); (h) = 0; }

// Frees every loaded voice/announcer sample (g_sfx*) and zeroes its handle. Called
// before reloading voices and during shutdown.
void FreeSamples()
{
    FREE_SAMPLE(g_sfxBonus);
    FREE_SAMPLE(g_sfxDoubleShot);
    FREE_SAMPLE(g_sfxExtraBullet);
    FREE_SAMPLE(g_sfxExtraLife);
    FREE_SAMPLE(g_sfxExtraSpeed);

    FREE_SAMPLE(g_sfxExtraTime);
    FREE_SAMPLE(g_sfxGetReady);
    FREE_SAMPLE(g_sfxGetReady2);
    FREE_SAMPLE(g_sfxGetReady3);
    FREE_SAMPLE(g_sfxHurryUp1);

    FREE_SAMPLE(g_sfxHurryUp2);
    FREE_SAMPLE(g_sfxMoney);
    FREE_SAMPLE(g_sfxScoop);
    FREE_SAMPLE(g_sfxShield);
    FREE_SAMPLE(g_sfxSingleShotVoice);

    FREE_SAMPLE(g_sfxSucker);
    FREE_SAMPLE(g_sfxSucker2);
    FREE_SAMPLE(g_sfxSucker3);
    FREE_SAMPLE(g_sfxSuperTripleShot);
    FREE_SAMPLE(g_sfxTripleShot);

    FREE_SAMPLE(g_sfxQuadShot);
    FREE_SAMPLE(g_sfxOops);
    FREE_SAMPLE(g_sfxRankEnsign);
    FREE_SAMPLE(g_sfxRankLieutenant);
    FREE_SAMPLE(g_sfxRankCommander);

    FREE_SAMPLE(g_sfxRankCaptain);
    FREE_SAMPLE(g_sfxRankAdmiral);
    FREE_SAMPLE(g_sfxAlright);
    FREE_SAMPLE(g_sfxCongratulations);
    FREE_SAMPLE(g_sfxArmour);

    FREE_SAMPLE(g_sfxBonusUnused);
    FREE_SAMPLE(g_sfxMemoryStation);
    FREE_SAMPLE(g_sfxMeteorStorm);
    FREE_SAMPLE(g_sfxPlayer1);
    FREE_SAMPLE(g_sfxPlayer2);

    FREE_SAMPLE(g_sfxPlayer);
    FREE_SAMPLE(g_sfxPlayers);
    FREE_SAMPLE(g_sfxWarning);
    FREE_SAMPLE(g_sfxWarpMalfunction);
    FREE_SAMPLE(g_sfxWelcome);

    FREE_SAMPLE(g_sfxVoiceLetterE);
    FREE_SAMPLE(g_sfxVoiceLetterX);
    FREE_SAMPLE(g_sfxVoiceLetterT);
    FREE_SAMPLE(g_sfxVoiceLetterR);
    FREE_SAMPLE(g_sfxVoiceLetterA);

    FREE_SAMPLE(g_sampleSecret);
    FREE_SAMPLE(g_sfxGameOver);
    FREE_SAMPLE(g_sfxTimes2);
    FREE_SAMPLE(g_sfxTimes5);
    FREE_SAMPLE(g_sfxPerfect);

    FREE_SAMPLE(g_sfxGoodbye);
    FREE_SAMPLE(g_sfxFreeze);
    FREE_SAMPLE(g_sfxGemDrop);
    FREE_SAMPLE(g_sfxAutofire);
    FREE_SAMPLE(g_sfxDrunk);

    FREE_SAMPLE(g_sfxMirror);
    FREE_SAMPLE(g_sfxShop1);
    FREE_SAMPLE(g_sfxShop2);
    FREE_SAMPLE(g_sfxShop3);
    FREE_SAMPLE(g_sfxShop4);

    FREE_SAMPLE(g_sfxShop5);
    FREE_SAMPLE(g_sfxVoiceOne);
    FREE_SAMPLE(g_sfxVoiceTwo);
    FREE_SAMPLE(g_sfxVoiceThree);
    FREE_SAMPLE(g_sfxVoiceFour);

    FREE_SAMPLE(g_sfxVoiceFive);
    FREE_SAMPLE(g_sfxVoiceSix);
    FREE_SAMPLE(g_sfxVoiceSeven);
    FREE_SAMPLE(g_sfxVoiceEight);
    FREE_SAMPLE(g_sfxVoiceNine);

    FREE_SAMPLE(g_sfxVoiceTen);
    FREE_SAMPLE(g_sfxRankKnight);
    FREE_SAMPLE(g_sfxRankLord);
    FREE_SAMPLE(g_sfxRankOverlord);
    FREE_SAMPLE(g_sfxRankGrandmaster);

    FREE_SAMPLE(g_sfxRankChampion);
    FREE_SAMPLE(g_sfxRankGod);
    FREE_SAMPLE(g_sfxStar);
    FREE_SAMPLE(g_sfxStars);
    FREE_SAMPLE(g_sfxWarblade);

    FREE_SAMPLE(g_sfxRankBronze);
    FREE_SAMPLE(g_sfxRankSilver);
    FREE_SAMPLE(g_sfxRankGold);
    FREE_SAMPLE(g_sfxOhNo);
    FREE_SAMPLE(g_sfxBomb);

    FREE_SAMPLE(g_sfxRankMarker);
    FREE_SAMPLE(g_sfxGotcha);
    FREE_SAMPLE(g_sfxSpeed);
    FREE_SAMPLE(g_sfxPlanetPluto);
    FREE_SAMPLE(g_sfxPlanetNeptune);

    FREE_SAMPLE(g_sfxPlanetUranus);
    FREE_SAMPLE(g_sfxPlanetSaturn);
    FREE_SAMPLE(g_sfxPlanetJupiter);
    FREE_SAMPLE(g_sfxPlanetMars);
    FREE_SAMPLE(g_sfxPlanetTellus);

    FREE_SAMPLE(g_sfxPlanetVenus);
    FREE_SAMPLE(g_sfxPlanetMercury);
    FREE_SAMPLE(g_sfxPlanetSol);
    FREE_SAMPLE(g_sfxUltimateRank);
    FREE_SAMPLE(g_sfxRank);

    FREE_SAMPLE(g_sfxAvailable);
    FREE_SAMPLE(g_sfxPlanet);
    FREE_SAMPLE(g_sfxNew);
    FREE_SAMPLE(g_sfxYouAreThe);
}

#undef FREE_SAMPLE

// Loads a voice/announcer sample by base name, trying .wav/.mp3/.ogg. `mode` == 1 looks
// only in the voice pack folder (VoicePath); `mode` == 0 also falls back to the regular
// samples folder (SamplePath) if the voice pack doesn't have it. Logs and returns 0 on
// failure.
// Tries loading `name`.wav, then .mp3, then .ogg via `pathFn` (VoicePath or SamplePath),
// stopping at the first that loads.
#define TRY_LOAD_VOICE_CASCADE(pathFn)                          \
    sprintf(buf, "%s.wav", name);                                \
    sample = SampleLoad(pathFn(buf), 8, flags);                  \
    if (sample == 0) {                                           \
        sprintf(buf, "%s.mp3", name);                            \
        sample = SampleLoad(pathFn(buf), 8, flags);               \
        if (sample == 0) {                                       \
            sprintf(buf, "%s.ogg", name);                        \
            sample = SampleLoad(pathFn(buf), 8, flags);           \
        }                                                          \
    }

AudioHandle LoadVoiceSample(const char *name, int mode)
{
    char buf[512];
    AudioHandle sample;
    int flags;

    flags = 0;                                    // SampleLoad flags (a busy sample steals its quietest voice)
    if (mode == 1) {
        TRY_LOAD_VOICE_CASCADE(VoicePath);
    }

    if (mode == 0) {
        TRY_LOAD_VOICE_CASCADE(VoicePath);

        // Not found under the voice path; fall back to the plain sample path.
        if (sample == 0) {
            TRY_LOAD_VOICE_CASCADE(SamplePath);
        }
    }
    if (sample == 0) {
        LogPrint("Voice sample: '");
        LogPrint(name);
        LogPrint("' : NOT FOUND!\r\n");
    }
    return sample;
}

#undef TRY_LOAD_VOICE_CASCADE

// Frees, then reloads, every voice/announcer sample (rank names, pickup callouts,
// numbers, etc.) via LoadVoiceSample(). "welcome" and "goodbye" fall back to a regular
// LoadSample() a couple of lines further down if not found here (and again at the end
// of InitSound() as a final safety net).
void LoadVoices()
{
    FreeSamples();

    g_sfxBonus = LoadVoiceSample("bonus", 1);
    g_sfxDoubleShot = LoadVoiceSample("doubleshot", 1);
    g_sfxGetReady = LoadVoiceSample("getready", 1);
    g_sfxGetReady2 = LoadVoiceSample("getready2", 1);
    g_sfxGetReady3 = LoadVoiceSample("getready3", 1);
    g_sfxHurryUp1 = LoadVoiceSample("hurryup1", 1);

    g_sfxHurryUp2 = LoadVoiceSample("hurryup2", 1);
    g_sfxSingleShotVoice = LoadVoiceSample("singleshot", 1);
    g_sfxSucker = LoadVoiceSample("sucker", 1);
    g_sfxSucker2 = LoadVoiceSample("sucker2", 1);
    g_sfxSucker3 = LoadVoiceSample("sucker3", 1);
    g_sfxSuperTripleShot = LoadVoiceSample("supertripleshot", 1);

    g_sfxTripleShot = LoadVoiceSample("tripleshot", 1);
    g_sfxQuadShot = LoadVoiceSample("quadshot", 1);
    g_sfxRankEnsign = LoadVoiceSample("ensign", 1);
    g_sfxRankLieutenant = LoadVoiceSample("lieutenant", 1);
    g_sfxRankCommander = LoadVoiceSample("commander", 1);
    g_sfxRankCaptain = LoadVoiceSample("captain", 1);

    g_sfxRankAdmiral = LoadVoiceSample("admiral", 1);
    g_sfxAlright = LoadVoiceSample("alright", 1);
    g_sfxCongratulations = LoadVoiceSample("congratulations", 1);
    g_sfxArmour = LoadVoiceSample("armour", 1);
    g_sfxBonusUnused = LoadVoiceSample("bonus", 1);
    g_sfxPlayer1 = LoadVoiceSample("player1", 1);

    g_sfxPlayer2 = LoadVoiceSample("player2", 1);
    g_sfxPlayer = LoadVoiceSample("player", 1);
    g_sfxPlayers = LoadVoiceSample("players", 1);
    g_sfxWarning = LoadVoiceSample("warning", 1);
    g_sfxWarpMalfunction = LoadVoiceSample("warpmalfunction", 1);
    g_sfxExtraBullet = LoadVoiceSample("extrabullet", 1);

    g_sfxExtraLife = LoadVoiceSample("extralife", 1);
    g_sfxExtraSpeed = LoadVoiceSample("extraspeed", 1);
    g_sfxExtraTime = LoadVoiceSample("extratime", 1);
    g_sfxMemoryStation = LoadVoiceSample("memorystation", 1);
    g_sfxMeteorStorm = LoadVoiceSample("meteorstorm", 1);
    g_sfxOops = LoadVoiceSample("oops", 1);

    g_sfxScoop = LoadVoiceSample("scoop", 1);
    g_sfxShield = LoadVoiceSample("shield", 1);
    g_sfxMoney = LoadVoiceSample("money", 1);
    g_sfxWelcome = LoadVoiceSample("welcome", 1);
    if (g_sfxWelcome == 0) {
        g_sfxWelcome = LoadSample("welcome", 1);

    }
    g_sfxVoiceLetterE = LoadVoiceSample("e", 1);
    g_sfxVoiceLetterX = LoadVoiceSample("x", 1);
    g_sfxVoiceLetterT = LoadVoiceSample("t", 1);
    g_sfxVoiceLetterR = LoadVoiceSample("r", 1);
    g_sfxVoiceLetterA = LoadVoiceSample("a", 1);

    g_sampleSecret = LoadVoiceSample("secretfound", 1);
    g_sfxGameOver = LoadVoiceSample("gameover", 1);
    g_sfxTimes2 = LoadVoiceSample("times2", 1);
    g_sfxTimes5 = LoadVoiceSample("times5", 1);
    g_sfxPerfect = LoadVoiceSample("perfect", 1);
    g_sfxGoodbye = LoadVoiceSample("goodbye", 1);

    if (g_sfxGoodbye == 0) {
        g_sfxGoodbye = LoadSample("goodbye", 1);
    }
    g_sfxFreeze = LoadVoiceSample("freeze", 1);
    g_sfxGemDrop = LoadVoiceSample("gemdrop", 1);
    g_sfxAutofire = LoadVoiceSample("autofire", 1);

    g_sfxDrunk = LoadVoiceSample("drunk", 1);
    g_sfxMirror = LoadVoiceSample("mirror", 1);
    g_sfxShop1 = LoadVoiceSample("shop1", 1);
    g_sfxShop2 = LoadVoiceSample("shop2", 1);
    g_sfxShop3 = LoadVoiceSample("shop3", 1);
    g_sfxShop4 = LoadVoiceSample("shop4", 1);

    g_sfxShop5 = LoadVoiceSample("shop5", 1);
    g_sfxVoiceOne = LoadVoiceSample("one", 1);
    g_sfxVoiceTwo = LoadVoiceSample("two", 1);
    g_sfxVoiceThree = LoadVoiceSample("three", 1);
    g_sfxVoiceFour = LoadVoiceSample("four", 1);
    g_sfxVoiceFive = LoadVoiceSample("five", 1);

    g_sfxVoiceSix = LoadVoiceSample("six", 1);
    g_sfxVoiceSeven = LoadVoiceSample("seven", 1);
    g_sfxVoiceEight = LoadVoiceSample("eight", 1);
    g_sfxVoiceNine = LoadVoiceSample("nine", 1);
    g_sfxVoiceTen = LoadVoiceSample("ten", 1);
    g_sfxRankKnight = LoadVoiceSample("knight", 1);

    g_sfxRankLord = LoadVoiceSample("lord", 1);
    g_sfxRankOverlord = LoadVoiceSample("overlord", 1);
    g_sfxRankGrandmaster = LoadVoiceSample("grandmaster", 1);
    g_sfxRankChampion = LoadVoiceSample("champion", 1);
    g_sfxRankGod = LoadVoiceSample("god", 1);
    g_sfxStar = LoadVoiceSample("star", 1);

    g_sfxStars = LoadVoiceSample("stars", 1);
    g_sfxWarblade = LoadVoiceSample("warblade", 1);
    g_sfxRankBronze = LoadVoiceSample("Bronze", 1);
    g_sfxRankSilver = LoadVoiceSample("Silver", 1);
    g_sfxRankGold = LoadVoiceSample("gold", 1);
    g_sfxOhNo = LoadVoiceSample("ohno", 1);

    g_sfxBomb = LoadVoiceSample("bomb", 1);
    g_sfxRankMarker = LoadVoiceSample("rankmarker", 1);
    g_sfxGotcha = LoadVoiceSample("gotcha", 1);
    g_sfxSpeed = LoadVoiceSample("speed", 1);
    g_sfxPlanetPluto = LoadVoiceSample("pluto", 1);
    g_sfxPlanetNeptune = LoadVoiceSample("neptune", 1);

    g_sfxPlanetUranus = LoadVoiceSample("uranus", 1);
    g_sfxPlanetSaturn = LoadVoiceSample("saturn", 1);
    g_sfxPlanetJupiter = LoadVoiceSample("jupiter", 1);
    g_sfxPlanetMars = LoadVoiceSample("mars", 1);
    g_sfxPlanetTellus = LoadVoiceSample("tellus", 1);
    g_sfxPlanetVenus = LoadVoiceSample("venus", 1);

    g_sfxPlanetMercury = LoadVoiceSample("mercury", 1);
    g_sfxPlanetSol = LoadVoiceSample("sol", 1);
    g_sfxUltimateRank = LoadVoiceSample("ultimaterank", 1);
    g_sfxRank = LoadVoiceSample("rank", 1);
    g_sfxAvailable = LoadVoiceSample("available", 1);
    g_sfxPlanet = LoadVoiceSample("planet", 1);

    g_sfxNew = LoadVoiceSample("new", 1);
    g_sfxYouAreThe = LoadVoiceSample("youarethe", 1);
}

// Starts up the audio system and loads every sound effect and voice sample used by the
// game.
void InitSound()
{
    SoundShutdown();
    if (AudioInit())
        g_soundEnabled = 1;
    else
        LogPrint("SOUNDSYSTEM : Error initializing soundcard !!\r\n");
    if (g_soundEnabled != 0) {
        g_sfxAlienShoot1 = LoadSample("alienshoot1", 15);
        g_sfxAlienShoot2 = LoadSample("alienshoot2", 15);
        g_sfxAlienShoot3 = LoadSample("alienshoot3", 15);
        g_sfxAlienShoot4 = LoadSample("alienshoot4", 15);
        g_sndAlienShoot5 = LoadSample("alienshoot5", 15);
        g_sfxAlienShoot6 = LoadSample("alienshoot6", 15);

        g_sfxAlienShoot7 = LoadSample("alienshoot7", 15);
        g_sfxAlienShoot8 = LoadSample("alienshoot8", 15);
        g_sfxAlienShoot9 = LoadSample("alienshoot9", 15);
        g_sndAlienShoot10 = LoadSample("alienshoot10", 15);
        g_sfxAlienShoot11 = LoadSample("alienshoot11", 15);
        g_sndAlienShoot12 = LoadSample("alienshoot12", 15);

        g_sfxAlienShoot12Alt = LoadSample("alienshoot12_2", 15);
        g_sfxAlienShoot13 = LoadSample("alienshoot13", 15);
        g_sfxAlienShoot14 = LoadSample("alienshoot14", 15);
        g_sfxAlienShoot15 = LoadSample("alienshoot15", 15);
        g_sfxAlienShoot16 = LoadSample("alienshoot16", 15);
        g_sfxAlienShoot17 = LoadSample("alienshoot17", 15);

        g_sfxAlienShoot18 = LoadSample("alienshoot18", 15);
        g_sfxTast = LoadSample("tast", 6);
        g_sfxBell1 = LoadSample("bell1", 6);
        g_sfxBell2 = LoadSample("bell2", 6);
        g_sfxBell3 = LoadSample("bell3", 20);
        g_sfxBigFire = LoadSample("bigfire", 5);

        g_sfxBigSmall = LoadSample("bigsmall", 6);
        g_sfxHit1 = LoadSample("hit1", 5);
        g_sfxHit2 = LoadSample("hit2", 5);
        g_sfxHit3 = LoadSample("hit3", 15);
        g_sfxHit4 = LoadSample("hit4", 15);
        g_sfxClickGeneric = LoadSample("click", 3);

        g_sfxOver = LoadSample("over", 15);
        g_sfxFoundIt = LoadSample("foundit", 4);
        g_sfxBing = LoadSample("bing", 6);
        g_sfxBirth = LoadSample("birth", 4);
        g_sfxChaching = LoadSample("chaching", 6);
        g_sfxCash = LoadSample("cash", 6);

        g_sfxExplo1 = LoadSample("explo1", 7);
        g_sfxExplo2 = LoadSample("explo2", 7);
        g_sfxExplo3 = LoadSample("explo3", 7);
        g_sfxExplo4 = LoadSample("explo4", 7);
        g_sfxExplo5 = LoadSample("explo5", 7);
        g_sfxGuit = LoadSample("guit", 3);

        g_sfxFanfare = LoadSample("fanfare", 2);
        g_sfxFanfare1 = LoadSample("fanfare1", 2);
        g_sfxLaser1 = LoadSample("laser1", 15);
        g_sfxLaser2 = LoadSample("laser2", 15);
        g_sfxOrkHit = LoadSample("orkhit", 4);
        g_sfxShot1 = LoadSample("shot1", 5);

        g_sfxShot2 = LoadSample("shot2", 5);
        g_sfxWaom = LoadSample("waom", 3);
        g_sfxWooing = LoadSample("wooing", 3);
        g_sfxWaauw = LoadSample("waauw", 3);
        g_sfxZuzk = LoadSample("zuzk", 3);
        g_sfxJangle = LoadSample("jangle", 3);

        g_sfxHarpGliss1 = LoadSample("harpgliss1", 2);
        g_sfxMachine = LoadSample("machine", 3);
        g_sfxRocket = LoadSample("rocket", 6);
        g_sfxMouww = LoadSample("mouww", 5);
        g_sfxPing = LoadSample("ping", 8);
        g_sfxWarp = LoadSample("warp", 5);

        g_sfxWarp2 = LoadSample("warp2", 5);
        g_sfxWarp3 = LoadSample("warp3", 5);
        g_sfxWheommm = LoadSample("wheommm", 5);
        g_sfxAlienAttack = LoadSample("alienattack", 5);
        g_sfxAlienAttack2 = LoadSample("alienattack2", 10);
        g_sfxAlienAttack3 = LoadSample("alienattack3", 5);

        g_sfxAlienAttack4 = LoadSample("alienattack4", 5);
        g_sfxAlienAttack5 = LoadSample("alienattack5", 5);
        g_sfxMeteorPass = LoadSample("meteorpass", 4);
        g_sfxDeath = LoadSample("death", 2);
        g_sfxCapture = LoadSample("capture", 2);
        g_sfxSlide = LoadSample("slide", 2);

        g_sfxKanganang = LoadSample("kanganang", 2);
        g_sfxJingles = LoadSample("jingles", 5);
        g_sfxHahaha = LoadSample("hahaha", 3);
        g_sfxScopeHum = LoadSampleLoop("scopehum", 1);
        g_sfxShieldHum = LoadSampleLoop("shieldhum", 1);
        g_sfxMotherLoop = LoadSampleLoop("mother", 3);

        g_sfxGuardLoop = LoadSampleLoop("guard", 1);
        g_sfxBossLoop = LoadSampleLoop("boss", 1);
        g_sfxShipHumLoop = LoadSampleLoop("mshiphum", 4);
        g_sfxMothership = LoadSample("mothership", 4);
        g_sfxPing2 = LoadSample("ping2", 5);
        g_sfxCoin = LoadSample("coin", 5);

        g_sfxCurrent = LoadSample("current", 2);
        g_sfxRollover = LoadSample("rollover", 2);
        g_sfxBuzzer = LoadSample("buzzer", 2);
        g_sfxBuzzer2 = LoadSample("buzzer2", 2);
        g_sfxMoneyBomb = LoadSample("moneybomb", 2);
        g_sfxGemBomb = LoadSample("gembomb", 2);

        g_sfxWindow = LoadSample("maximize", 2);
        g_sfxMinimize = LoadSample("minimize", 2);
        g_sfxButtonClick = LoadSample("buttonclick", 2);
        g_sfxZoom = LoadSample("zoom", 2);
        g_sfxMetal = LoadSample("metal", 2);
        g_sfxSword = LoadSample("sword", 2);

        g_sfxThump = LoadSample("thump", 2);
        g_sfxThumpBig = LoadSample("thumpbig", 2);
        g_sfxSwoosh = LoadSample("swoosh", 2);
        g_sfxComing = LoadSample("coming", 2);
        g_sfxWhip = LoadSample("whip", 2);
        g_sfxFalling = LoadSample("falling", 2);

        g_sfxSingleShot = LoadSample("singleshot", 5);

        LogPrint("All samples loaded is passed...\r\n");
        InitSampleTable();
        LoadVoices();

        if (g_sfxWelcome == 0) {
            g_sfxWelcome = LoadSample("welcome", 1);
        }
        if (g_sfxGoodbye == 0) {
            g_sfxGoodbye = LoadSample("goodbye", 1);
        }
    }
}

// Frees a sample handle if it is loaded, and zeroes it.
#define FREE_SAMPLE(h) if (h) { SampleFree(h); (h) = 0; }

// Stops playback, frees every sound-effect and voice sample, then shuts down audio and
// clears g_soundEnabled. Safe to call even if sound was never started.
void SoundShutdown()
{
    if (g_soundEnabled) {
        AudioStop();
        g_pend0 = 0;
        g_pend1 = 0;
        g_pend2 = 0;
        g_pend3 = 0;
        g_pend4 = 0;
        g_pend5 = 0;
        LogPrint("CLOSE MUSIC\r\n");
        LogPrint("FREE SAMPLES\r\n");

        FREE_SAMPLE(g_sfxAlienShoot1);
        FREE_SAMPLE(g_sfxAlienShoot2);
        FREE_SAMPLE(g_sfxAlienShoot3);
        FREE_SAMPLE(g_sfxAlienShoot4);
        FREE_SAMPLE(g_sndAlienShoot5);

        FREE_SAMPLE(g_sfxAlienShoot6);
        FREE_SAMPLE(g_sfxAlienShoot7);
        FREE_SAMPLE(g_sfxAlienShoot8);
        FREE_SAMPLE(g_sfxAlienShoot9);
        FREE_SAMPLE(g_sndAlienShoot10);

        FREE_SAMPLE(g_sfxAlienShoot11);
        FREE_SAMPLE(g_sndAlienShoot12);
        FREE_SAMPLE(g_sfxAlienShoot12Alt);
        FREE_SAMPLE(g_sfxAlienShoot13);
        FREE_SAMPLE(g_sfxAlienShoot14);

        FREE_SAMPLE(g_sfxAlienShoot15);
        FREE_SAMPLE(g_sfxAlienShoot16);
        FREE_SAMPLE(g_sfxAlienShoot17);
        FREE_SAMPLE(g_sfxAlienShoot18);
        FREE_SAMPLE(g_sfxTast);

        FREE_SAMPLE(g_sfxBell1);
        FREE_SAMPLE(g_sfxBell2);
        FREE_SAMPLE(g_sfxBell3);
        FREE_SAMPLE(g_sfxBigFire);
        FREE_SAMPLE(g_sfxBigSmall);

        FREE_SAMPLE(g_sfxHit1);
        FREE_SAMPLE(g_sfxHit2);
        FREE_SAMPLE(g_sfxHit3);
        FREE_SAMPLE(g_sfxHit4);
        FREE_SAMPLE(g_sfxClickGeneric);

        FREE_SAMPLE(g_sfxOver);
        FREE_SAMPLE(g_sfxFoundIt);
        FREE_SAMPLE(g_sfxBing);
        FREE_SAMPLE(g_sfxBirth);
        FREE_SAMPLE(g_sfxChaching);

        FREE_SAMPLE(g_sfxCash);
        FREE_SAMPLE(g_sfxExplo1);
        FREE_SAMPLE(g_sfxExplo2);
        FREE_SAMPLE(g_sfxExplo3);
        FREE_SAMPLE(g_sfxExplo4);

        FREE_SAMPLE(g_sfxExplo5);
        FREE_SAMPLE(g_sfxGuit);
        FREE_SAMPLE(g_sfxFanfare);
        FREE_SAMPLE(g_sfxFanfare1);
        FREE_SAMPLE(g_sfxLaser1);

        FREE_SAMPLE(g_sfxLaser2);
        FREE_SAMPLE(g_sfxOrkHit);
        FREE_SAMPLE(g_sfxShot1);
        FREE_SAMPLE(g_sfxShot2);
        FREE_SAMPLE(g_sfxWaom);

        FREE_SAMPLE(g_sfxWooing);
        FREE_SAMPLE(g_sfxWaauw);
        FREE_SAMPLE(g_sfxZuzk);
        FREE_SAMPLE(g_sfxJangle);
        FREE_SAMPLE(g_sfxHarpGliss1);

        FREE_SAMPLE(g_sfxMachine);
        FREE_SAMPLE(g_sfxRocket);
        FREE_SAMPLE(g_sfxMouww);
        FREE_SAMPLE(g_sfxPing);
        FREE_SAMPLE(g_sfxWarp);

        FREE_SAMPLE(g_sfxWarp2);
        FREE_SAMPLE(g_sfxWarp3);
        FREE_SAMPLE(g_sfxWheommm);
        FREE_SAMPLE(g_sfxScopeHum);
        FREE_SAMPLE(g_sfxShieldHum);

        FREE_SAMPLE(g_sfxMotherLoop);
        FREE_SAMPLE(g_sfxGuardLoop);
        FREE_SAMPLE(g_sfxBossLoop);
        FREE_SAMPLE(g_sfxShipHumLoop);
        FREE_SAMPLE(g_sfxAlienAttack);

        FREE_SAMPLE(g_sfxAlienAttack2);
        FREE_SAMPLE(g_sfxAlienAttack3);
        FREE_SAMPLE(g_sfxAlienAttack4);
        FREE_SAMPLE(g_sfxAlienAttack5);
        FREE_SAMPLE(g_sfxMeteorPass);

        FREE_SAMPLE(g_sfxDeath);
        FREE_SAMPLE(g_sfxCapture);
        FREE_SAMPLE(g_sfxSlide);
        FREE_SAMPLE(g_sfxKanganang);
        FREE_SAMPLE(g_sfxJingles);

        FREE_SAMPLE(g_sfxMothership);
        FREE_SAMPLE(g_sfxPing2);
        FREE_SAMPLE(g_sfxCoin);
        FREE_SAMPLE(g_sfxCurrent);
        FREE_SAMPLE(g_sfxBuzzer);

        FREE_SAMPLE(g_sfxBuzzer2);
        FREE_SAMPLE(g_sfxGemBomb);
        FREE_SAMPLE(g_sfxMoneyBomb);
        FREE_SAMPLE(g_sfxRollover);
        FREE_SAMPLE(g_sfxMinimize);

        FREE_SAMPLE(g_sfxWindow);
        FREE_SAMPLE(g_sfxButtonClick);
        FREE_SAMPLE(g_sfxZoom);
        FREE_SAMPLE(g_sfxMetal);
        FREE_SAMPLE(g_sfxSword);

        FREE_SAMPLE(g_sfxThump);
        FREE_SAMPLE(g_sfxThumpBig);
        FREE_SAMPLE(g_sfxSwoosh);
        FREE_SAMPLE(g_sfxComing);
        FREE_SAMPLE(g_sfxWhip);

        FREE_SAMPLE(g_sfxFalling);
        FREE_SAMPLE(g_sfxSingleShot);

        FreeSamples();
        LogPrint("SOUNDSYSTEM : Closing down...\r\n");
        AudioShutdown();
        g_soundEnabled = 0;
    }
}

#undef FREE_SAMPLE

// Plays the UI "minimize" sound effect.
void PlaySample()
{
    SoundPlay(g_sfxMinimize, -1, 200, 0.0f, 0xbf, g_sndFlags);
}

// Plays the generic menu click sound and resets the menu idle timeout (60s).
void PlayClick()
{
    g_menuIdleTimeout = g_time + 60000;
    SoundPlay(g_sfxTast, -1, SFX_VOL_FULL, 0.0f, 127, g_sndFlags);
}

// Plays up to 4 queued samples once, the frame after they were queued (g_fadeOnce is a
// one-shot gate that resets itself after firing).
void FadeLoopSamples()
{
    if (!g_fadeOnce--) {
        g_fadeOnce = 1;
        if (g_fadeQueueSample1) {
            SoundPlay(g_fadeQueueSample1, g_fadeFreq1, (int)g_fadeVolume0,
                      g_panTable[(int)g_fadePan0], 127, g_sndFlags);
            g_fadeQueueSample1 = 0;
        }

        if (g_fadeQueueSample2) {
            SoundPlay(g_fadeQueueSample2, g_fadeFreq2, (int)g_fadeVolume1,
                      g_panTable[(int)g_fadePan2], 127, g_sndFlags);
            g_fadeQueueSample2 = 0;
        }

        if (g_fadeQueueSample3) {
            SoundPlay(g_fadeQueueSample3, g_sfxFreq, (int)g_sfxVolume,
                      g_panTable[(int)g_sfxPanIdx], 127, g_sndFlags);
            g_fadeQueueSample3 = 0;
        }
        if (g_fadeQueueSample4) {
            SoundPlay(g_fadeQueueSample4, g_fadeFreq4, (int)g_fadeVolume3,
                      g_panTable[(int)g_msgPanX], 127, g_sndFlags);
            g_fadeQueueSample4 = 0;
        }
    }
}
