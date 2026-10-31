/* SPDX-License-Identifier: MIT
 * Public BINKSND-compatible WinMM adapter; supports WaveOut output without
 * the proprietary Bink audio system. Limited to 16-bit mono/stereo PCM.
 */
#include "bink32.h"
#include "audio.h"
#include "platform.h"
#include <stdint.h>
typedef struct {void *out;U32 channels;int volume,pan;int paused;} SoundUser;
static SoundUser *state(BINKSND *s){return s?(SoundUser*)(uintptr_t)s->sndcomp:NULL;}
static S32 BK_CALL snd_ready(BINKSND *s){return state(s)&&!state(s)->paused;}
static S32 BK_CALL snd_lock(BINKSND *s,uint8_t **buffer,U32 *size){
 if(!state(s)||!buffer||!size||!s->sndbuf||state(s)->paused)return 0;
 *buffer=s->sndbuf;*size=s->sndbufsize;return 1;
}
static S32 BK_CALL snd_unlock(BINKSND *s,U32 bytes){
 SoundUser*u=state(s);
 if(!u||!s->sndbuf||bytes>s->sndbufsize)return 0;
 if(!bytes)return 1;
 return bk_wave_submit(u->out,(const int16_t*)s->sndbuf,bytes,u->volume,u->pan);
}
static void BK_CALL snd_volume(BINKSND*s,S32 v){if(state(s))state(s)->volume=v;}
static void BK_CALL snd_pan(BINKSND*s,S32 p){if(state(s))state(s)->pan=p;}
static S32 BK_CALL snd_pause(BINKSND*s,S32 p){
 SoundUser*u=state(s);if(!u)return 0;
 S32 old=u->paused;u->paused=p!=0;bk_wave_pause(u->out,u->paused);return old;
}
static S32 BK_CALL snd_onoff(BINKSND*s,S32 on){return snd_pause(s,on?0:1);}
static void BK_CALL snd_close(BINKSND*s){
 SoundUser*u=state(s);if(!u)return;
 bk_wave_close(u->out);bk_free(u);bk_free(s->sndbuf);
 s->sndbuf=NULL;s->sndcomp=0;
}
S32 BK_CALL bk_sound_waveout_open(BINKSND*s,U32 hz,S32 bits,S32 channels,U32 flags,HBINK movie){
 (void)flags;(void)movie;
 if(!s||bits!=16||!(channels==1||channels==2))return 0;
 void *out=bk_wave_open(hz,(U32)channels);if(!out)return 0;
 SoundUser*u=(SoundUser*)bk_alloc(sizeof(*u));
 uint8_t *buf=(uint8_t*)bk_alloc(65536);
 if(!u||!buf){bk_wave_close(out);bk_free(u);bk_free(buf);return 0;}
 bk_memset(u,0,sizeof(*u));u->out=out;u->channels=(U32)channels;
 u->volume=32768;u->pan=32768;
 s->Ready=snd_ready;s->Lock=snd_lock;s->Unlock=snd_unlock;
 s->Volume=snd_volume;s->Pan=snd_pan;s->Pause=snd_pause;
 s->SetOnOff=snd_onoff;s->Close=snd_close;
 s->MixBins=NULL;s->MixBinVols=NULL; /* Unsupported speaker routing is explicit. */
 s->sndbufsize=65536;s->sndbuf=buf;s->sndend=buf+65536;
 s->sndwritepos=buf;s->sndreadpos=buf;s->sndcomp=(uintptr_t)u;
 s->freq=hz;s->bits=bits;s->chans=channels;s->OnOff=1;
 return 1;
}
