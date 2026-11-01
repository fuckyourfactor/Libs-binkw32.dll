#ifndef BK_CONTAINER_H
#define BK_CONTAINER_H
#include <stddef.h>
#include <stdint.h>
typedef struct BkIndex {
 uint32_t frames,tracks,width,height,fps_num,fps_den,flags,largest_frame,internal_frames,marker;
 uint32_t *offset; uint8_t *key;
 uint32_t *track_ids,*track_types,*track_sizes,*track_rates;
 const uint8_t *data; size_t file_size;
} BkIndex;

int bk_parse(const uint8_t *buf,size_t len,BkIndex *out);
void bk_index_free(BkIndex *idx);
int bk_video_packet(const BkIndex *idx,uint32_t frame,const uint8_t **data,uint32_t *bytes);
int bk_packet(const BkIndex *idx,uint32_t frame,uint32_t track,const uint8_t **p,uint32_t *n);
#endif
