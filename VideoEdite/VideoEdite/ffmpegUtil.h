//
//  ffmpegUtil.h
//  VideoEdite
//
//  Created by ronglei on 14-5-8.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#ifndef VideoEdite_ffmpegUtil_h
#define VideoEdite_ffmpegUtil_h

//如果这是一段c++的代码，那么加入extern"C"{和}处理其中的代码
//为了能在c++中能够调用c的接口
#ifdef __cplusplus
extern "C" {
#endif
    
#   include "libavutil/time.h"
#	include "libavcodec/avcodec.h"
#	include "libavformat/avformat.h"
#	include "libswscale/swscale.h"
#	include "libswresample/swresample.h"
#	include "libavutil/opt.h"
#   include "libavresample/avresample.h"
	
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

    void ffmpeg_init();
    void ffmpeg_lock_init();
    void ffmpeg_lock();
    void ffmpeg_unlock();
    
    AVDictionary **setup_find_stream_info_opts(AVFormatContext *s, AVDictionary *codec_opts);
    AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                    AVFormatContext *s, AVStream *st, AVCodec *codec);
    int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec);
    
    int stream_component_open(AVFormatContext *ic, int stream_index, AVDictionary *codec_opts);
    void av_pakcet_calculate_pts(AVFormatContext *ic, AVStream *st, AVPacket* pkt);
        
#ifdef __cplusplus
}
#endif

#endif
