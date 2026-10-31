#ifndef BK_AUDIO_H
#define BK_AUDIO_H
#include <stdint.h>
/* WaveOut stream uses signed 16-bit interleaved PCM; volume and pan scaling
 * below are approximation of the 2005 API pending comparative verification. */
void bk_audio_mix(int16_t *dest,const int16_t *src,uint32_t frames,uint32_t channels,int volume,int pan);
void *bk_wave_open(uint32_t hz,uint32_t channels);
int bk_wave_submit(void *w,const int16_t *pcm,uint32_t bytes,int volume,int pan);
void bk_wave_pause(void *w,int paused);
void bk_wave_close(void *w);
#endif
