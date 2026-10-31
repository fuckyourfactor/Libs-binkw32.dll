/* MIT licensed independently implemented Bink1 container index reader. */
#include "container.h"
#include "platform.h"
#define BK_MAX_FRAMES 1000000u
#define BK_MAX_TRACKS 256u
static uint16_t rd16(const uint8_t *p){return (uint16_t)(p[0]|((unsigned)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int get32(const uint8_t *p,size_t len,size_t off,uint32_t *v){if(off>len||len-off<4)return 0;*v=rd32(p+off);return 1;}
void bk_index_free(BkIndex *x){
 if(!x)return;bk_free(x->offset);bk_free(x->key);bk_free(x->track_ids);bk_free(x->track_types);bk_free(x->track_sizes);bk_free(x->track_rates);bk_memset(x,0,sizeof(*x));
}
int bk_parse(const uint8_t *p,size_t len,BkIndex *out){
 uint32_t filesize,n,fpsd,fps,marker,track_count,frame_count,largest,internal,width,height,flags;
 size_t off;uint32_t i,prev=0;
 if(!p||!out||len<48)return 0;
 bk_memset(out,0,sizeof(*out));
 if(p[0]!='B'||p[1]!='I'||p[2]!='K')return 0;
 /* The 2005 SDK targets Bink 1. Bink 2 is explicitly unsupported. */
 if(p[3]!='b'&&p[3]!='f'&&p[3]!='g'&&p[3]!='h'&&p[3]!='i'&&p[3]!='k')return 0;
 marker=rd32(p);filesize=rd32(p+4);frame_count=rd32(p+8);largest=rd32(p+12);
 internal=rd32(p+16);width=rd32(p+20);height=rd32(p+24);
 fps=rd32(p+28);fpsd=rd32(p+32);flags=rd32(p+36);track_count=rd32(p+44);
 if(filesize>UINT32_MAX-8u||filesize+8u>len||!frame_count||frame_count>BK_MAX_FRAMES ||
    !width||width>7680||!height||height>4800||!fps||!fpsd||largest>len||track_count>BK_MAX_TRACKS)return 0;
 out->frames=frame_count;out->tracks=track_count;out->width=width;out->height=height;
 out->fps_num=fps;out->fps_den=fpsd;out->marker=marker;out->largest_frame=largest;
 out->internal_frames=internal;out->flags=flags;out->data=p;out->file_size=filesize+8u;
 off=48;
 if(p[3]=='k')off+=4; /* newer Bink1 variants contain an extra field */
 if(off>len || track_count>(len-off)/12)return 0;
 out->offset=(uint32_t*)bk_alloc((size_t)(frame_count+1)*sizeof(uint32_t));
 out->key=(uint8_t*)bk_alloc(frame_count);
 out->track_ids=(uint32_t*)bk_alloc((track_count?track_count:1)*sizeof(uint32_t));
 out->track_types=(uint32_t*)bk_alloc((track_count?track_count:1)*sizeof(uint32_t));
 out->track_sizes=(uint32_t*)bk_alloc((track_count?track_count:1)*sizeof(uint32_t));
 out->track_rates=(uint32_t*)bk_alloc((track_count?track_count:1)*sizeof(uint32_t));
 if(!out->offset||!out->key||!out->track_ids||!out->track_types||!out->track_sizes||!out->track_rates)goto bad;
 for(i=0;i<track_count;i++){out->track_sizes[i]=rd32(p+off);off+=4;}
 for(i=0;i<track_count;i++){out->track_rates[i]=(uint32_t)rd16(p+off);out->track_types[i]=(uint32_t)rd16(p+off+2);off+=4;}
 for(i=0;i<track_count;i++){out->track_ids[i]=rd32(p+off);off+=4;}
 if(frame_count>(len-off)/4)goto bad;
 for(i=0;i<frame_count;i++){
  if(!get32(p,len,off,&n))goto bad;off+=4;
  out->offset[i]=n&~1u;out->key[i]=(uint8_t)(i==0 || (n&1u));
  if(out->offset[i]<off||out->offset[i]>=out->file_size || (i && out->offset[i]<=prev))goto bad;
  prev=out->offset[i];
 }
 out->offset[frame_count]=(uint32_t)out->file_size;
 if(out->offset[frame_count]<=prev)goto bad;
 return 1;
 bad:bk_index_free(out);return 0;
}
int bk_packet(const BkIndex *x,uint32_t frame,uint32_t track,const uint8_t **p,uint32_t *n){
 uint32_t start,end,pos,i,bytes;
 if(!x||frame>=x->frames||track>=x->tracks||!p||!n)return 0;
 start=x->offset[frame];end=x->offset[frame+1];
 if(end>x->file_size||start>=end)return 0;
 pos=start;
 for(i=0;i<x->tracks;i++){
  if(pos>end||end-pos<4)return 0;
  bytes=rd32(x->data+pos);pos+=4;
  if(bytes>end-pos)return 0;
  if(i==track){*p=x->data+pos;*n=bytes;return 1;}
  pos+=bytes;
 }
 return 0;
}
