#include "bink32.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 BINKSNDOPEN driver=BinkOpenWaveOut(0);
 assert(driver!=0);
 assert(BinkSetSoundSystem(BinkOpenWaveOut,0)==1);
 assert(BinkSetSoundSystem(0,0)==1);
 assert(BinkSetSoundSystem(BinkOpenDirectSound,0)==0);
 BINKSND snd={0};
 /* On non-Windows targets WaveOut cannot initialize; gracefully reject it. */
#ifndef _WIN32
 assert(driver(&snd,44100,16,2,0,0)==0);
#endif
 puts("sound driver public interface tests passed");return 0;
}
