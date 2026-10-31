#include "trace.h"
#include <stdarg.h>
#ifdef _WIN32
#include <stddef.h>
__declspec(dllimport) unsigned int __stdcall GetEnvironmentVariableA(const char*,char*,unsigned int);
__declspec(dllimport) void* __stdcall CreateFileA(const char*,unsigned int,unsigned int,void*,unsigned int,unsigned int,void*);
__declspec(dllimport) int __stdcall WriteFile(void*,const void*,unsigned int,unsigned int*,void*);
__declspec(dllimport) int __stdcall CloseHandle(void*);
__declspec(dllimport) int __cdecl _vsnprintf(char*,size_t,const char*,va_list);
void bk_trace(const char *fmt,...){
 char path[512],line[512];
 unsigned int r=GetEnvironmentVariableA("BINK_OPEN_TRACE",path,sizeof(path));
 if(r==0||r>=sizeof(path)||!fmt)return;
 void *file=CreateFileA(path,0x00000004u,0x1u|0x2u,0,4,0x80u,0);
 if(file==(void*)-1)return;
 va_list args;va_start(args,fmt);
 int len=_vsnprintf(line,sizeof(line)-2,fmt,args);
 va_end(args);
 if(len<0||len>(int)sizeof(line)-2)len=(int)sizeof(line)-2;
 line[len++]='\r';line[len++]='\n';
 unsigned int written=0;
 WriteFile(file,line,(unsigned int)len,&written,0);
 CloseHandle(file);
}
#else
#include <stdio.h>
#include <stdlib.h>
void bk_trace(const char *fmt,...){
 const char *path=getenv("BINK_OPEN_TRACE");
 if(!path||!*path||!fmt)return;
 FILE *f=fopen(path,"a");
 if(!f)return;
 va_list ap;va_start(ap,fmt);vfprintf(f,fmt,ap);va_end(ap);
 fputc('\n',f);fclose(f);
}
#endif
