#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
void *bk_alloc(size_t n){return malloc(n?n:1);}
void bk_free(void *p){free(p);}
void *bk_memcpy(void*d,const void*s,size_t n){return memcpy(d,s,n);}
void *bk_memset(void*d,int v,size_t n){return memset(d,v,n);}
unsigned char *bk_load_file(const char *path,size_t *size){
 FILE *f=fopen(path,"rb"); long n; unsigned char *p;
 if(!f)return NULL;
 if(fseek(f,0,SEEK_END)|| (n=ftell(f))<0 || n>256*1024*1024L || fseek(f,0,SEEK_SET)) {fclose(f);return NULL;}
 p=(unsigned char*)bk_alloc((size_t)n);if(!p){fclose(f);return NULL;}
 if(fread(p,1,(size_t)n,f)!=(size_t)n){bk_free(p);fclose(f);return NULL;}
 fclose(f);*size=(size_t)n;return p;
}
uint32_t bk_millis(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint32_t)(t.tv_sec*1000u+t.tv_nsec/1000000u);}
void bk_sleep(uint32_t ms){struct timespec t={(time_t)(ms/1000), (long)((ms%1000)*1000000L)};nanosleep(&t,NULL);}
