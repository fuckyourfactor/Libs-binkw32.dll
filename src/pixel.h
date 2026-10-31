#ifndef BK_PIXEL_H
#define BK_PIXEL_H
#include <stdint.h>

int bk_pixels_copy(const uint8_t *src, uint32_t width, uint32_t height,
                   void *dst, int32_t pitch, uint32_t dest_height,
                   uint32_t dx, uint32_t dy, uint32_t sx, uint32_t sy,
                   uint32_t sw, uint32_t sh, uint32_t flags);
#endif
