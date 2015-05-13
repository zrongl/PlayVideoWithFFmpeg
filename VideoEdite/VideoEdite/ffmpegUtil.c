//
//  ffmpegUtil.c
//  VideoEdite
//
//  Created by ronglei on 14-5-8.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#include <stdio.h>
#include "ffmpegUtil.h"

#define POSIX_THREAD    1
#define _WIN32_         0

#include <pthread.h>
#include "libavutil/avstring.h"

#ifdef POSIX_THREAD
    static pthread_mutex_t      ffmpeg_mtx;
#elif defined (_WIN32_)
    static HANDLE               ffmpeg_mtx;
#endif

int FFMPEG_INIT()
{
    ffmpeg_init();
    return 0;
}

//保证av_register_all()只被调用一次
static int init_status = 0;
void ffmpeg_init()
{
    if (0 == init_status) {
        av_register_all();
        
        init_status = 1;
    }
    
    ffmpeg_lock_init();
}

void ffmpeg_lock_init()
{
#ifdef POSIX_THREAD
    pthread_mutex_init(&ffmpeg_mtx, NULL);
#elif defined (_WIN32_)
	WaitForSingleObject(FFMPEG_mtx, INFINITE);
#endif
}

void ffmpeg_lock()
{
#ifdef POSIX_THREAD
    pthread_mutex_lock(&ffmpeg_mtx);
#elif defined (_WIN32_)
    
#endif
}

void ffmpeg_unlock()
{
#ifdef POSIX_THREAD
    pthread_mutex_unlock(&ffmpeg_mtx);
#elif defined (_WIN32_)
	ReleaseMutex(FFMPEG_mtx);
#endif
}


AVDictionary **setup_find_stream_info_opts(AVFormatContext *s, AVDictionary *codec_opts)
{
    int i;
    AVDictionary **opts;
    
    if (!s->nb_streams)
        return NULL;
    opts = av_mallocz(s->nb_streams * sizeof(*opts));
    if (!opts) {
        av_log(NULL, AV_LOG_ERROR,
               "Could not alloc memory for stream options.\n");
        return NULL;
    }
    for (i = 0; i < s->nb_streams; i++)
        opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codec->codec_id, s, s->streams[i], NULL);
    return opts;
}

AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                AVFormatContext *s, AVStream *st, AVCodec *codec)
{
    AVDictionary    *ret = NULL;
    AVDictionaryEntry *t = NULL;
    //oformat为空需要对iformat进行解码;
    //oformat非空需要对oformat进行编码
    int            flags = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM
    : AV_OPT_FLAG_DECODING_PARAM;
    char          prefix = 0;
    const AVClass    *cc = avcodec_get_class();
    
    if (!codec)
        codec            = s->oformat ? avcodec_find_encoder(codec_id)
        : avcodec_find_decoder(codec_id);
    
    switch (st->codec->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            prefix  = 'v';
            flags  |= AV_OPT_FLAG_VIDEO_PARAM;
            break;
        case AVMEDIA_TYPE_AUDIO:
            prefix  = 'a';
            flags  |= AV_OPT_FLAG_AUDIO_PARAM;
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            prefix  = 's';
            flags  |= AV_OPT_FLAG_SUBTITLE_PARAM;
            break;
            
        default:
            break;
    }
    
    while ( (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) ) {
        char *p = strchr(t->key, ':');
        
        /* check stream specification in opt name */
        if (p)
            switch (check_stream_specifier(s, st, p + 1)) {
                case  1: *p = 0; break;
                case  0:         continue;
                default:         return NULL;
            }
        
        if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) ||
            (codec && codec->priv_class &&
             av_opt_find(&codec->priv_class, t->key, NULL, flags,
                         AV_OPT_SEARCH_FAKE_OBJ)))
            av_dict_set(&ret, t->key, t->value, 0);
        else if (t->key[0] == prefix &&
                 av_opt_find(&cc, t->key + 1, NULL, flags,
                             AV_OPT_SEARCH_FAKE_OBJ))
            av_dict_set(&ret, t->key + 1, t->value, 0);
        
        if (p)
            *p = ':';
    }
    return ret;
}

int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec)
{
    int ret = avformat_match_stream_specifier(s, st, spec);
    if (ret < 0)
        av_log(s, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
    return ret;
}

int stream_component_open(AVFormatContext *ic, int stream_index, AVDictionary *codec_opts)
{
    AVCodecContext      *avctx;
    AVCodec             *codec;
    const char          *forced_codec_name = NULL;
    AVDictionary        *opts;
    AVDictionaryEntry   *t = NULL;
    int                 stream_lowres = 0;
    
    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;
    
    //查找解码器
    avctx = ic->streams[stream_index]->codec;
    codec = avcodec_find_decoder(avctx->codec_id);
    
    if (forced_codec_name)
        codec = avcodec_find_decoder_by_name(forced_codec_name);
    
    if (!codec) {
        if (forced_codec_name)
            av_log(NULL, AV_LOG_WARNING, "No codec could be found with name '%s'\n", forced_codec_name);
        else
            av_log(NULL, AV_LOG_WARNING, "No codec could be found with id %d\n", avctx->codec_id);
        return -1;
    }
    
    avctx->codec_id = codec->id;
    //解码过程遇到错误的标示
    avctx->workaround_bugs   = 1;
    
    //max_lowres解码器支持的图像分辨率的最大值
    //无法直接访问,只能通过av_codec_set_lowres()函数去设置
    if(stream_lowres > av_codec_get_lowres(codec)){
        av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n", av_codec_get_lowres(codec));
        stream_lowres = av_codec_get_lowres(codec);
    }
    av_codec_set_lowres(avctx, stream_lowres);
    avctx->error_concealment = 3;
    
    if(stream_lowres)
        avctx->flags |= CODEC_FLAG_EMU_EDGE;
#if 0
    if (fast)
        avctx->flags2 |= CODEC_FLAG2_FAST;
#endif
    
    if(codec->capabilities & CODEC_CAP_DR1)
        avctx->flags |= CODEC_FLAG_EMU_EDGE;
    
    opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
    if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    
    if (stream_lowres)
        av_dict_set(&opts, "lowres", av_asprintf("%d", stream_lowres), AV_DICT_DONT_STRDUP_VAL);
    
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);
    
    if (avcodec_open2(avctx, codec, &opts) < 0)
        return -1;
    
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        return AVERROR_OPTION_NOT_FOUND;
    }
    
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            break;
            
        case AVMEDIA_TYPE_VIDEO:
            break;
            
        case AVMEDIA_TYPE_SUBTITLE:
            break;
            
        default:
            break;
    }
    return 0;
}

void av_pakcet_calculate_pts(AVFormatContext *ic, AVStream *st, AVPacket* pkt)
{
	int64_t pkt_pts = 0;
	int64_t pkt_dts = 0;
#if 0
	if (pkt->dts != AV_NOPTS_VALUE)
		pkt->dts -= av_rescale_q(ic->start_time, avtime_base, st->time_base);
#endif
    
	if (pkt->dts != AV_NOPTS_VALUE)
		pkt_dts = av_rescale_q(pkt->dts, st->time_base, AV_TIME_BASE_Q);
    
#if 0
	if (pkt->pts != AV_NOPTS_VALUE)
		pkt->pts -= av_rescale_q(ic->start_time, avtime_base, st->time_base);
#endif
    
	if (pkt->pts != AV_NOPTS_VALUE)
		pkt_pts = av_rescale_q(pkt->pts, st->time_base, AV_TIME_BASE_Q);
    
#if 0
	if (pkt->duration!=0)
		pkt->duration -= av_rescale_q(ic->start_time, avtime_base, st->time_base);
#endif
    
	if (pkt->duration!=0)
		pkt->duration = (int)av_rescale_q(pkt->duration, st->time_base, AV_TIME_BASE_Q);
    
#if 0
	if (pkt->duration==AV_NOPTS_VALUE)
	{
		pkt->duration = m_frameDuration;
	}
#endif
    
	pkt->pts  = pkt_pts;
	pkt->dts  = pkt_dts;
    
	pkt->pts = pkt_dts;
}
