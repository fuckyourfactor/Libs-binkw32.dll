#ifndef BK_PRIVATE_H
#define BK_PRIVATE_H
#include "bink32.h"
#include "container.h"
typedef struct BkHandle {
 BINK api;
 BkIndex index;
 unsigned char *file;
 char *path;
 unsigned char *rgb;
 uint32_t frame_decoded;
 uint32_t next_tick;
 uint32_t last_decode_ms;
 uint32_t played;
 uint32_t video_enabled, sound_enabled;
 void *decoder;
 BINKMEMFREE rgb_free;
 void *audio_device;
 uint32_t audio_track;
 int volume,pan;
 uint32_t last_audio_decode_ms;

 uint32_t trace_rect_count, trace_copy_count, trace_copy_failure_count;

} BkHandle;

int bk_decoder_open(BkHandle*,const char*);
int bk_decoder_frame(BkHandle*,uint32_t frame_zero_based);
void bk_decoder_close(BkHandle*);

int bk_audio_get(BkHandle*,uint32_t track,uint32_t frame,
                 const uint8_t **pcm,uint32_t *bytes);
#endif
