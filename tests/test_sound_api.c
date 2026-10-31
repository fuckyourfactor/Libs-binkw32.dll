#include "bink32.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
int main(void){
 BINKSNDOPEN driver=BinkOpenWaveOut(0);
 assert(driver!=0);
 assert(BinkSetSoundSystem(BinkOpenWaveOut,0)==1);
 assert(BinkSetSoundSystem(0,0)==1);
 assert(BinkOpenDirectSound(0)==driver);
 assert(BinkOpenMiles(0)==driver);
 assert(BinkSetSoundSystem(BinkOpenDirectSound,0)==1);
 assert(BinkSetSoundSystem(BinkOpenMiles,0)==1);
 BINKSND snd={0};

#ifndef _WIN32
 assert(driver(&snd,44100,16,2,0,0)==0);
#endif
 puts("sound driver public interface tests passed");return 0;
}
