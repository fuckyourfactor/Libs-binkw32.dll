#include "audio.h"
#include "platform.h"
#include <stddef.h>
#include <stdint.h>
#define BK_WAVE_SLOTS 16
#define WHDR_DONE 1u
#define WAVE_MAPPER 0xFFFFFFFFu
#define WAVE_FORMAT_PCM 1u

typedef struct {uint16_t format,channels;uint32_t rate,avg;uint16_t align,bits,extra;} BK_WAVEFORMAT;
typedef struct BK_WAVEHDR {char *data;uint32_t len,recorded;uintptr_t user;uint32_t flags,loops;struct BK_WAVEHDR *next;uintptr_t reserved;} BK_WAVEHDR;
__declspec(dllimport) uint32_t __stdcall waveOutOpen(void **,uint32_t,const BK_WAVEFORMAT*,uintptr_t,uintptr_t,uint32_t);
__declspec(dllimport) uint32_t __stdcall waveOutPrepareHeader(void *,BK_WAVEHDR*,uint32_t);
__declspec(dllimport) uint32_t __stdcall waveOutUnprepareHeader(void *,BK_WAVEHDR*,uint32_t);
__declspec(dllimport) uint32_t __stdcall waveOutWrite(void *,BK_WAVEHDR*,uint32_t);
__declspec(dllimport) uint32_t __stdcall waveOutReset(void *);
__declspec(dllimport) uint32_t __stdcall waveOutPause(void *);
__declspec(dllimport) uint32_t __stdcall waveOutRestart(void *);
__declspec(dllimport) uint32_t __stdcall waveOutClose(void *);
typedef struct {void *dev;uint32_t channels;uint32_t next;BK_WAVEHDR headers[BK_WAVE_SLOTS];int prepared[BK_WAVE_SLOTS];} BkWave;
void *bk_wave_open(uint32_t hz,uint32_t channels){
 if(hz<8000||hz>192000||!(channels==1||channels==2))return NULL;
 BkWave *w=(BkWave*)bk_alloc(sizeof(*w));if(!w)return NULL;
 bk_memset(w,0,sizeof(*w));w->channels=channels;
 BK_WAVEFORMAT f={WAVE_FORMAT_PCM,(uint16_t)channels,hz,hz*channels*2u,(uint16_t)(channels*2),16,0};
 if(waveOutOpen(&w->dev,WAVE_MAPPER,&f,0,0,0)!=0){bk_free(w);return NULL;}
 return w;
}
int bk_wave_submit(void *opaque,const int16_t *pcm,uint32_t bytes,int volume,int pan){
 BkWave *w=(BkWave*)opaque;if(!w||!pcm||!bytes||bytes>8*1024*1024 || bytes%(w->channels*2))return 0;
 uint32_t slot=w->next;BK_WAVEHDR *h=&w->headers[slot];
 if(w->prepared[slot]){
  if(!(h->flags&WHDR_DONE))return 0;
  if(waveOutUnprepareHeader(w->dev,h,sizeof(*h))!=0)return 0;
  w->prepared[slot]=0;bk_free(h->data);bk_memset(h,0,sizeof(*h));
 }
 char *buffer=(char*)bk_alloc(bytes);if(!buffer)return 0;
 bk_audio_mix((int16_t*)buffer,pcm,bytes/(w->channels*2),w->channels,volume,pan);
 h->data=buffer;h->len=bytes;
 if(waveOutPrepareHeader(w->dev,h,sizeof(*h))!=0){bk_free(buffer);bk_memset(h,0,sizeof(*h));return 0;}
 w->prepared[slot]=1;
 if(waveOutWrite(w->dev,h,sizeof(*h))!=0){waveOutUnprepareHeader(w->dev,h,sizeof(*h));w->prepared[slot]=0;bk_free(buffer);bk_memset(h,0,sizeof(*h));return 0;}
 w->next=(slot+1)%BK_WAVE_SLOTS;return 1;
}
void bk_wave_pause(void *opaque,int p){BkWave*w=(BkWave*)opaque;if(w){if(p)waveOutPause(w->dev);else waveOutRestart(w->dev);}}
void bk_wave_close(void *opaque){
 BkWave *w=(BkWave*)opaque;if(!w)return;
 waveOutReset(w->dev);
 for(uint32_t i=0;i<BK_WAVE_SLOTS;i++)if(w->prepared[i]){
  waveOutUnprepareHeader(w->dev,&w->headers[i],sizeof(w->headers[i]));bk_free(w->headers[i].data);
 }
 waveOutClose(w->dev);bk_free(w);
}
