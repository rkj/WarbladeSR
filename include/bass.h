#pragma once
// BASS audio library (bass.dll, un4seen): the functions the game calls, with the types
// of BASS 2.x's bass.h spelled on our own typedefs (BASS_DWORD is in types.h).  The exe
// imports them from bass.dll through `jmp [IAT]` stubs.
#include "types.h"

typedef int BASS_BOOL;                     // bass.h: BOOL
typedef unsigned __int64 BASS_QWORD;       // bass.h: QWORD

// The constants the game passes, with bass.h's names and values.
#define BASS_CONFIG_BUFFER          0
#define BASS_CONFIG_UPDATEPERIOD    1
#define BASS_CONFIG_GVOL_SAMPLE     4
#define BASS_CONFIG_GVOL_STREAM     5
#define BASS_CONFIG_UPDATETHREADS   24

#define BASS_SAMPLE_LOOP            4
#define BASS_SAMPLE_SOFTWARE        0x10
#define BASS_SAMPLE_OVER_VOL        0x10000
#define BASS_MUSIC_LOOP             BASS_SAMPLE_LOOP
#define BASS_MUSIC_PRESCAN          0x20000
#define BASS_STREAM_PRESCAN         0x20000

#define BASS_ATTRIB_FREQ            1
#define BASS_ATTRIB_VOL             2
#define BASS_ATTRIB_PAN             3

#define BASS_POS_BYTE               0

extern "C" {
BASS_BOOL __stdcall BASS_SetConfig(BASS_DWORD option, BASS_DWORD value);
BASS_DWORD __stdcall BASS_GetVersion();
int __stdcall BASS_ErrorGetCode();
BASS_BOOL __stdcall BASS_Init(int device, BASS_DWORD freq, BASS_DWORD flags, void *win, const void *dsguid);
BASS_BOOL __stdcall BASS_Free();
BASS_BOOL __stdcall BASS_Update(BASS_DWORD length);
BASS_BOOL __stdcall BASS_Start();
BASS_BOOL __stdcall BASS_Stop();
BASS_BOOL __stdcall BASS_Pause();

BASS_DWORD __stdcall BASS_MusicLoad(BASS_BOOL mem, const void *file, BASS_QWORD offset, BASS_DWORD length, BASS_DWORD flags, BASS_DWORD freq);
BASS_BOOL __stdcall BASS_MusicFree(BASS_DWORD handle);

BASS_DWORD __stdcall BASS_SampleLoad(BASS_BOOL mem, const void *file, BASS_QWORD offset, BASS_DWORD length, BASS_DWORD max, BASS_DWORD flags);
BASS_BOOL __stdcall BASS_SampleFree(BASS_DWORD handle);
BASS_DWORD __stdcall BASS_SampleGetChannel(BASS_DWORD handle, BASS_BOOL onlynew);

BASS_DWORD __stdcall BASS_StreamCreateFile(BASS_BOOL mem, const void *file, BASS_QWORD offset, BASS_QWORD length, BASS_DWORD flags);
BASS_BOOL __stdcall BASS_StreamFree(BASS_DWORD handle);

BASS_BOOL __stdcall BASS_ChannelPlay(BASS_DWORD handle, BASS_BOOL restart);
BASS_BOOL __stdcall BASS_ChannelStop(BASS_DWORD handle);
BASS_BOOL __stdcall BASS_ChannelSetAttribute(BASS_DWORD handle, BASS_DWORD attrib, float value);
BASS_BOOL __stdcall BASS_ChannelSlideAttribute(BASS_DWORD handle, BASS_DWORD attrib, float value, BASS_DWORD time);
BASS_QWORD __stdcall BASS_ChannelGetLength(BASS_DWORD handle, BASS_DWORD mode);
BASS_QWORD __stdcall BASS_ChannelGetPosition(BASS_DWORD handle, BASS_DWORD mode);
BASS_BOOL __stdcall BASS_ChannelSetPosition(BASS_DWORD handle, BASS_QWORD pos, BASS_DWORD mode);
double __stdcall BASS_ChannelBytes2Seconds(BASS_DWORD handle, BASS_QWORD pos);
}

// HIWORD(BASS_GetVersion()) of BASS 2.4, the version the game requires.
#define BASS_VERSION_24 0x204
