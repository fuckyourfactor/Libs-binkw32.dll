#include "platform.h"
#define WIN32API __declspec(dllimport)
extern WIN32API void * __stdcall CreateFileA(const char*,unsigned int,unsigned int,void*,unsigned int,unsigned int,void*);
extern WIN32API unsigned int __stdcall GetFileSize(void*,unsigned int*);
extern WIN32API int __stdcall ReadFile(void*,void*,unsigned int,unsigned int*,void*);
extern WIN32API int __stdcall CloseHandle(void*);
extern WIN32API unsigned int __stdcall GetTickCount(void);

typedef struct BkMemoryRegion {
 void *base, *allocation_base;
 unsigned int allocation_protect;
 size_t region_size;
 unsigned int state, protect, type;
} BkMemoryRegion;
extern WIN32API size_t __stdcall VirtualQuery(const void*, BkMemoryRegion*,size_t);
#define BK_MEM_COMMIT 0x1000u
#define BK_PAGE_GUARD 0x100u
#define BK_PAGE_NOACCESS 0x01u
#define BK_FILE_LIMIT (256u*1024u*1024u)

static int readable_region(const uint8_t *ptr,size_t len){
 uintptr_t cur=(uintptr_t)ptr;
 if(!ptr||len==0||len>BK_FILE_LIMIT||cur+len<cur)return 0;
 uintptr_t end=cur+len;
 while(cur<end){
  BkMemoryRegion r;
  if(VirtualQuery((void*)cur,&r,sizeof(r))!=sizeof(r))return 0;
  if(r.state!=BK_MEM_COMMIT || (r.protect&(BK_PAGE_GUARD|BK_PAGE_NOACCESS)))return 0;
  uintptr_t region_end=(uintptr_t)r.base+r.region_size;
  if(region_end<=cur)return 0;
  cur=region_end<end?region_end:end;
 }
 return 1;
}
unsigned char *bk_load_memory(const void *source,size_t *size){
 const uint8_t *p=(const uint8_t*)source;
 if(!size||!readable_region(p,44))return NULL;
 if(p[0]!='B'||p[1]!='I'||p[2]!='K')return NULL;
 uint32_t declared=(uint32_t)p[4]|((uint32_t)p[5]<<8)|((uint32_t)p[6]<<16)|((uint32_t)p[7]<<24);
 size_t len=(size_t)declared+8u;
 if(len<44||len>BK_FILE_LIMIT||!readable_region(p,len))return NULL;
 uint8_t *copy=(uint8_t*)bk_alloc(len);
 if(!copy)return NULL;
 bk_memcpy(copy,p,len);*size=len;return copy;
}
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
