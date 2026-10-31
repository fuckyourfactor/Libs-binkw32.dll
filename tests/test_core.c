#include "bink32.h"
#include "container.h"
#include <stdio.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>
static void put32(unsigned char *p,unsigned x){p[0]=(unsigned char)x;p[1]=(unsigned char)(x>>8);p[2]=(unsigned char)(x>>16);p[3]=(unsigned char)(x>>24);}
int main(void){
 unsigned char movie[64]={0};movie[0]='B';movie[1]='I';movie[2]='K';movie[3]='i';
 put32(movie+4,56);put32(movie+8,2);put32(movie+12,8);put32(movie+16,2);
 put32(movie+20,2);put32(movie+24,2);put32(movie+28,30);put32(movie+32,1);
 put32(movie+44,0);put32(movie+48,56|1);put32(movie+52,60|1);
 BkIndex i={0};assert(bk_parse(movie,sizeof(movie),&i));
 assert(i.frames==2&&i.offset[0]==56&&i.offset[1]==60);
 assert(i.key[0]==1&&i.key[1]==1);
 bk_index_free(&i);assert(!bk_parse(movie,32,&i));
 movie[0]='Z';assert(!bk_parse(movie,sizeof(movie),&i));movie[0]='B';
 FILE *f=fopen("synthetic_test.bik","wb");assert(f);
 assert(fwrite(movie,1,sizeof(movie),f)==sizeof(movie));fclose(f);
 HBINK b=BinkOpen("synthetic_test.bik",0);assert(b);
 assert(b->Width==2&&b->Height==2&&b->Frames==2&&b->FrameNum==1);
 assert(BinkGetKeyFrame(b,2,0)==2);
 BinkNextFrame(b);assert(b->FrameNum==2);
 BinkGoto(b,1,0);assert(b->FrameNum==1);
 assert(BinkDoFrame(b)==0); /* invalid test video must NEVER be accepted */
 assert(strstr(BinkGetError(),"decoding")!=NULL);
 BINKSUMMARY summary;BinkGetSummary(b,&summary);
 assert(summary.TotalFrames==2&&summary.TotalTime==66);
 BinkPause(b,1);assert(BinkWait(b)==1);BinkPause(b,0);
 BinkClose(b);remove("synthetic_test.bik");
 HBINKBUFFER bb=BinkBufferOpen(NULL,2,2,0);assert(bb&&bb->BufferPitch==8);
 assert(BinkBufferLock(bb));assert(BinkBufferClear(bb,0x112233));
 unsigned char *px=(unsigned char*)bb->Buffer;assert(px[0]==0x33&&px[1]==0x22&&px[2]==0x11);
 assert(BinkBufferSetScale(bb,4,4));assert(BinkBufferSetOffset(bb,10,11));
 BinkBufferUnlock(bb);BinkBufferClose(bb);

 /* Independently constructed track metadata and per-frame packet boundaries. */
 unsigned char audio_movie[92]={0};
 audio_movie[0]='B';audio_movie[1]='I';audio_movie[2]='K';audio_movie[3]='i';
 put32(audio_movie+4,84);put32(audio_movie+8,2);put32(audio_movie+12,16);
 put32(audio_movie+16,2);put32(audio_movie+20,16);put32(audio_movie+24,8);
 put32(audio_movie+28,24);put32(audio_movie+32,1);put32(audio_movie+44,1);
 put32(audio_movie+48,32); /* declared largest decoded audio block */
 audio_movie[52]=0x22;audio_movie[53]=0x56; /* 22050 Hz */
 audio_movie[54]=0;audio_movie[55]=0x20; /* stereo */
 put32(audio_movie+56,1234); /* public track id */
 put32(audio_movie+60,68|1);put32(audio_movie+64,84); /* second frame not key */
 put32(audio_movie+68,8);put32(audio_movie+72,123);put32(audio_movie+84,0);
 assert(bk_parse(audio_movie,sizeof(audio_movie),&i));
 assert(i.tracks==1&&i.track_ids[0]==1234&&i.track_sizes[0]==32);
 assert(i.track_rates[0]==22050&&i.track_types[0]==0x2000);
 assert(i.key[0]==1&&i.key[1]==0);
 const unsigned char *packet=NULL;uint32_t packet_bytes=0;
 assert(bk_packet(&i,0,0,&packet,&packet_bytes));
 assert(packet_bytes==8&&packet==audio_movie+72);
 assert(bk_packet(&i,1,0,&packet,&packet_bytes)&&packet_bytes==0);
 assert(!bk_packet(&i,3,0,&packet,&packet_bytes));
 FILE *af=fopen("synthetic_audio.bik","wb");assert(af);
 assert(fwrite(audio_movie,1,sizeof(audio_movie),af)==sizeof(audio_movie));fclose(af);
 HBINK ba=BinkOpen("synthetic_audio.bik",0);assert(ba);
 assert(BinkGetTrackID(ba,0)==1234);assert(BinkGetTrackMaxSize(ba,0)==32);
 assert(BinkGetTrackType(ba,0)==0x2000);
 HBINKTRACK bt=BinkOpenTrack(ba,0);assert(bt);
 assert(bt->Frequency==22050&&bt->Channels==2&&bt->Bits==16);
 assert(BinkGetTrackData(bt,NULL)==0); /* audio decode unsupported */
 BinkCloseTrack(bt);BinkClose(ba);remove("synthetic_audio.bik");
 put32(audio_movie+68,800000);assert(!bk_packet(&i,0,0,&packet,&packet_bytes));
 bk_index_free(&i);
 BinkSetError("test-error");assert(strcmp(BinkGetError(),"test-error")==0);
 assert(RADTimerRead()==0);
 puts("track and audio packet indexing tests passed");
 puts("core tests passed");return 0;
}
