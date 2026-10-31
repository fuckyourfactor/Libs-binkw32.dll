#ifndef BK_PIXEL_H
#define BK_PIXEL_H
#include <stdint.h>
/* Convert a decoded, packed BGRA top-down frame into a caller-supplied Bink
 * surface rectangle. A negative pitch is allowed if dst points to logical row 0.
 * Returns 1 on success, 0 on unsupported format or invalid geometry. */
int bk_pixels_copy(const uint8_t *src, uint32_t width, uint32_t height,
                   void *dst, int32_t pitch, uint32_t dest_height,
                   uint32_t dx, uint32_t dy, uint32_t sx, uint32_t sy,
                   uint32_t sw, uint32_t sh, uint32_t flags);
#endif
