/* No proprietary SDK used. Imports the system's documented Kernel32 APIs. */
#include "platform.h"
#define WIN32API __declspec(dllimport)
extern WIN32API void * __stdcall CreateFileA(const char*,unsigned int,unsigned int,void*,unsigned int,unsigned int,void*);
extern WIN32API unsigned int __stdcall GetFileSize(void*,unsigned int*);
extern WIN32API int __stdcall ReadFile(void*,void*,unsigned int,unsigned int*,void*);
extern WIN32API int __stdcall CloseHandle(void*);
extern WIN32API unsigned int __stdcall GetTickCount(void);
extern WIN32API void __stdcall Sleep(unsigned int);
__declspec(dllimport) void* __cdecl malloc(size_t);
__declspec(dllimport) void __cdecl free(void*);
__declspec(dllimport) void* __cdecl memcpy(void*,const void*,size_t);
__declspec(dllimport) void* __cdecl memset(void*,int,size_t);
void *bk_alloc(size_t n){return malloc(n?n:1);}
void bk_free(void *p){free(p);}
void *bk_memcpy(void*d,const void*s,size_t n){return memcpy(d,s,n);}
void *bk_memset(void*d,int v,size_t n){return memset(d,v,n);}
unsigned char *bk_load_file(const char *path,size_t *size){
 void *f=CreateFileA(path,0x80000000u,1|2,NULL,3,0x80u,NULL);
 if(f==(void*)-1)return NULL;
 unsigned int high=0,n=GetFileSize(f,&high),got=0;unsigned char *p=NULL;
 if(high || n==0xffffffffu || n>256u*1024u*1024u)goto done;
 p=(unsigned char*)bk_alloc(n);if(!p)goto done;
 if(!ReadFile(f,p,n,&got,NULL)||got!=n){bk_free(p);p=NULL;goto done;}
 *size=n;
 done: CloseHandle(f);return p;
}
uint32_t bk_millis(void){return GetTickCount();}
void bk_sleep(uint32_t ms){Sleep(ms);}
int __stdcall DllMain(void *m,unsigned int reason,void *reserved){(void)m;(void)reason;(void)reserved;return 1;}
int _fltused = 0;
