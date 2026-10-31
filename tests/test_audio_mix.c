#include "audio.h"
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
int main(void){
 const int16_t stereo[]={10000,-10000,20000,-20000};int16_t out[4];
 bk_audio_mix(out,stereo,2,2,32768,32768);for(int i=0;i<4;i++)assert(out[i]==stereo[i]);
 bk_audio_mix(out,stereo,2,2,32768,0);assert(out[0]==10000&&out[1]==0);
 bk_audio_mix(out,stereo,2,2,32768,65536);assert(out[0]==0&&out[1]==-10000);
 bk_audio_mix(out,stereo,2,2,16384,32768);assert(out[0]==5000&&out[1]==-5000);
 bk_audio_mix(out,stereo,2,2,0,32768);assert(out[0]==0&&out[1]==0);
 bk_audio_mix(out,stereo,2,2,99999,32768);assert(out[0]==stereo[0]);
 bk_audio_mix(out,stereo,2,1,32768,32768);assert(out[0]==10000&&out[1]==-10000);
 puts("audio PCM mixer tests passed");return 0;
}
