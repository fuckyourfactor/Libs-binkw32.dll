/* Bink1 Win32 ABI regression + rendered-frame handoff tests.
 * A frame can decode, but it will not be visible in games if the public
 * BINKRECT/NumRects fields are one DWORD away from their expected offsets.
 * No proprietary media or runtime library is needed for this test. */
#include "bink32.h"
#include "private.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

_Static_assert(offsetof(BINK, Width) == 0, "Bink Width ABI");
_Static_assert(offsetof(BINK, FrameNum) == 12, "Bink FrameNum ABI");
_Static_assert(offsetof(BINK, FrameChangePercent) == 52, "Bink 1 change percentage ABI");
_Static_assert(offsetof(BINK, FrameRects) == 56, "Bink 1 dirty rect ABI");
_Static_assert(offsetof(BINK, NumRects) == 184, "Bink 1 rect count ABI");
#if UINTPTR_MAX == UINT32_MAX
_Static_assert(offsetof(BINK, FrameBuffers) == 188, "Bink Win32 frame buffer ABI");
_Static_assert(offsetof(BINK, LargestFrameSize) == 216, "Bink Win32 frame size ABI");
_Static_assert(offsetof(BINK, NumTracks) == 224, "Bink Win32 track count ABI");
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
  h.api.Frames = 1;
  h.api.FrameNum = 1;
  h.api.FrameRate = 30;
  h.api.FrameRateDiv = 1;
  h.frame_decoded = 1;
  h.rgb = bgra;
  assert(BinkGetRects(&h.api, BINKSURFACE32) == 1);
  assert(h.api.NumRects == 1);
  assert(h.api.FrameRects[0].Left == 0);
  assert(h.api.FrameRects[0].Top == 0);
  assert(h.api.FrameRects[0].Width == 4);
  assert(h.api.FrameRects[0].Height == 4);
  /* This is exactly what rFactor's host reads from the 32-bit ABI. */
  assert(*(S32 *)((unsigned char *)&h.api + 56 + 8) == 4);
  assert(*(S32 *)((unsigned char *)&h.api + 56 + 12) == 4);
  assert(*(S32 *)((unsigned char *)&h.api + 184) == 1);
  memset(dst, 0xCC, sizeof(dst));
  assert(BinkCopyToBuffer(&h.api, dst, 16, 4, 0, 0,
                          BINKSURFACE32 | 0x80000000u));
  assert(memcmp(bgra, dst, sizeof(bgra)) == 0);
  memset(dst, 0xCC, sizeof(dst));
  assert(BinkCopyToBufferRect(&h.api, dst, 16, 4, 1, 1, 1, 1, 2, 2,
                              BINKSURFACE32));
  assert(memcmp(dst+1*16+4, bgra+1*16+4, 8)==0); /* dest y=1, src y=1 */
  assert(memcmp(dst+2*16+4, bgra+2*16+4, 8)==0);
  assert(dst[0] == 0xCC);
  h.frame_decoded = 0;
  assert(!BinkCopyToBuffer(&h.api,dst,16,4,0,0,BINKSURFACE32));
  assert(!BinkGetRects(&h.api,BINKSURFACE32));
  puts("video handoff and Bink1 ABI offsets verified");
  return 0;
}
