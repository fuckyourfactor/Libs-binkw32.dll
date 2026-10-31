#include "pixel.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
int main(void){
 const uint8_t bgra[16]={0,0,255,255, 0,255,0,128, 255,0,0,64, 255,255,255,255};
 uint8_t out[64];
 memset(out,0xaa,sizeof(out));
 assert(bk_pixels_copy(bgra,2,2,out,8,2,0,0,0,0,2,2,3));
 assert(out[0]==0 && out[1]==0 && out[2]==255 && out[3]==255);
 assert(out[4]==0 && out[5]==255 && out[6]==0);
 assert(bk_pixels_copy(bgra,2,2,out,8,2,0,0,0,0,2,2,4));
 assert(out[0]==255&&out[2]==0);
 assert(bk_pixels_copy(bgra,2,2,out,8,2,0,0,0,0,2,2,5));
 assert(out[7]==128);
 assert(bk_pixels_copy(bgra,2,2,out,4,2,0,0,0,0,2,2,10));
 assert(out[0]==0&&out[1]==0xf8&&out[2]==0xe0&&out[3]==0x07);
 assert(bk_pixels_copy(bgra,2,2,out,4,2,0,0,0,0,2,2,9));
 assert(out[0]==0&&out[1]==0x7c);
 assert(bk_pixels_copy(bgra,2,2,out+4,-4,2,0,0,0,0,2,2,10));
 assert(out[4]==0&&out[5]==0xf8);
 assert(bk_pixels_copy(bgra,2,2,out,4,2,0,0,0,0,2,2,13));
 assert(out[0] != 0 && out[1] != 0);
 assert(!bk_pixels_copy(bgra,2,2,out,3,2,0,0,0,0,2,2,10));
 assert(!bk_pixels_copy(bgra,2,2,out,4,2,0,0,1,0,2,1,10));
 assert(!bk_pixels_copy(bgra,2,2,out,4,2,0,0,0,0,2,2,15));
 assert(!bk_pixels_copy(bgra,2,2,out,4,2,1,0,0,0,2,1,13));
 puts("pixel conversion tests passed");return 0;
}
