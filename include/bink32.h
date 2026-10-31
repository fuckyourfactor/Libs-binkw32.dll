/* SPDX-License-Identifier: MIT
   Independently written compatibility declarations for the 32-bit Bink API.
   ABI structure layouts beyond the documented public prefix remain unverified. */
#ifndef BINK_OPEN_32_H
#define BINK_OPEN_32_H
#include <stdint.h>
#if defined(_WIN32) || defined(_WIN64)
# define BK_CALL __stdcall
# define BK_API
#else
# define BK_CALL
# define BK_API
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef uint32_t U32;
typedef int32_t S32;
typedef struct BINKRECT { S32 Left,Top,Width,Height; } BINKRECT;
typedef struct BINKPLANE { S32 Allocate; void *Buffer; U32 BufferPitch; } BINKPLANE;
typedef struct BINKFRAMEPLANESET { BINKPLANE YPlane, cRPlane, cBPlane, APlane; } BINKFRAMEPLANESET;
typedef struct BINKFRAMEBUFFERS {
 S32 TotalFrames; U32 YABufferWidth,YABufferHeight,cRcBBufferWidth,cRcBBufferHeight,FrameNum;
 BINKFRAMEPLANESET Frames[2];
} BINKFRAMEBUFFERS;
typedef struct BINK {
 U32 Width,Height,Frames,FrameNum,LastFrameNum,FrameRate,FrameRateDiv,ReadError,OpenFlags;
 U32 BinkType,Size,FrameSize,SndSize;
 /* Bink 1 public ABI: this U32 is BETWEEN SndSize and FrameRects.
  * Missing it shifts every dirty rectangle and makes the host see a
  * zero-height first rectangle (invisible video despite decoded audio). */
 U32 FrameChangePercent;
 BINKRECT FrameRects[8];
 S32 NumRects;
 BINKFRAMEBUFFERS *FrameBuffers;
 void *MaskPlane;
 U32 MaskPitch,MaskLength;
 void *AsyncMaskPlane,*InUseMaskPlane,*LastMaskPlane;
 U32 LargestFrameSize,InternalFrames;
 S32 NumTracks;
 U32 Highest1SecRate,Highest1SecFrame;
 S32 Paused;
 U32 BackgroundThread;
 /* Not part of the claimed verified ABI. Private storage begins in an enclosing object. */
} BINK;
typedef BINK *HBINK;
typedef struct BINKTRACK {
 U32 Frequency,Bits,Channels,MaxSize; HBINK bink; uintptr_t sndcomp; S32 trackindex;
} BINKTRACK;
typedef BINKTRACK *HBINKTRACK;
typedef struct BINKSUMMARY {
 U32 Width,Height,TotalTime,FileFrameRate,FileFrameRateDiv,FrameRate,FrameRateDiv;
 U32 TotalOpenTime,TotalFrames,TotalPlayedFrames,SkippedFrames,SkippedBlits,SoundSkips;
 U32 TotalBlitTime,TotalReadTime,TotalVideoDecompTime,TotalAudioDecompTime,TotalIdleReadTime;
 U32 TotalBackReadTime,TotalReadSpeed,SlowestFrameTime,Slowest2FrameTime,SlowestFrameNum;
 U32 Slowest2FrameNum,AverageDataRate,AverageFrameSize,HighestMemAmount,TotalIOMemory;
 U32 HighestIOUsed,Highest1SecRate,Highest1SecFrame;
} BINKSUMMARY;
typedef struct BINKREALTIME {
 U32 FrameNum,FrameRate,FrameRateDiv,Frames,FramesTime,FramesVideoDecompTime;
 U32 FramesAudioDecompTime,FramesReadTime,FramesIdleReadTime,FramesThreadReadTime;
 U32 FramesBlitTime,ReadBufferSize,ReadBufferUsed,FramesDataRate;
} BINKREALTIME;
/* The pixel buffer public prefix. Do not assume its private tail matches the SDK. */
typedef struct BINKBUFFER {
 U32 Width,Height,WindowWidth,WindowHeight,SurfaceType;
 void *Buffer; S32 BufferPitch,ClientOffsetX,ClientOffsetY;
 U32 ScreenWidth,ScreenHeight,ScreenDepth,ExtraWindowWidth,ExtraWindowHeight;
 U32 ScaleFlags,StretchWidth,StretchHeight;
 S32 surface; void *ddsurface,*ddclipper;
 S32 destx,desty,wndx,wndy; void *wnd;
 S32 minimized,ddoverlay,ddoffscreen,lastovershow,issoftcur;
 U32 cursorcount; void *buffertop; U32 type; S32 noclipping,loadeddd,loadedwin;
 void *dibh,*dibbuffer; S32 dibpitch; void *dibinfo,*dibdc,*diboldbitmap;
} BINKBUFFER;
typedef BINKBUFFER *HBINKBUFFER;
typedef void *(BK_CALL *BINKMEMALLOC)(U32);
typedef void (BK_CALL *BINKMEMFREE)(void *);
/* Public Bink sound driver callbacks reconstructed from published API facts.
 * The parts of BINKSND beyond these public fields are not ABI-verified against
 * the supplied 2005 binary. All driver callbacks are stdcall on 32-bit Win32. */
typedef struct BINKSND BINKSND;
typedef S32 (BK_CALL *BINKSNDREADY)(BINKSND*);
typedef S32 (BK_CALL *BINKSNDLOCK)(BINKSND*,uint8_t**,U32*);
typedef S32 (BK_CALL *BINKSNDUNLOCK)(BINKSND*,U32);
typedef void (BK_CALL *BINKSNDVOLUME)(BINKSND*,S32);
typedef void (BK_CALL *BINKSNDPAN)(BINKSND*,S32);
typedef S32 (BK_CALL *BINKSNDPAUSE)(BINKSND*,S32);
typedef S32 (BK_CALL *BINKSNDONOFF)(BINKSND*,S32);
typedef void (BK_CALL *BINKSNDCLOSE)(BINKSND*);
typedef void (BK_CALL *BINKSNDMIXBINS)(BINKSND*,U32*,U32);
typedef void (BK_CALL *BINKSNDMIXBINVOLS)(BINKSND*,U32*,S32*,U32);
struct BINKSND {
 BINKSNDREADY Ready; BINKSNDLOCK Lock; BINKSNDUNLOCK Unlock;
 BINKSNDVOLUME Volume; BINKSNDPAN Pan; BINKSNDPAUSE Pause;
 BINKSNDONOFF SetOnOff; BINKSNDCLOSE Close;
 BINKSNDMIXBINS MixBins; BINKSNDMIXBINVOLS MixBinVols;
 U32 sndbufsize;uint8_t *sndbuf,*sndend,*sndwritepos,*sndreadpos;
 uintptr_t sndcomp;U32 sndamt,sndconvert8,sndendframe,sndprime,sndpad;
 U32 BestSizeIn16,BestSizeMask,SoundDroppedOut;S32 NoThreadService,OnOff;
 U32 Latency,VideoScale,freq;S32 bits,chans;uint8_t snddata[256];
};
typedef S32 (BK_CALL *BINKSNDOPEN)(BINKSND*,U32,S32,S32,U32,HBINK);
typedef BINKSNDOPEN (BK_CALL *BINKSNDSYSOPEN)(uintptr_t);
typedef void *(BK_CALL *BINKIOOPEN)(void *);
#define BINKSURFACE24 1
#define BINKSURFACE24R 2
#define BINKSURFACE32 3
#define BINKSURFACE32R 4
#define BINKSURFACE32A 5
#define BINKSURFACE32RA 6
#define BINKSURFACE4444 7
#define BINKSURFACE5551 8
#define BINKSURFACE555 9
#define BINKSURFACE565 10
#define BINKSURFACE655 11
#define BINKSURFACE664 12
#define BINKSURFACEYUY2 13
#define BINKSURFACEUYVY 14
#define BINKFROMMEMORY 0x04000000u
#define BINKGETKEYPREVIOUS 0
#define BINKGETKEYNEXT 1
#define BINKGETKEYCLOSEST 2
#define BINKGETKEYNOTEQUAL 128
/* All names below are present in the sample's export table; signatures which
   depend on third-party API types require further differential validation. */
BK_API HBINKBUFFER BK_CALL BinkBufferOpen(void*,U32,U32,U32);
BK_API void BK_CALL BinkBufferCheckWinPos(HBINKBUFFER,S32*,S32*);
BK_API S32 BK_CALL BinkBufferClear(HBINKBUFFER,U32);
BK_API void BK_CALL BinkBufferClose(HBINKBUFFER);
BK_API char* BK_CALL BinkBufferGetDescription(HBINKBUFFER);
BK_API char* BK_CALL BinkBufferGetError(void);
BK_API S32 BK_CALL BinkBufferLock(HBINKBUFFER);
BK_API S32 BK_CALL BinkBufferSetDirectDraw(void*,void*);
BK_API S32 BK_CALL BinkBufferSetHWND(HBINKBUFFER,void*);
BK_API S32 BK_CALL BinkBufferSetOffset(HBINKBUFFER,S32,S32);
BK_API void BK_CALL BinkBufferSetResolution(S32,S32,S32);
BK_API S32 BK_CALL BinkBufferSetScale(HBINKBUFFER,U32,U32);
BK_API S32 BK_CALL BinkBufferUnlock(HBINKBUFFER);
BK_API void BK_CALL BinkBufferBlit(HBINKBUFFER,BINKRECT*,U32);
BK_API S32 BK_CALL BinkCheckCursor(void*,S32,S32,S32,S32);
BK_API void BK_CALL BinkClose(HBINK);
BK_API void BK_CALL BinkCloseTrack(HBINKTRACK);
BK_API S32 BK_CALL BinkControlBackgroundIO(HBINK,U32);
BK_API S32 BK_CALL BinkControlPlatformFeatures(S32,S32);
BK_API S32 BK_CALL BinkCopyToBuffer(HBINK,void*,S32,U32,U32,U32,U32);
BK_API S32 BK_CALL BinkCopyToBufferRect(HBINK,void*,S32,U32,U32,U32,U32,U32,U32,U32,U32);
BK_API S32 BK_CALL BinkDDSurfaceType(void*);
BK_API S32 BK_CALL BinkDX8SurfaceType(void*);
BK_API S32 BK_CALL BinkDX9SurfaceType(void*);
BK_API S32 BK_CALL BinkDoFrame(HBINK);
BK_API char* BK_CALL BinkGetError(void);
BK_API void BK_CALL BinkGetFrameBuffersInfo(HBINK,BINKFRAMEBUFFERS*);
BK_API U32 BK_CALL BinkGetKeyFrame(HBINK,U32,S32);
BK_API void BK_CALL BinkGetPalette(void*);
BK_API void BK_CALL BinkGetRealtime(HBINK,BINKREALTIME*,U32);
BK_API S32 BK_CALL BinkGetRects(HBINK,U32);
BK_API void BK_CALL BinkGetSummary(HBINK,BINKSUMMARY*);
BK_API U32 BK_CALL BinkGetTrackData(HBINKTRACK,void*);
BK_API U32 BK_CALL BinkGetTrackID(HBINK,U32);
BK_API U32 BK_CALL BinkGetTrackMaxSize(HBINK,U32);
BK_API U32 BK_CALL BinkGetTrackType(HBINK,U32);
BK_API void BK_CALL BinkGoto(HBINK,U32,S32);
BK_API S32 BK_CALL BinkIsSoftwareCursor(void*,void*);
BK_API void* BK_CALL BinkLogoAddress(void);
BK_API void BK_CALL BinkNextFrame(HBINK);
BK_API HBINK BK_CALL BinkOpen(const char*,U32);
BK_API BINKSNDOPEN BK_CALL BinkOpenDirectSound(uintptr_t);
BK_API BINKSNDOPEN BK_CALL BinkOpenMiles(uintptr_t);
BK_API HBINKTRACK BK_CALL BinkOpenTrack(HBINK,U32);
BK_API BINKSNDOPEN BK_CALL BinkOpenWaveOut(uintptr_t);
BK_API S32 BK_CALL BinkPause(HBINK,S32);
BK_API void BK_CALL BinkRegisterFrameBuffers(HBINK,BINKFRAMEBUFFERS*);
BK_API void BK_CALL BinkRestoreCursor(S32);
BK_API void BK_CALL BinkService(HBINK);
BK_API void BK_CALL BinkSetError(const char*);
BK_API void BK_CALL BinkSetFrameRate(U32,U32);
BK_API void BK_CALL BinkSetIO(BINKIOOPEN);
BK_API void BK_CALL BinkSetIOSize(U32);
BK_API void BK_CALL BinkSetMemory(BINKMEMALLOC,BINKMEMFREE);
BK_API void BK_CALL BinkSetMixBinVolumes(HBINK,U32,U32*,S32*,U32);
BK_API void BK_CALL BinkSetMixBins(HBINK,U32,U32*,U32);
BK_API void BK_CALL BinkSetPan(HBINK,U32,S32);
BK_API void BK_CALL BinkSetSimulate(U32);
BK_API S32 BK_CALL BinkSetSoundOnOff(HBINK,S32);
BK_API S32 BK_CALL BinkSetSoundSystem(BINKSNDSYSOPEN,uintptr_t);
BK_API void BK_CALL BinkSetSoundTrack(U32,U32*);
BK_API S32 BK_CALL BinkSetVideoOnOff(HBINK,S32);
BK_API void BK_CALL BinkSetVolume(HBINK,U32,S32);
BK_API S32 BK_CALL BinkShouldSkip(HBINK);
BK_API S32 BK_CALL BinkWait(HBINK);
BK_API U32 BK_CALL RADTimerRead(void);
#ifdef __cplusplus
}
#endif
#endif
