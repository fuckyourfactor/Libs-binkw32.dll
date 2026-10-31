/* SPDX-License-Identifier: MIT
 * COM surface introspection based only on published DirectDraw/Direct3D ABI.
 * These are experimental mappings, not validated against RAD's 2005 binary.
 */
#include "surface.h"
#include "bink32.h"
#include <stdint.h>
#include <stddef.h>
#define DDPF_ALPHAPIXELS 0x1u
#define DDPF_FOURCC 0x4u
#define DDPF_RGB 0x40u
#define FOURCC(a,b,c,d) ((uint32_t)(a)|((uint32_t)(b)<<8)|((uint32_t)(c)<<16)|((uint32_t)(d)<<24))
static int32_t map_d3d(uint32_t fmt){
 switch(fmt){
  case 20: return BINKSURFACE24; /* D3DFMT_R8G8B8 */
  case 21: return BINKSURFACE32A; /* D3DFMT_A8R8G8B8 */
  case 22: return BINKSURFACE32; /* D3DFMT_X8R8G8B8 */
  case 23: return BINKSURFACE565; /* D3DFMT_R5G6B5 */
  case 24: return BINKSURFACE555; /* D3DFMT_X1R5G5B5 */
  case 25: return BINKSURFACE5551; /* D3DFMT_A1R5G5B5 */
  case 26: return BINKSURFACE4444; /* D3DFMT_A4R4G4B4 */
  case FOURCC('Y','U','Y','2'):return BINKSURFACEYUY2;
  case FOURCC('U','Y','V','Y'):return BINKSURFACEUYVY;
  default:return -1;
 }
}
/* GetDesc at COM vtable index 12 for IDirect3DSurface8 and 9. The format
 * field is first; 64-byte output provides extra guard space for DX variants. */
int32_t bk_d3d_surface_type(void *surface){
 if(!surface)return -1;
 void **vt=*(void***)surface;if(!vt||!vt[12])return -1;
 typedef int32_t (BK_CALL *GetDesc)(void*,void*);
 uint32_t desc[16]={0};
 if(((GetDesc)vt[12])(surface,desc)<0)return -1;
 return map_d3d(desc[0]);
}
/* GetPixelFormat is COM slot 21 for classic DirectDraw surface interfaces. */
int32_t bk_dd_surface_type(void *surface){
 if(!surface)return -1;
 void **vt=*(void***)surface;if(!vt||!vt[21])return -1;
 typedef int32_t (BK_CALL *GetPixelFormat)(void*,void*);
 struct {uint32_t size,flags,fourcc,bits,r,g,b,a;} pf={32,0,0,0,0,0,0,0};
 if(((GetPixelFormat)vt[21])(surface,&pf)<0)return -1;
 if(pf.flags&DDPF_FOURCC){
  if(pf.fourcc==FOURCC('Y','U','Y','2'))return BINKSURFACEYUY2;
  if(pf.fourcc==FOURCC('U','Y','V','Y'))return BINKSURFACEUYVY;
  return -1;
 }
 if(!(pf.flags&DDPF_RGB))return -1;
 if(pf.bits==32 && pf.r==0x00ff0000u && pf.g==0x0000ff00u && pf.b==0x000000ffu)
  return pf.flags&DDPF_ALPHAPIXELS?BINKSURFACE32A:BINKSURFACE32;
 if(pf.bits==24 && pf.r==0x00ff0000u && pf.g==0x0000ff00u && pf.b==0x000000ffu)return BINKSURFACE24;
 if(pf.bits==16 && pf.r==0xf800u && pf.g==0x07e0u && pf.b==0x001fu)return BINKSURFACE565;
 if(pf.bits==16 && pf.r==0x7c00u && pf.g==0x03e0u && pf.b==0x001fu)
  return pf.flags&DDPF_ALPHAPIXELS?BINKSURFACE5551:BINKSURFACE555;
 return -1;
}
