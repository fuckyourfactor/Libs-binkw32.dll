#ifndef BK_PLATFORM_H
#define BK_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
void *bk_alloc(size_t n);
void bk_free(void *p);
void *bk_memcpy(void *d,const void *s,size_t n);
void *bk_memset(void *d,int v,size_t n);
/* Returns a newly allocated file buffer, or NULL. Cap: 256 MiB. */
unsigned char *bk_load_file(const char *path, size_t *size);
uint32_t bk_millis(void);
void bk_sleep(uint32_t ms);
#endif
