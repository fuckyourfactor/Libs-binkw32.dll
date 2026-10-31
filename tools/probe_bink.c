/* SPDX-License-Identifier: MIT
 * Command-line integration harness for legally obtained Bink1 files.
 * Runs with the native Linux test library or the Win32 DLL, provided the
 * build was made with BINK_WITH_FFMPEG=ON for actual decoding.
 * Usage: bink_probe filename.bik [decoded-audio.pcm]
 */
#include "bink32.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
 if(argc<2){fprintf(stderr,"Usage: bink_probe FILE.bik [audio.pcm]\n");return 2;}
 HBINK movie=BinkOpen(argv[1],0);
 if(!movie){fprintf(stderr,"BinkOpen: %s\n",BinkGetError());return 1;}
 if(movie->Width>8192 || movie->Height>8192 || movie->Width>SIZE_MAX/movie->Height/4){
  fprintf(stderr,"Invalid dimensions\n");BinkClose(movie);return 1;
 }
 size_t fb_bytes=(size_t)movie->Width*movie->Height*4;
 uint8_t *frame=(uint8_t*)malloc(fb_bytes);
 U32 max=movie->NumTracks>0?BinkGetTrackMaxSize(movie,0):0;
 uint8_t *pcm=max?(uint8_t*)malloc(max):NULL;
 HBINKTRACK track=(pcm&&argc>2)?BinkOpenTrack(movie,0):NULL;
 FILE *audio=track?fopen(argv[2],"wb"):NULL;
 if(!frame||(max&&argc>2&&(!pcm||!track||!audio))){
  fprintf(stderr,"Cannot allocate probe buffers\n");goto failed;
 }
 uint32_t digest=2166136261u, frames=0;
 U32 count=movie->Frames;
 printf("Bink %ux%u, %u frames, %u/%u fps, %d sound tracks\n",
        movie->Width,movie->Height,movie->Frames,movie->FrameRate,
        movie->FrameRateDiv,movie->NumTracks);
 for(U32 i=0;i<count;i++){
  if(!BinkDoFrame(movie)){
   fprintf(stderr,"Frame %u failed: %s\n",i+1,BinkGetError());goto failed;
  }
  if(!BinkCopyToBuffer(movie,frame,(S32)(movie->Width*4),movie->Height,0,0,BINKSURFACE32)){
   fprintf(stderr,"Copy of frame %u failed: %s\n",i+1,BinkGetError());goto failed;
  }
  for(size_t p=0;p<fb_bytes;p++)digest=(digest^frame[p])*16777619u;
  if(track){
   U32 bytes=BinkGetTrackData(track,pcm);
   if(bytes && fwrite(pcm,1,bytes,audio)!=bytes){fprintf(stderr,"PCM file write failed\n");goto failed;}
  }
  frames++;
  if(i+1<count)BinkNextFrame(movie);
 }
 printf("Decoded frames: %u, FNV1a(BGRA) = %08x\n",frames,digest);
 if(audio)fclose(audio);
 if(track)BinkCloseTrack(track);
 BinkClose(movie);free(frame);free(pcm);return 0;
 failed:
 if(audio)fclose(audio);
 if(track)BinkCloseTrack(track);
 BinkClose(movie);free(frame);free(pcm);return 1;
}
