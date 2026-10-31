#ifndef BK_PRIVATE_H
#define BK_PRIVATE_H
#include "bink32.h"
#include "container.h"
typedef struct BkHandle {
 BINK api; /* must be first */
 BkIndex index;
 unsigned char *file;
 char *path;
 unsigned char *rgb;
 uint32_t frame_decoded; /* 1-based; zero means no decoded frame */
 uint32_t next_tick;
 uint32_t last_decode_ms;
 uint32_t played;
 uint32_t video_enabled, sound_enabled;
 void *decoder;
 BINKMEMFREE rgb_free;
 void *audio_device;      /* WinMM audio stream when available */
 uint32_t audio_track;   /* selected 0-based track, UINT32_MAX = none */
 int volume,pan;         /* playback gain, pan */
 uint32_t last_audio_decode_ms;
 /* Diagnostic rate limits: log the first few host presentation requests even
  * if they occur after frame 3. These fields are private, after BINK. */
 uint32_t trace_rect_count, trace_copy_count, trace_copy_failure_count;

} BkHandle;
/* Returns 1 only on an actual decoded frame, never on success-shaped placeholders. */
int bk_decoder_open(BkHandle*,const char*);
int bk_decoder_frame(BkHandle*,uint32_t frame_zero_based);
void bk_decoder_close(BkHandle*);
/* Decode a given Bink audio track/frame to owned interleaved PCM16.
 * The returned pointer is valid until the next audio request for that track. */
int bk_audio_get(BkHandle*,uint32_t track,uint32_t frame,
                 const uint8_t **pcm,uint32_t *bytes);
#endif
