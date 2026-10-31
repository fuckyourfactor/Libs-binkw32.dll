#include "bink32.h"
#include "private.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

_Static_assert(offsetof(BINK, Width) == 0, "Bink Width ABI");
_Static_assert(offsetof(BINK, FrameNum) == 12, "Bink FrameNum ABI");

_Static_assert(offsetof(BINK, FrameRects) == 52, "Bink 1.8f dirty rect ABI");
_Static_assert(offsetof(BINK, NumRects) == 180, "Bink 1.8f rect count ABI");
#if UINTPTR_MAX == UINT32_MAX
_Static_assert(offsetof(BINK, FrameBuffers) == 184, "Bink 1.8f frame buffer ABI");
_Static_assert(offsetof(BINK, LargestFrameSize) == 212, "Bink 1.8f frame size ABI");
_Static_assert(offsetof(BINK, NumTracks) == 220, "Bink 1.8f track count ABI");
#endif
int main(void) {
  BkHandle h;
  unsigned char bgra[4 * 4 * 4];
  unsigned char dst[4 * 4 * 4];
  memset(&h, 0, sizeof(h));
  for (unsigned i = 0; i < 16; ++i) {
    bgra[4*i] = (unsigned char)(i+1);
    bgra[4*i+1] = (unsigned char)(i+10);
    bgra[4*i+2] = (unsigned char)(i+50);
    bgra[4*i+3] = 255;
  }
  h.api.Width = 4;
  h.api.Height = 4;
  h.api.Frames = 3;
  h.api.FrameNum = 1;
  h.api.FrameRate = 30;
  h.api.FrameRateDiv = 1;
  h.frame_decoded = 1;
  h.rgb = bgra;

  assert(BinkGetRects(&h.api, BINKSURFACE32 | BINKSURFACEDIRECT) == 0);
  assert(BinkGetRects(&h.api, BINKSURFACE32) == 1);
  assert(h.api.NumRects == 1);
  assert(h.api.FrameRects[0].Left == 0);
  assert(h.api.FrameRects[0].Top == 0);
  assert(h.api.FrameRects[0].Width == 4);
  assert(h.api.FrameRects[0].Height == 4);

  assert(*(S32 *)((unsigned char *)&h.api + 52 + 8) == 4);
  assert(*(S32 *)((unsigned char *)&h.api + 52 + 12) == 4);
  assert(*(S32 *)((unsigned char *)&h.api + 180) == 1);
  memset(dst, 0xCC, sizeof(dst));
  assert(BinkCopyToBuffer(&h.api, dst, 16, 4, 0, 0,
                          BINKSURFACE32 | 0x80000000u));
  assert(memcmp(bgra, dst, sizeof(bgra)) == 0);
  memset(dst, 0xCC, sizeof(dst));
  assert(BinkCopyToBufferRect(&h.api, dst, 16, 4, 1, 1, 1, 1, 2, 2,
                              BINKSURFACE32));
  assert(memcmp(dst+1*16+4, bgra+1*16+4, 8)==0);
  assert(memcmp(dst+2*16+4, bgra+2*16+4, 8)==0);
  assert(dst[0] == 0xCC);

  U32 frame_offsets[4] = {0, 16, 32, 48};
  h.index.frames = 3;
  h.index.offset = frame_offsets;
  h.api.LastFrameNum = 1;
  BinkNextFrame(&h.api);
  assert(h.api.FrameNum == 2 && h.frame_decoded == 1);
  assert(BinkGetRects(&h.api, BINKSURFACE32) == 1);
  assert(h.api.NumRects == 1);
  memset(dst, 0xCC, sizeof(dst));
  assert(BinkCopyToBufferRect(&h.api, dst, 16, 4, 0, 0, 0, 0, 4, 4,
                              BINKSURFACE32));
  assert(memcmp(dst, bgra, sizeof(dst)) == 0);

  memset(dst, 0xCC, sizeof(dst));
  assert(BinkCopyToBuffer(&h.api, dst, 16, 4, 0, 0, BINKSURFACE32));
  assert(memcmp(dst, bgra, sizeof(dst)) == 0);

  BinkGoto(&h.api, 3, 0);
  assert(h.api.FrameNum == 3 && h.frame_decoded == 0);
  assert(BinkGetRects(&h.api, BINKSURFACE32) == 0);
  assert(BinkCopyToBuffer(&h.api, dst, 16, 4, 0, 0, BINKSURFACE32) == 0);

  h.frame_decoded = 3;
  h.api.LastFrameNum = 3;
  assert(BinkCopyToBufferRect(&h.api, dst, 16, 4, 0, 0, 0, 0, 4, 4,
                              BINKSURFACE32));
  h.frame_decoded = 0;
  assert(!BinkCopyToBuffer(&h.api,dst,16,4,0,0,BINKSURFACE32));
  assert(!BinkGetRects(&h.api,BINKSURFACE32));
  puts("rFactor decode -> NextFrame -> Copy, seek invalidation and Bink 1.8f ABI verified");
  return 0;
}
