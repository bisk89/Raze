/*
 Copyright (C) 2009 Jonathon Fowler <jf@jonof.id.au>
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 
 See the GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 */

/**
 * Abstraction layer for hiding the various supported sound devices
 * behind a common and opaque interface called on by MultiVoc.
 */

#include "drivers.h"

#include "driver_adlib.h"

#ifdef RENDERTYPESDL
# include "driver_sdl.h"
#endif

#ifdef _WIN32
# include "driver_directsound.h"
# include "driver_winmm.h"
#endif

int ASS_PCMSoundDriver  = ASS_AutoDetect;
int ASS_MIDISoundDriver = ASS_AutoDetect;
int ASS_EMIDICard = -1;

#define UNSUPPORTED_PCM          nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
#define UNSUPPORTED_MIDI         nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
#define UNSUPPORTED_COMPLETELY   nullptr, nullptr, UNSUPPORTED_PCM, UNSUPPORTED_MIDI

static struct {
    const char * DriverName;
    
    int  (*GetError)(void);
    const char *(*ErrorString)(int);

    int  (*PCM_Init)(int *, int *, void *);
    void (*PCM_Shutdown)(void);
    int  (*PCM_BeginPlayback)(char *, int, int, void (*)(void));
    void (*PCM_StopPlayback)(void);
    void (*PCM_Lock)(void);
    void (*PCM_Unlock)(void);

    int  (*MIDI_Init)(midifuncs *);
    void (*MIDI_Shutdown)(void);
    int  (*MIDI_StartPlayback)(void (*service)(void));
    void (*MIDI_HaltPlayback)(void);
    void (*MIDI_SetTempo)(int tempo, int division);
    void (*MIDI_Lock)(void);
    void (*MIDI_Unlock)(void);
} SoundDrivers[ASS_NumSoundCards] = {
    
  
    // Simple DirectMedia Layer
    {
        "SDL",
    #ifdef RENDERTYPESDL
        SDLDrv_GetError,
        SDLDrv_ErrorString,
        SDLDrv_PCM_Init,
        SDLDrv_PCM_Shutdown,
        SDLDrv_PCM_BeginPlayback,
        SDLDrv_PCM_StopPlayback,
        SDLDrv_PCM_Lock,
        SDLDrv_PCM_Unlock,
        UNSUPPORTED_MIDI,
    #else
        UNSUPPORTED_COMPLETELY
    #endif
    },

    // Windows DirectSound
    {
        "DirectSound",
    #ifdef RENDERTYPEWIN
        DirectSoundDrv_GetError,
        DirectSoundDrv_ErrorString,
        DirectSoundDrv_PCM_Init,
        DirectSoundDrv_PCM_Shutdown,
        DirectSoundDrv_PCM_BeginPlayback,
        DirectSoundDrv_PCM_StopPlayback,
        DirectSoundDrv_PCM_Lock,
        DirectSoundDrv_PCM_Unlock,
        UNSUPPORTED_MIDI,
    #else
        UNSUPPORTED_COMPLETELY
    #endif
    },

    // OPL3 emulation
    {
        "Nuked OPL3 AdLib emulator",
        AdLibDrv_GetError,
        AdLibDrv_ErrorString,

        UNSUPPORTED_PCM,

        AdLibDrv_MIDI_Init,
        AdLibDrv_MIDI_Shutdown,
        AdLibDrv_MIDI_StartPlayback,
        AdLibDrv_MIDI_HaltPlayback,
        AdLibDrv_MIDI_SetTempo,
        nullptr,
        nullptr,
    },

    // Windows MultiMedia system
    {
        "WinMM",
    #ifdef _WIN32
        WinMMDrv_GetError,
        WinMMDrv_ErrorString,

        UNSUPPORTED_PCM,

        WinMMDrv_MIDI_Init,
        WinMMDrv_MIDI_Shutdown,
        WinMMDrv_MIDI_StartPlayback,
        WinMMDrv_MIDI_HaltPlayback,
        WinMMDrv_MIDI_SetTempo,
        WinMMDrv_MIDI_Lock,
        WinMMDrv_MIDI_Unlock,
    #else
        UNSUPPORTED_COMPLETELY
    #endif
    },
};


int SoundDriver_IsPCMSupported(int driver)  { return (SoundDrivers[driver].PCM_Init != 0); }
int SoundDriver_IsMIDISupported(int driver) { return (SoundDrivers[driver].MIDI_Init != 0); }
const char *SoundDriver_GetName(int driver) { return  SoundDrivers[driver].DriverName; }

int SoundDriver_PCM_GetError(void)
{
    if (!SoundDriver_IsPCMSupported(ASS_PCMSoundDriver))
        return -1;

    return SoundDrivers[ASS_PCMSoundDriver].GetError();
}

const char * SoundDriver_PCM_ErrorString( int ErrorNumber )
{
    if (ASS_PCMSoundDriver < 0 || ASS_PCMSoundDriver >= ASS_NumSoundCards)
        return "No sound driver selected.";

    if (!SoundDriver_IsPCMSupported(ASS_PCMSoundDriver))
        return "Unsupported sound driver selected.";

    return SoundDrivers[ASS_PCMSoundDriver].ErrorString(ErrorNumber);
}

int SoundDriver_MIDI_GetError(void)
{
    if (!SoundDriver_IsMIDISupported(ASS_MIDISoundDriver))
        return -1;

    return SoundDrivers[ASS_MIDISoundDriver].GetError();
}

const char * SoundDriver_MIDI_ErrorString( int ErrorNumber )
{
    if (ASS_MIDISoundDriver < 0 || ASS_MIDISoundDriver >= ASS_NumSoundCards)
        return "No sound driver selected.";

    if (!SoundDriver_IsMIDISupported(ASS_MIDISoundDriver))
        return "Unsupported sound driver selected.";

    return SoundDrivers[ASS_MIDISoundDriver].ErrorString(ErrorNumber);
}

int SoundDriver_PCM_Init(int *mixrate, int *numchannels, void *initdata)
{
    return SoundDrivers[ASS_PCMSoundDriver].PCM_Init(mixrate, numchannels, initdata);
}

int SoundDriver_PCM_BeginPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void))
{
    return SoundDrivers[ASS_PCMSoundDriver].PCM_BeginPlayback(BufferStart, BufferSize, NumDivisions, CallBackFunc);
}

void SoundDriver_PCM_Shutdown(void)                        { SoundDrivers[ASS_PCMSoundDriver].PCM_Shutdown(); }
void SoundDriver_PCM_StopPlayback(void)                    { SoundDrivers[ASS_PCMSoundDriver].PCM_StopPlayback(); }
void SoundDriver_PCM_Lock(void)                            { SoundDrivers[ASS_PCMSoundDriver].PCM_Lock(); }
void SoundDriver_PCM_Unlock(void)                          { SoundDrivers[ASS_PCMSoundDriver].PCM_Unlock(); }
int  SoundDriver_MIDI_Init(midifuncs *funcs)               { return SoundDrivers[ASS_MIDISoundDriver].MIDI_Init(funcs); }
int  SoundDriver_MIDI_StartPlayback(void (*service)(void)) { return SoundDrivers[ASS_MIDISoundDriver].MIDI_StartPlayback(service); }
void SoundDriver_MIDI_Shutdown(void)                       { SoundDrivers[ASS_MIDISoundDriver].MIDI_Shutdown(); }
void SoundDriver_MIDI_HaltPlayback(void)                   { SoundDrivers[ASS_MIDISoundDriver].MIDI_HaltPlayback(); }
void SoundDriver_MIDI_SetTempo(int tempo, int division)    { SoundDrivers[ASS_MIDISoundDriver].MIDI_SetTempo(tempo, division); }
void SoundDriver_MIDI_Lock(void)                           { if (SoundDrivers[ASS_MIDISoundDriver].MIDI_Lock) SoundDrivers[ASS_MIDISoundDriver].MIDI_Lock(); }
void SoundDriver_MIDI_Unlock(void)                         { if (SoundDrivers[ASS_MIDISoundDriver].MIDI_Unlock) SoundDrivers[ASS_MIDISoundDriver].MIDI_Unlock(); }

// vim:ts=4:sw=4:expandtab:
