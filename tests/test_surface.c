#include "surface.h"
#include "bink32.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef struct {void **vt;} Mock;
static uint32_t d3dfmt=23;
static int32_t BK_CALL mock_desc(void *self,void *arg){(void)self;((uint32_t*)arg)[0]=d3dfmt;return 0;}
static int32_t BK_CALL mock_pixel(void *self,void *arg){
 (void)self;uint32_t*p=(uint32_t*)arg;
 assert(p[0]==32);p[1]=0x40;p[3]=16;p[4]=0xf800;p[5]=0x07e0;p[6]=0x001f;return 0;
}
int main(void){
 void *dv[13]={0};Mock d3d={dv};dv[12]=(void*)mock_desc;
 assert(bk_d3d_surface_type(NULL)==-1);
 assert(bk_d3d_surface_type(&d3d)==BINKSURFACE565);
 d3dfmt=21;assert(bk_d3d_surface_type(&d3d)==BINKSURFACE32A);
 d3dfmt=0xffffffff;assert(bk_d3d_surface_type(&d3d)==-1);
 void *ddv[22]={0};Mock dd={ddv};ddv[21]=(void*)mock_pixel;
 assert(bk_dd_surface_type(NULL)==-1);
 assert(bk_dd_surface_type(&dd)==BINKSURFACE565);
 puts("COM pixel format tests passed");return 0;
}
