/* SPDX-License-Identifier: MIT
 * Original, portable Bink CPU surface conversion: no proprietary decoder code.
 */
#include "pixel.h"
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
static uint8_t clamp8(int x) {return (uint8_t)(x<0?0:x>255?255:x);}
static uint8_t y_luma(uint8_t r,uint8_t g,uint8_t b){return clamp8(((66*r+129*g+25*b+128)>>8)+16);}
static uint8_t y_chroma_u(uint8_t r,uint8_t g,uint8_t b){return clamp8(((-38*r-74*g+112*b+128)>>8)+128);}
static uint8_t y_chroma_v(uint8_t r,uint8_t g,uint8_t b){return clamp8(((112*r-94*g-18*b+128)>>8)+128);}
int bk_pixels_copy(const uint8_t *src,uint32_t w,uint32_t h,void *dst,int32_t pitch,
 uint32_t dh,uint32_t dx,uint32_t dy,uint32_t sx,uint32_t sy,uint32_t sw,uint32_t sh,uint32_t flags){
 uint32_t fmt=flags&15u, bpp;
 if(!src||!dst||!w||!h||!sw||!sh || sx>=w||sy>=h||sw>w-sx||sh>h-sy ||dy>dh||sh>dh-dy)return 0;
 switch(fmt){
  case 1:case 2:bpp=3;break;
  case 3:case 4:case 5:case 6:bpp=4;break;
  case 7:case 8:case 9:case 10:case 11:case 12:case 13:case 14:bpp=2;break;
  default:return 0;
 }
 if((fmt==13||fmt==14)&&((sx|dx|sw)&1u))return 0;
 if((uint64_t)(dx+ (uint64_t)sw)*bpp>INT32_MAX || pitch==INT32_MIN || pitch==0 ||
    (uint64_t)(dx+ (uint64_t)sw)*bpp>(uint32_t)(pitch<0?-pitch:pitch))return 0;
 for(uint32_t y=0;y<sh;y++){
  const uint8_t *in=src+(((size_t)sy+y)*w+sx)*4;
  uint8_t *out=(uint8_t*)dst+(ptrdiff_t)(dy+y)*(ptrdiff_t)pitch+(size_t)dx*bpp;
  if(fmt==13||fmt==14){
   for(uint32_t x=0;x<sw;x+=2){
    const uint8_t *a=in+4*x,*b=in+4*(x+1);
    uint8_t y0=y_luma(a[2],a[1],a[0]),y1=y_luma(b[2],b[1],b[0]);
    uint8_t u=y_chroma_u((a[2]+(int)b[2])/2,(a[1]+(int)b[1])/2,(a[0]+(int)b[0])/2);
    uint8_t v=y_chroma_v((a[2]+(int)b[2])/2,(a[1]+(int)b[1])/2,(a[0]+(int)b[0])/2);
    if(fmt==13){out[2*x]=y0;out[2*x+1]=u;out[2*x+2]=y1;out[2*x+3]=v;}
    else {out[2*x]=u;out[2*x+1]=y0;out[2*x+2]=v;out[2*x+3]=y1;}
   }
   continue;
  }
  for(uint32_t x=0;x<sw;x++){
   uint8_t b=in[x*4],g=in[x*4+1],r=in[x*4+2],a=in[x*4+3];
   if(bpp>=3){
    if(fmt==2||fmt==4||fmt==6){out[x*bpp]=r;out[x*bpp+1]=g;out[x*bpp+2]=b;}
    else {out[x*bpp]=b;out[x*bpp+1]=g;out[x*bpp+2]=r;}
    if(bpp==4)out[x*4+3]=(fmt==5||fmt==6)?a:255;
   } else {
    uint16_t v;
    switch(fmt){
     case 7:v=(uint16_t)(((a>>4)<<12)|((r>>4)<<8)|((g>>4)<<4)|(b>>4));break; /* ARGB4444 */
     case 8:v=(uint16_t)(((r>>3)<<11)|((g>>3)<<6)|((b>>3)<<1)|(a>>7));break; /* RGB5551 */
     case 9:v=(uint16_t)(((r>>3)<<10)|((g>>3)<<5)|(b>>3));break; /* RGB555 */
     case 10:v=(uint16_t)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));break; /* RGB565 */
     case 11:v=(uint16_t)(((r>>2)<<10)|((g>>3)<<5)|(b>>3));break; /* RGB655 */
     default:v=(uint16_t)(((r>>2)<<10)|((g>>2)<<4)|(b>>4));break; /* RGB664 */
    }
    out[x*2]=(uint8_t)v;out[x*2+1]=(uint8_t)(v>>8);
   }
  }
 }
 return 1;
}
