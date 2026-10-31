#ifndef BK_SURFACE_H
#define BK_SURFACE_H
#include <stdint.h>
/* Public Direct3D/DirectDraw COM pixel-format introspection, limited to known
 * uncompressed formats. Requires a valid COM interface pointer from the caller.
 * Returns the BINKSURFACE enum or -1 when unrecognized / API method failed. */
int32_t bk_d3d_surface_type(void *surface);
int32_t bk_dd_surface_type(void *surface);
#endif
