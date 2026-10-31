/* SPDX-License-Identifier: MIT
 * Partial clean-room reconstruction: uses no code or runtime services from the
 * original Bink DLL. Unsupported features report errors rather than fake success.
 */
#include "private.h"
#include "platform.h"
#include "pixel.h"
#include "audio.h"
#include "surface.h"
#include "trace.h"
#define HANDLE(x) ((BkHandle*)(x))
static char g_error[256];
static char g_buffer_error[256];
static U32 g_forced_num,g_forced_den,g_io_size,g_simulate;
static U32 g_timer_base;static int g_timer_initialized;
static BINKMEMALLOC g_allocator;static BINKMEMFREE g_deallocator;
static BINKIOOPEN g_io;
static U32 g_sound_tracks[256],g_sound_count;
static BINKSNDSYSOPEN g_sound_system;
static uintptr_t g_sound_param;
static void error(const char *s){unsigned i=0;if(!s)s="";while(i<255&&s[i]){g_error[i]=s[i];i++;}g_error[i]=0;}
static void buffer_error(const char *s){unsigned i=0;if(!s)s="";while(i<255&&s[i]){g_buffer_error[i]=s[i];i++;}g_buffer_error[i]=0;}
static void unsupported(const char *which){error(which);}
static char *strdup_local(const char *s){size_t n=0;char *p;if(!s)return NULL;while(s[n]){if(n>32767)return NULL;n++;}p=(char*)bk_alloc(n+1);if(p)bk_memcpy(p,s,n+1);return p;}
static void *alloc_local(size_t n){if(g_allocator&&g_deallocator)return g_allocator((U32)n);return bk_alloc(n);}
static uint32_t period_ms(const BINK *b){double t;if(!b||!b->FrameRate)return 0;t=1000.0*(double)b->FrameRateDiv/(double)b->FrameRate;return t>=4294967295.0?UINT32_MAX:(t>=1.0?(uint32_t)t:1);}
static int valid(HBINK b){return b!=NULL && b->FrameRate>0 && b->Frames>0;}
static void update_frame_sizes(BkHandle *h){
 U32 frame=h->api.FrameNum-1,start=h->index.offset[frame],end=h->index.offset[frame+1];
 h->api.FrameSize=end-start;h->api.SndSize=0;
 for(U32 t=0;t<h->index.tracks;t++){
  const uint8_t *p;U32 n;
  if(bk_packet(&h->index,frame,t,&p,&n))h->api.SndSize+=n;
 }
}
BK_API void BK_CALL BinkSetError(const char *s){error(s);}
BK_API char* BK_CALL BinkGetError(void){return g_error;}
BK_API char* BK_CALL BinkBufferGetError(void){return g_buffer_error;}
BK_API HBINK BK_CALL BinkOpen(const char *name,U32 flags){
 size_t len=0;uint8_t *data=NULL;BkHandle *h=NULL;
 error("");
 if(!name){error("BinkOpen: null source");bk_trace("BinkOpen null source flags=0x%08X",flags);return NULL;}
 const int from_memory=(flags&BINKFROMMEMORY)!=0;
 if(from_memory)bk_trace("BinkOpen memory source flags=0x%08X",flags);
 else bk_trace("BinkOpen path=%.220s flags=0x%08X",name,flags);
 if(flags & (0x00800000u|0x02000000u)){
  error("BinkOpen: existing-handle and custom-IO sources not implemented");return NULL;
 }
 data=from_memory?bk_load_memory(name,&len):bk_load_file(name,&len);
 if(!data){
  error(from_memory?"BinkOpen: unreadable or invalid memory source":"BinkOpen: file inaccessible or exceeds 256 MiB");
  bk_trace("BinkOpen failed to read %s",from_memory?"memory source":"file");return NULL;
 }
 h=(BkHandle*)bk_alloc(sizeof(*h));
 if(!h){bk_free(data);error("Out of memory");return NULL;}
 bk_memset(h,0,sizeof(*h));
 if(!bk_parse(data,len,&h->index)){
  bk_free(data);bk_free(h);error("BinkOpen: invalid or unsupported Bink 1 container");bk_trace("BinkOpen invalid Bink header / frame index");return NULL;
 }
 h->file=data;
 h->path=from_memory?NULL:strdup_local(name);
 h->api.Width=h->index.width;h->api.Height=h->index.height;
 h->api.Frames=h->index.frames;h->api.FrameNum=1;
 h->api.FrameRate=(g_forced_num && (flags&0x1000u))?g_forced_num:h->index.fps_num;
 h->api.FrameRateDiv=(g_forced_den && (flags&0x1000u))?g_forced_den:h->index.fps_den;
 h->api.OpenFlags=flags;h->api.BinkType=h->index.flags;h->api.Size=(U32)h->index.file_size;
 h->api.LargestFrameSize=h->index.largest_frame;
 h->api.InternalFrames=h->index.internal_frames;
 h->api.NumTracks=(S32)h->index.tracks;
 h->video_enabled=1;h->sound_enabled=1;h->next_tick=bk_millis();
 h->volume=32768;h->pan=32768;h->audio_track=UINT32_MAX;
 if(h->index.tracks && g_sound_count==0)h->audio_track=0;
 else for(U32 selected=0;selected<g_sound_count;selected++){
  /* BinkSetSoundTrack normally receives zero-based stream indices.
   * Accept matching stream IDs as a fallback for older integrations. */
  U32 wanted=g_sound_tracks[selected];
  if(wanted<h->index.tracks){h->audio_track=wanted;break;}
  for(U32 t=0;t<h->index.tracks;t++)
   if(h->index.track_ids[t]==wanted){h->audio_track=t;break;}
  if(h->audio_track!=UINT32_MAX)break;
 }

 h->rgb_free=(g_allocator&&g_deallocator)?g_deallocator:NULL;
 h->rgb=(unsigned char*)alloc_local((size_t)h->api.Width*h->api.Height*4);
 if(!h->rgb||(!from_memory&&!h->path)){BinkClose(&h->api);error("Out of memory");return NULL;}
 update_frame_sizes(h);
 if(!bk_decoder_open(h,from_memory?NULL:name)){
#ifdef BINK_WITH_FFMPEG
  /* Never return a movie handle that is guaranteed to fail at DoFrame. */
  error("BinkOpen: FFmpeg could not initialize this Bink video");
  bk_trace("BinkOpen FFmpeg decoder initialization FAILED");
  BinkClose(&h->api);
  return NULL;
#else
  error("BinkOpen: parsed; video decoder unavailable (build with BINK_WITH_FFMPEG)");
#endif
 }
 bk_trace("BinkOpen OK %ux%u frames=%u audio_tracks=%u selected=%u",h->api.Width,h->api.Height,h->api.Frames,h->index.tracks,h->audio_track);
 return &h->api;
}
BK_API void BK_CALL BinkClose(HBINK b){
 BkHandle *h;if(!b)return;h=HANDLE(b);
 bk_wave_close(h->audio_device);bk_decoder_close(h);bk_index_free(&h->index);if(h->rgb){if(h->rgb_free)h->rgb_free(h->rgb);else bk_free(h->rgb);}bk_free(h->path);bk_free(h->file);bk_free(h);
}
BK_API S32 BK_CALL BinkDoFrame(HBINK b){
 BkHandle *h;if(!valid(b))return 0;h=HANDLE(b);
 if(h->frame_decoded==b->FrameNum)return 1;
 uint32_t start=bk_millis();
 if(h->video_enabled){
  if(!bk_decoder_frame(h,b->FrameNum-1)){
   error("BinkDoFrame: video decoding failed or unavailable");bk_trace("BinkDoFrame VIDEO FAILED frame=%u",b->FrameNum);return 0;
  }
  h->last_decode_ms=bk_millis()-start;
  b->NumRects=1;b->FrameRects[0]=(BINKRECT){0,0,(S32)b->Width,(S32)b->Height};
 } else b->NumRects=0;
 if(h->sound_enabled && h->audio_track!=UINT32_MAX && g_sound_system){
  const uint8_t *pcm=NULL;U32 size=0;
  uint32_t astart=bk_millis();
  if(!bk_audio_get(h,h->audio_track,b->FrameNum-1,&pcm,&size)){
   /* An unavailable audio device or malformed audio packet must not prevent
    * displaying a valid decoded video frame. */
   bk_trace("BinkDoFrame audio decode unavailable frame=%u track=%u",b->FrameNum,h->audio_track);
   size=0;
  }
  h->last_audio_decode_ms=bk_millis()-astart;
  if(!h->audio_device){
   U32 track=h->audio_track;
   h->audio_device=bk_wave_open(h->index.track_rates[track],
     (h->index.track_types[track]&0x2000u)?2:1);
   if(!h->audio_device){bk_trace("BinkDoFrame WaveOut device unavailable, video continues");h->sound_enabled=0;}
  }
  if(size && h->audio_device && !bk_wave_submit(h->audio_device,(const int16_t*)pcm,size,h->volume,h->pan)){
   bk_trace("BinkDoFrame audio output queue full frame=%u; video continues",b->FrameNum);
  }
 }
 b->LastFrameNum=b->FrameNum;h->frame_decoded=b->FrameNum;
 h->played++;
 if(b->FrameNum<=3)bk_trace("BinkDoFrame decoded frame=%u size=%u",b->FrameNum,b->FrameSize);
 return 1;
}
BK_API void BK_CALL BinkNextFrame(HBINK b){
 if(!valid(b))return;
 if(b->FrameNum>=b->Frames)b->FrameNum=1;else b->FrameNum++;
 BkHandle *h=HANDLE(b);h->next_tick+=period_ms(b);update_frame_sizes(h);
}
BK_API void BK_CALL BinkGoto(HBINK b,U32 frame,S32 flags){
 (void)flags;if(!valid(b))return;
 if(frame==0)frame=1;if(frame>b->Frames)frame=b->Frames;
 b->FrameNum=frame;HANDLE(b)->next_tick=bk_millis();update_frame_sizes(HANDLE(b));
}
BK_API U32 BK_CALL BinkGetKeyFrame(HBINK b,U32 frame,S32 flags){
 if(!valid(b))return 0;
 BkIndex *x=&HANDLE(b)->index;U32 prev=0,next=0;
 if(frame<1)frame=1;if(frame>x->frames)frame=x->frames;
 for(U32 i=0;i<x->frames;i++)if(x->key[i] && (i+1!=frame || !(flags&BINKGETKEYNOTEQUAL))){
  if(i+1<=frame)prev=i+1;
  if(!next && i+1>=frame)next=i+1;
 }
 switch(flags&3){case BINKGETKEYNEXT:return next;case BINKGETKEYCLOSEST:
  if(!prev)return next;if(!next)return prev;return (frame-prev<=next-frame)?prev:next;
 default:return prev;}
}
BK_API S32 BK_CALL BinkPause(HBINK b,S32 p){
 if(!valid(b))return 0;S32 old=b->Paused;b->Paused=p!=0;if(old && !b->Paused)HANDLE(b)->next_tick=bk_millis();
 bk_wave_pause(HANDLE(b)->audio_device,b->Paused);return old;
}
BK_API S32 BK_CALL BinkWait(HBINK b){
 if(!valid(b))return 0;if(b->Paused)return 1;
 return (S32)((int32_t)(HANDLE(b)->next_tick-bk_millis())>0);
}
BK_API S32 BK_CALL BinkShouldSkip(HBINK b){
 if(!valid(b))return 0;return (S32)((int32_t)(bk_millis()-HANDLE(b)->next_tick)>(int32_t)(period_ms(b)*2));
}
BK_API void BK_CALL BinkService(HBINK b){(void)b;unsupported("BinkService: background IO and audio servicing unavailable");}
BK_API S32 BK_CALL BinkGetRects(HBINK b,U32 flags){
 (void)flags;if(!valid(b)||HANDLE(b)->frame_decoded!=b->FrameNum)return 0;
 b->NumRects=1;b->FrameRects[0]=(BINKRECT){0,0,(S32)b->Width,(S32)b->Height};return 1;
}
/* Portable, bounds-checked surface conversion from a genuinely decoded frame. */
BK_API S32 BK_CALL BinkCopyToBufferRect(HBINK b,void *dst,S32 pitch,U32 dh,U32 dx,U32 dy,U32 sx,U32 sy,U32 sw,U32 sh,U32 flags){
 if(!valid(b)||!dst||HANDLE(b)->frame_decoded!=b->FrameNum){error("No decoded frame available");return 0;}
 if(!bk_pixels_copy(HANDLE(b)->rgb,b->Width,b->Height,dst,pitch,dh,dx,dy,sx,sy,sw,sh,flags)){
  error("Invalid surface, stride, rectangle, or unsupported format");bk_trace("BinkCopyToBufferRect FAILED format=%u pitch=%d dstheight=%u dst=(%u,%u) src=(%u,%u %ux%u)",flags,pitch,dh,dx,dy,sx,sy,sw,sh);return 0;
 }
 return 1;
}
BK_API S32 BK_CALL BinkCopyToBuffer(HBINK b,void *dst,S32 pitch,U32 h,U32 x,U32 y,U32 flags){
 if(!valid(b))return 0;return BinkCopyToBufferRect(b,dst,pitch,h,x,y,0,0,b->Width,b->Height,flags);
}
BK_API U32 BK_CALL BinkGetTrackID(HBINK b,U32 t){return valid(b)&&t<(U32)b->NumTracks?HANDLE(b)->index.track_ids[t]:0;}
BK_API U32 BK_CALL BinkGetTrackType(HBINK b,U32 t){return valid(b)&&t<(U32)b->NumTracks?HANDLE(b)->index.track_types[t]:0;}
BK_API U32 BK_CALL BinkGetTrackMaxSize(HBINK b,U32 t){return valid(b)&&t<(U32)b->NumTracks?HANDLE(b)->index.track_sizes[t]:0;}
BK_API HBINKTRACK BK_CALL BinkOpenTrack(HBINK b,U32 t){
 if(!valid(b)||t>=(U32)b->NumTracks)return NULL;
 BINKTRACK *tr=(BINKTRACK*)bk_alloc(sizeof(*tr));if(!tr)return NULL;
 bk_memset(tr,0,sizeof(*tr));tr->bink=b;tr->trackindex=(S32)t;tr->MaxSize=BinkGetTrackMaxSize(b,t);tr->Frequency=HANDLE(b)->index.track_rates[t];
 U32 typ=BinkGetTrackType(b,t);tr->Bits=16;tr->Channels=(typ&0x2000u)?2:1;
 return tr;
}
BK_API void BK_CALL BinkCloseTrack(HBINKTRACK t){bk_free(t);}
BK_API U32 BK_CALL BinkGetTrackData(HBINKTRACK t,void *dst){
 if(!t||!dst||!valid(t->bink)||t->trackindex<0 ||
    (U32)t->trackindex>=(U32)t->bink->NumTracks){error("Invalid audio track or destination");return 0;}
 const uint8_t *pcm=NULL;uint32_t bytes=0;
 if(!bk_audio_get(HANDLE(t->bink),(uint32_t)t->trackindex,
                  t->bink->FrameNum-1,&pcm,&bytes)){
  error("BinkGetTrackData: audio decoder unavailable or malformed packet");return 0;
 }
 if(bytes>t->MaxSize){error("BinkGetTrackData: decoded data exceeds public track MaxSize");return 0;}
 if(bytes)bk_memcpy(dst,pcm,bytes);
 return bytes;
}
BK_API void BK_CALL BinkGetSummary(HBINK b,BINKSUMMARY *s){
 if(!valid(b)||!s)return;bk_memset(s,0,sizeof(*s));BkHandle *h=HANDLE(b);
 s->Width=b->Width;s->Height=b->Height;s->TotalFrames=b->Frames;
 s->FrameRate=b->FrameRate;s->FrameRateDiv=b->FrameRateDiv;
 s->FileFrameRate=h->index.fps_num;s->FileFrameRateDiv=h->index.fps_den;
 s->TotalTime=(U32)((double)b->Frames*b->FrameRateDiv*1000.0/b->FrameRate);
 s->TotalPlayedFrames=h->played;s->AverageFrameSize=b->Size/b->Frames;
 s->AverageDataRate=(U32)((double)b->Size*1000.0/(s->TotalTime?s->TotalTime:1));
}
BK_API void BK_CALL BinkGetRealtime(HBINK b,BINKREALTIME *r,U32 frames){
 (void)frames;if(!valid(b)||!r)return;bk_memset(r,0,sizeof(*r));
 r->FrameNum=b->FrameNum;r->FrameRate=b->FrameRate;r->FrameRateDiv=b->FrameRateDiv;
 r->Frames=(HANDLE(b)->frame_decoded>0);r->FramesVideoDecompTime=HANDLE(b)->last_decode_ms;
 r->FramesTime=period_ms(b)*r->Frames;
 r->FramesAudioDecompTime=HANDLE(b)->last_audio_decode_ms;
}
BK_API void BK_CALL BinkGetFrameBuffersInfo(HBINK b,BINKFRAMEBUFFERS *f){
 if(!valid(b)||!f)return;bk_memset(f,0,sizeof(*f));f->TotalFrames=2;
 f->YABufferWidth=b->Width;f->YABufferHeight=b->Height;
 f->cRcBBufferWidth=(b->Width+1)/2;f->cRcBBufferHeight=(b->Height+1)/2;
}
BK_API void BK_CALL BinkRegisterFrameBuffers(HBINK b,BINKFRAMEBUFFERS *f){
 if(!valid(b)||!f)return; /* Bookkeeping only: decoder does not write YUV plane sets. */
 b->FrameBuffers=f;unsupported("Registered YUV buffers are not integrated into FFmpeg decoder");
}
BK_API S32 BK_CALL BinkSetVideoOnOff(HBINK b,S32 on){if(!valid(b))return 0;S32 prior=(S32)HANDLE(b)->video_enabled;HANDLE(b)->video_enabled=on!=0;return prior;}
BK_API S32 BK_CALL BinkSetSoundOnOff(HBINK b,S32 on){
 if(!valid(b))return 0;BkHandle *h=HANDLE(b);
 S32 prior=(S32)h->sound_enabled;h->sound_enabled=on!=0;
 bk_wave_pause(h->audio_device,!on);return prior;
}
BK_API void BK_CALL BinkSetVolume(HBINK b,U32 t,S32 v){
 if(!valid(b)||t>=(U32)b->NumTracks)return;
 HANDLE(b)->volume=v; /* Gain applied to subsequently queued PCM */
}
BK_API void BK_CALL BinkSetPan(HBINK b,U32 t,S32 p){
 if(!valid(b)||t>=(U32)b->NumTracks)return;
 HANDLE(b)->pan=p;
}
BK_API void BK_CALL BinkSetMixBins(HBINK b,U32 t,U32 *m,U32 n){(void)b;(void)t;(void)m;(void)n;unsupported("Mix bins unavailable");}
BK_API void BK_CALL BinkSetMixBinVolumes(HBINK b,U32 t,U32 *m,S32 *v,U32 n){(void)b;(void)t;(void)m;(void)v;(void)n;unsupported("Mix-bin volumes unavailable");}
BK_API S32 BK_CALL BinkControlBackgroundIO(HBINK b,U32 ctl){(void)b;(void)ctl;unsupported("Background IO not implemented");return 0;}
BK_API S32 BK_CALL BinkControlPlatformFeatures(S32 use,S32 exclude){(void)use;(void)exclude;unsupported("Platform feature CPU masks not implemented");return 0;}
BK_API void BK_CALL BinkSetFrameRate(U32 n,U32 d){if(n&&d){g_forced_num=n;g_forced_den=d;}}
BK_API void BK_CALL BinkSetIO(BINKIOOPEN io){g_io=io;unsupported("Custom IO callback not supported by BinkOpen");}
BK_API void BK_CALL BinkSetIOSize(U32 n){g_io_size=n;unsupported("Custom IO buffer sizing has no effect in buffered-file mode");}
BK_API void BK_CALL BinkSetSimulate(U32 s){g_simulate=s;unsupported("Simulated IO throttling unavailable");}
BK_API void BK_CALL BinkSetMemory(BINKMEMALLOC a,BINKMEMFREE f){g_allocator=a;g_deallocator=f;}
BK_API void BK_CALL BinkSetSoundTrack(U32 n,U32 *ids){
 if(n>256||(!ids&&n)){error("Invalid sound track list");return;}g_sound_count=n;
 if(n)bk_memcpy(g_sound_tracks,ids,(size_t)n*sizeof(U32));
}
BK_API S32 BK_CALL BinkSetSoundSystem(BINKSNDSYSOPEN o,uintptr_t p){
 if(!o){g_sound_system=NULL;g_sound_param=0;bk_trace("BinkSetSoundSystem disabled");return 1;}
 if(o!=BinkOpenWaveOut && o!=BinkOpenDirectSound && o!=BinkOpenMiles){
  error("Unknown sound system backend");bk_trace("BinkSetSoundSystem unknown backend");return 0;
 }
 /* DirectSound/Miles need not be loaded or initialized by the substitute.
  * The decoded PCM is routed independently to WinMM WaveOut. */
 g_sound_system=o;g_sound_param=p;
 bk_trace("BinkSetSoundSystem accepted backend=%s",o==BinkOpenMiles?"Miles (WaveOut fallback)":o==BinkOpenDirectSound?"DirectSound (WaveOut fallback)":"WaveOut");
 return 1;
}
extern S32 BK_CALL bk_sound_waveout_open(BINKSND*,U32,S32,S32,U32,HBINK);
BK_API BINKSNDOPEN BK_CALL BinkOpenDirectSound(uintptr_t p){
 (void)p; /* Valid callback, backed by WinMM without needing a DirectSound interface. */
 return bk_sound_waveout_open;
}
BK_API BINKSNDOPEN BK_CALL BinkOpenWaveOut(uintptr_t p){
 (void)p;return bk_sound_waveout_open;
}
BK_API BINKSNDOPEN BK_CALL BinkOpenMiles(uintptr_t p){
 (void)p; /* PCM playback uses WinMM, not the proprietary Miles DLL. */
 return bk_sound_waveout_open;
}
BK_API void BK_CALL BinkGetPalette(void *p){(void)p;unsupported("Palette is not exposed in the RGB backend");}
BK_API void* BK_CALL BinkLogoAddress(void){unsupported("Embedded proprietary logo intentionally not included");return NULL;}
BK_API U32 BK_CALL RADTimerRead(void){
 U32 now=bk_millis();
 if(!g_timer_initialized){g_timer_base=now;g_timer_initialized=1;return 0;}
 return now-g_timer_base; /* approximate milliseconds since first call */
}
BK_API S32 BK_CALL BinkIsSoftwareCursor(void *a,void *b){(void)a;(void)b;unsupported("Software cursor detection unavailable");return 0;}
BK_API S32 BK_CALL BinkCheckCursor(void *w,S32 x,S32 y,S32 cw,S32 ch){(void)w;(void)x;(void)y;(void)cw;(void)ch;unsupported("Software cursor clipping unavailable");return 0;}
BK_API void BK_CALL BinkRestoreCursor(S32 c){(void)c;unsupported("Software cursor restoration unavailable");}
BK_API S32 BK_CALL BinkDDSurfaceType(void *surface){return bk_dd_surface_type(surface);}
BK_API S32 BK_CALL BinkDX8SurfaceType(void *surface){return bk_d3d_surface_type(surface);}
BK_API S32 BK_CALL BinkDX9SurfaceType(void *surface){return bk_d3d_surface_type(surface);}
BK_API S32 BK_CALL BinkBufferSetDirectDraw(void *dd,void *surface){(void)dd;(void)surface;buffer_error("DirectDraw backing surfaces unsupported");return 0;}
/* CPU backing buffer is a working independently allocated 32-bit BGRA DIB. */
BK_API HBINKBUFFER BK_CALL BinkBufferOpen(void *window,U32 w,U32 h,U32 flags){
 if(!w||!h||w>7680||h>4800||w>UINT32_MAX/h/4){buffer_error("Invalid surface dimensions");return NULL;}
 if((flags&31u)!=0 && (flags&31u)!=2){buffer_error("Only auto/DIB section surfaces supported");return NULL;}
 BINKBUFFER *b=(BINKBUFFER*)bk_alloc(sizeof(*b));if(!b){buffer_error("Out of memory");return NULL;}
 bk_memset(b,0,sizeof(*b));b->Buffer=bk_alloc((size_t)w*h*4);
 if(!b->Buffer){bk_free(b);buffer_error("Out of memory");return NULL;}
 bk_memset(b->Buffer,0,(size_t)w*h*4);
 b->Width=w;b->Height=h;b->WindowWidth=w;b->WindowHeight=h;b->StretchWidth=w;b->StretchHeight=h;
 b->SurfaceType=BINKSURFACE32;b->BufferPitch=(S32)(w*4);b->wnd=window;b->type=2;return b;
}
BK_API void BK_CALL BinkBufferClose(HBINKBUFFER b){if(b){bk_free(b->Buffer);bk_free(b);}}
BK_API S32 BK_CALL BinkBufferLock(HBINKBUFFER b){return (b&&b->Buffer)?1:0;}
BK_API S32 BK_CALL BinkBufferUnlock(HBINKBUFFER b){return (b&&b->Buffer)?1:0;}
BK_API S32 BK_CALL BinkBufferClear(HBINKBUFFER b,U32 rgb){
 if(!b||!b->Buffer)return 0;
 unsigned char *dst=(unsigned char*)b->Buffer;
 for(size_t i=0;i<(size_t)b->Width*b->Height;i++){
  dst[4*i]=(unsigned char)(rgb&255);dst[4*i+1]=(unsigned char)((rgb>>8)&255);
  dst[4*i+2]=(unsigned char)((rgb>>16)&255);dst[4*i+3]=255;
 }
 return 1;
}
BK_API S32 BK_CALL BinkBufferSetHWND(HBINKBUFFER b,void *w){if(!b)return 0;b->wnd=w;return 1;}
BK_API S32 BK_CALL BinkBufferSetOffset(HBINKBUFFER b,S32 x,S32 y){if(!b)return 0;b->destx=x;b->desty=y;return 1;}
BK_API S32 BK_CALL BinkBufferSetScale(HBINKBUFFER b,U32 w,U32 h){if(!b||!w||!h||w>32768||h>32768)return 0;b->StretchWidth=w;b->StretchHeight=h;return 1;}
BK_API void BK_CALL BinkBufferSetResolution(S32 w,S32 h,S32 bits){(void)w;(void)h;(void)bits;buffer_error("Global display resolution changes intentionally unsupported");}
BK_API char* BK_CALL BinkBufferGetDescription(HBINKBUFFER b){(void)b;return "32-bit CPU DIB (independent implementation)";}
#ifdef _WIN32
typedef struct {S32 x,y;} BK_POINT;
typedef struct {S32 l,t,r,b;} BK_RECT;
__declspec(dllimport) int __stdcall ClientToScreen(void*,BK_POINT*);
__declspec(dllimport) int __stdcall GetClientRect(void*,BK_RECT*);
#endif
BK_API void BK_CALL BinkBufferCheckWinPos(HBINKBUFFER b,S32 *x,S32 *y){
#ifdef _WIN32
 if(b&&b->wnd){
  BK_POINT origin={0,0};BK_RECT client={0,0,0,0};
  if(ClientToScreen(b->wnd,&origin)){b->wndx=origin.x;b->wndy=origin.y;}
  if(GetClientRect(b->wnd,&client) && client.r>client.l&&client.b>client.t){
   b->WindowWidth=(U32)(client.r-client.l);b->WindowHeight=(U32)(client.b-client.t);
  }
 }
#endif
 if(x)*x=b?b->wndx:0;if(y)*y=b?b->wndy:0;
}
#ifdef _WIN32
/* gdi32: StretchDIBits to an existing Win32 window handle */
__declspec(dllimport) void* __stdcall GetDC(void*);
__declspec(dllimport) int __stdcall ReleaseDC(void*,void*);
__declspec(dllimport) int __stdcall StretchDIBits(void*,int,int,int,int,int,int,int,int,const void*,const void*,unsigned,int);
typedef struct {U32 size;S32 width,height;uint16_t planes,bits;U32 compression,sizeimage;S32 xppm,yppm;U32 colors,important;U32 masks[3];} BK_BITMAPINFO;
#endif
/* Render only requested dirty rectangles and map each rectangle to its scaled
 * destination coordinates. CPU buffer remains a top-down 32-bit DIB. */
BK_API void BK_CALL BinkBufferBlit(HBINKBUFFER b,BINKRECT *rect,U32 n){
 if(!b||!b->Buffer||!b->wnd)return;
#ifdef _WIN32
 BK_BITMAPINFO bi;bk_memset(&bi,0,sizeof(bi));bi.size=40;
 bi.width=(S32)b->Width;bi.height=-(S32)b->Height;bi.planes=1;bi.bits=32;
 void *dc=GetDC(b->wnd);
 if(!dc){buffer_error("GetDC failed");return;}
 U32 count=(rect&&n)?n:1;
 if(count>4096){buffer_error("Invalid dirty rectangle count");ReleaseDC(b->wnd,dc);return;}
 for(U32 i=0;i<count;i++){
  BINKRECT r=(rect&&n)?rect[i]:(BINKRECT){0,0,(S32)b->Width,(S32)b->Height};
  if(r.Left<0){r.Width+=r.Left;r.Left=0;}
  if(r.Top<0){r.Height+=r.Top;r.Top=0;}
  if(r.Width<=0||r.Height<=0||(U32)r.Left>=b->Width||(U32)r.Top>=b->Height)continue;
  if((U32)r.Width>b->Width-(U32)r.Left)r.Width=(S32)(b->Width-r.Left);
  if((U32)r.Height>b->Height-(U32)r.Top)r.Height=(S32)(b->Height-r.Top);
  int32_t x0=((uint32_t)r.Left*b->StretchWidth)/b->Width;
  int32_t y0=((uint32_t)r.Top*b->StretchHeight)/b->Height;
  int32_t x1=((uint32_t)(r.Left+r.Width)*b->StretchWidth+b->Width-1)/b->Width;
  int32_t y1=((uint32_t)(r.Top+r.Height)*b->StretchHeight+b->Height-1)/b->Height;
  int32_t width=x1-x0,height=y1-y0;
  if(width<1||height<1||x1>INT32_MAX||y1>INT32_MAX)continue;
  if(StretchDIBits(dc,b->destx+(int)x0,b->desty+(int)y0,(int)width,(int)height,
       r.Left,r.Top,r.Width,r.Height,b->Buffer,&bi,0,0x00CC0020u)<=0){
   buffer_error("StretchDIBits failed");
  }
 }
 ReleaseDC(b->wnd,dc);
#else
 (void)rect;(void)n;buffer_error("Window blit requires Win32 GDI");
#endif
}
