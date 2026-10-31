#include "private.h"
#include "platform.h"
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#define BK_PCM_LIMIT (8u*1024u*1024u)
typedef struct BkFFAudio {
 AVCodecContext *ctx;
 AVFrame *decoded;
 AVPacket *packet;
 SwrContext *swr;
 uint8_t *pcm;
 size_t pcm_cap;
 uint32_t pcm_size;
 uint32_t next_frame;
 uint32_t cached_frame;
 int configured;
} BkFFAudio;
typedef struct BkMemoryIO {
 const uint8_t *data;
 int64_t length;
 int64_t position;
} BkMemoryIO;

static int bink_read(void *opaque,uint8_t *dst,int count){
 BkMemoryIO *io=(BkMemoryIO*)opaque;
 if(count<=0)return AVERROR(EINVAL);
 if(io->position>=io->length)return AVERROR_EOF;
 int64_t remain=io->length-io->position;
 if((int64_t)count>remain)count=(int)remain;
 bk_memcpy(dst,io->data+io->position,(size_t)count);
 io->position+=count;
 return count;
}
static int64_t bink_seek(void *opaque,int64_t offset,int whence){
 BkMemoryIO *io=(BkMemoryIO*)opaque;
 if(whence==AVSEEK_SIZE)return io->length;
 whence &= ~AVSEEK_FORCE;
 int64_t base;
 if(whence==SEEK_SET)base=0;
 else if(whence==SEEK_CUR)base=io->position;
 else if(whence==SEEK_END)base=io->length;
 else return AVERROR(EINVAL);

 if(offset< -base || offset>io->length-base)return AVERROR(EINVAL);
 io->position=base+offset;
 return io->position;
}
typedef struct BkFFmpeg {
 BkMemoryIO source;
 AVIOContext *io;
 AVFormatContext *fmt;
 AVCodecContext *codec;
 AVFrame *frame;
 AVPacket *pkt;
 struct SwsContext *sws;
 int stream;
 int64_t next;
 BkFFAudio **audio;
} BkFFmpeg;
static void audio_close(BkFFAudio *a){
 if(!a)return;
 avcodec_free_context(&a->ctx);
 av_frame_free(&a->decoded);
 av_packet_free(&a->packet);
 swr_free(&a->swr);
 av_free(a->pcm);
 bk_free(a);
}
void bk_decoder_close(BkHandle *h){
 BkFFmpeg *d=(BkFFmpeg*)h->decoder;
 if(!d)return;
 if(d->audio){for(uint32_t i=0;i<h->index.tracks;i++)audio_close(d->audio[i]);bk_free(d->audio);}
 sws_freeContext(d->sws);
 av_packet_free(&d->pkt);av_frame_free(&d->frame);
 avcodec_free_context(&d->codec);avformat_close_input(&d->fmt);
 avio_context_free(&d->io);
 bk_free(d);h->decoder=NULL;
}
int bk_decoder_open(BkHandle *h,const char *path){
 (void)path;
 BkFFmpeg *d=(BkFFmpeg*)bk_alloc(sizeof(*d));const AVCodec *codec;
 if(!d)return 0;bk_memset(d,0,sizeof(*d));h->decoder=d;
 if(!h->file||h->index.file_size<44||h->index.file_size>INT64_MAX)goto failed;
 d->source.data=h->file;
 d->source.length=(int64_t)h->index.file_size;
 uint8_t *iobuf=(uint8_t*)av_malloc(32768);
 if(!iobuf)goto failed;
 d->io=avio_alloc_context(iobuf,32768,0,&d->source,bink_read,NULL,bink_seek);
 if(!d->io){av_free(iobuf);goto failed;}
 d->fmt=avformat_alloc_context();if(!d->fmt)goto failed;
 d->fmt->pb=d->io;
 d->fmt->flags|=AVFMT_FLAG_CUSTOM_IO;
 if(avformat_open_input(&d->fmt,NULL,NULL,NULL)<0 || avformat_find_stream_info(d->fmt,NULL)<0)goto failed;
 d->stream=av_find_best_stream(d->fmt,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
 if(d->stream<0)goto failed;
 AVStream *stream=d->fmt->streams[d->stream];
 codec=avcodec_find_decoder(stream->codecpar->codec_id);
 if(!codec)goto failed;
 d->codec=avcodec_alloc_context3(codec);if(!d->codec)goto failed;
 if(avcodec_parameters_to_context(d->codec,stream->codecpar)<0 || avcodec_open2(d->codec,codec,NULL)<0)goto failed;
 if(d->codec->width!=(int)h->api.Width || d->codec->height!=(int)h->api.Height)goto failed;
 d->frame=av_frame_alloc();d->pkt=av_packet_alloc();
 if(!d->frame||!d->pkt)goto failed;
 if(h->index.tracks){
  d->audio=(BkFFAudio**)bk_alloc((size_t)h->index.tracks*sizeof(*d->audio));
  if(!d->audio)goto failed;
  bk_memset(d->audio,0,(size_t)h->index.tracks*sizeof(*d->audio));
 }
 d->next=0;return 1;
 failed:bk_decoder_close(h);return 0;
}

int bk_decoder_frame(BkHandle *h,uint32_t target){
 BkFFmpeg *d=(BkFFmpeg*)h->decoder;
 if(!d||!h->rgb||target>=h->api.Frames)return 0;
 if((int64_t)target<d->next-1){

  bk_decoder_close(h);
  if(!bk_decoder_open(h,NULL))return 0;
  d=(BkFFmpeg*)h->decoder;
 }
 if((int64_t)target==d->next-1){
  if(!d->frame->data[0])return 0;
 } else {
  while(d->next<=(int64_t)target){
   int got=0;
   while(av_read_frame(d->fmt,d->pkt)>=0){
    if(d->pkt->stream_index==d->stream){
     int send=avcodec_send_packet(d->codec,d->pkt);
     if(send>=0){
      int ret=avcodec_receive_frame(d->codec,d->frame);
      if(ret==0)got=1;
     }
    }
    av_packet_unref(d->pkt);
    if(got)break;
   }
   if(!got)return 0;
   d->next++;
  }
 }
 d->sws=sws_getCachedContext(d->sws,d->frame->width,d->frame->height,
    (enum AVPixelFormat)d->frame->format,d->frame->width,d->frame->height,
    AV_PIX_FMT_BGRA,SWS_BILINEAR,NULL,NULL,NULL);
 if(!d->sws || d->frame->width!=(int)h->api.Width || d->frame->height!=(int)h->api.Height)return 0;
 uint8_t *planes[4]={h->rgb,NULL,NULL,NULL};
 int linesize[4]={(int)(h->api.Width*4),0,0,0};
 return sws_scale(d->sws,(const uint8_t * const*)d->frame->data,d->frame->linesize,
                   0,d->codec->height,planes,linesize)==d->codec->height;
}
static BkFFAudio *audio_open(BkHandle *h,uint32_t track){
 BkFFmpeg *d=(BkFFmpeg*)h->decoder;
 if(!d || !d->audio || track>=h->index.tracks)return NULL;
 if(d->audio[track])return d->audio[track];
 BkFFAudio *a=(BkFFAudio*)bk_alloc(sizeof(*a));
 if(!a)return NULL;
 bk_memset(a,0,sizeof(*a));a->cached_frame=UINT32_MAX;
 d->audio[track]=a;
 uint32_t kind=h->index.track_types[track];
 const AVCodec *codec=avcodec_find_decoder((kind&0x1000u)?AV_CODEC_ID_BINKAUDIO_DCT:AV_CODEC_ID_BINKAUDIO_RDFT);
 if(!codec)goto failed;
 a->ctx=avcodec_alloc_context3(codec);if(!a->ctx)goto failed;
 a->ctx->sample_rate=(int)h->index.track_rates[track];
 if(a->ctx->sample_rate<8000||a->ctx->sample_rate>192000)goto failed;
 av_channel_layout_default(&a->ctx->ch_layout,(kind&0x2000u)?2:1);
 a->ctx->extradata=(uint8_t*)av_mallocz(4+AV_INPUT_BUFFER_PADDING_SIZE);
 if(!a->ctx->extradata)goto failed;
 bk_memcpy(a->ctx->extradata,h->index.data,4);a->ctx->extradata_size=4;
 if(avcodec_open2(a->ctx,codec,NULL)<0)goto failed;
 a->packet=av_packet_alloc();a->decoded=av_frame_alloc();
 if(!a->packet||!a->decoded)goto failed;
 a->configured=1;return a;
 failed:d->audio[track]=NULL;audio_close(a);return NULL;
}
static int convert_audio_frame(BkFFAudio *a){
 AVFrame *f=a->decoded;
 if(!a->swr){
  AVChannelLayout output;
  av_channel_layout_default(&output,a->ctx->ch_layout.nb_channels);
  int ret=swr_alloc_set_opts2(&a->swr,&output,AV_SAMPLE_FMT_S16,a->ctx->sample_rate,
                              &f->ch_layout,(enum AVSampleFormat)f->format,f->sample_rate,0,NULL);
  av_channel_layout_uninit(&output);
  if(ret<0||!a->swr||swr_init(a->swr)<0)return 0;
 }
 int max_samples=swr_get_out_samples(a->swr,f->nb_samples);
 int channels=a->ctx->ch_layout.nb_channels;
 if(max_samples<=0||max_samples>INT_MAX/4||channels<=0||channels>2)return 0;
 size_t bytes=(size_t)max_samples*(size_t)channels*2u;
 if(bytes>BK_PCM_LIMIT || bytes>BK_PCM_LIMIT-a->pcm_size)return 0;
 size_t required=(size_t)a->pcm_size+bytes;
 if(a->pcm_cap<required){
  uint8_t *buf=(uint8_t*)av_realloc(a->pcm,required);
  if(!buf)return 0;
  a->pcm=buf;a->pcm_cap=required;
 }
 uint8_t *out[1]={a->pcm+a->pcm_size};
 int n=swr_convert(a->swr,out,max_samples,(const uint8_t**)f->extended_data,f->nb_samples);
 if(n<0)return 0;
 a->pcm_size+=(uint32_t)n*(uint32_t)channels*2u;return 1;
}

int bk_audio_get(BkHandle *h,uint32_t track,uint32_t frame,
                 const uint8_t **out,uint32_t *size){
 if(!out||!size||frame>=h->index.frames)return 0;
 *out=NULL;*size=0;
 BkFFAudio *a=audio_open(h,track);if(!a||!a->configured)return 0;
 if(a->cached_frame==frame){*out=a->pcm;*size=a->pcm_size;return 1;}
 if(frame<a->next_frame){
  avcodec_flush_buffers(a->ctx);
  if(a->swr){swr_close(a->swr);if(swr_init(a->swr)<0)return 0;}
  a->next_frame=0;a->cached_frame=UINT32_MAX;
 }
 for(uint32_t i=a->next_frame;i<=frame;i++){
  const uint8_t *packet=NULL;uint32_t bytes=0;
  if(!bk_packet(&h->index,i,track,&packet,&bytes))return 0;
  a->pcm_size=0;
  if(bytes){
   if(bytes>INT_MAX||av_new_packet(a->packet,(int)bytes)<0)return 0;
   bk_memcpy(a->packet->data,packet,bytes);
   int ret=avcodec_send_packet(a->ctx,a->packet);
   av_packet_unref(a->packet);
   if(ret<0)return 0;
   while((ret=avcodec_receive_frame(a->ctx,a->decoded))>=0){
    if(i==frame && !convert_audio_frame(a)){
     av_frame_unref(a->decoded);return 0;
    }
    av_frame_unref(a->decoded);
   }
   if(ret!=AVERROR(EAGAIN)&&ret!=AVERROR_EOF)return 0;
  }
 }
 a->next_frame=frame+1;
 a->cached_frame=frame;
 *out=a->pcm;*size=a->pcm_size;
 return 1;
}
