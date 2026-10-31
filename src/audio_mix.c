/* SPDX-License-Identifier: MIT. Original fixed-point 16-bit PCM volume/pan. */
#include "audio.h"
static int clamp(int v,int min,int max){return v<min?min:v>max?max:v;}
void bk_audio_mix(int16_t *out,const int16_t *in,uint32_t frames,uint32_t channels,int volume,int pan){
 if(!in||!out||!(channels==1||channels==2))return;
 int gain=clamp(volume,0,32768),p=clamp(pan,0,65536);
 int left=(p>32768)?(65536-p):32768;
 int right=(p<32768)?p:32768;
 for(uint32_t i=0;i<frames;i++){
  if(channels==1){out[i]=(int16_t)((int32_t)in[i]*gain/32768);}
  else {
   out[2*i]=(int16_t)(((int32_t)in[2*i]*gain/32768)*left/32768);
   out[2*i+1]=(int16_t)(((int32_t)in[2*i+1]*gain/32768)*right/32768);
  }
 }
}
