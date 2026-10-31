#include "audio.h"
void *bk_wave_open(uint32_t hz,uint32_t channels){(void)hz;(void)channels;return 0;}
int bk_wave_submit(void*w,const int16_t*p,uint32_t n,int v,int pan){(void)w;(void)p;(void)n;(void)v;(void)pan;return 0;}
void bk_wave_pause(void *w,int p){(void)w;(void)p;}
void bk_wave_close(void *w){(void)w;}
