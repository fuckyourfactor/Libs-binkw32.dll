/* Metadata-only configuration. Decoder requests must fail visibly. */
#include "private.h"
int bk_decoder_open(BkHandle *h,const char *path){(void)h;(void)path;return 0;}
int bk_decoder_frame(BkHandle *h,uint32_t f){(void)h;(void)f;return 0;}
void bk_decoder_close(BkHandle *h){(void)h;}

int bk_audio_get(BkHandle*h,uint32_t t,uint32_t f,const uint8_t**p,uint32_t*n){
 (void)h;(void)t;(void)f;if(p)*p=0;if(n)*n=0;return 0;
}
