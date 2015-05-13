//
//  CAVReader.cpp
//  VideoEdite
//
//  Created by ronglei on 14-5-8.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#include "CAVReader.h"
#include "VideoEdite.h"

int MediaInfo_Get(const char *file_path, MediaInfo *info)
{
    int nr = -1;
    AVOpenParam param;
    BaseReader *reader = BaseReader::OpenReader(file_path, &param);
    if (reader) {
        info->duration = reader->GetDuration();
        if (reader->VideoIsValid()) {
            info->video.valid = 1;
            info->video.width = reader->GetWidth();
            info->video.height = reader->GetHeight();
            info->video.frame_rate = reader->GetFrameRate();
        }
        
        if (reader->AudioIsValid()) {
            info->audio.valid = 1;
            info->audio.channels = reader->GetChannels();
            info->audio.freq = reader->GetFreq();
        }
        
        reader->Close();
        
        nr = 0;
    }
    
    return nr;
}

int GetAudioBufferLength(int64_t duration)
{
    double sz = (2 * 44100 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16)) * duration / 1000000.0;
    
    return (int)sz;
}

int GetAudioBufferLengthEx(int64_t duration)
{
    int sz = GetAudioBufferLength(duration);
    if (sz%8) {
        sz += 8 - sz % 8;
    }
    
    return sz;
}

AVPacket avReadImage(const char *file_path, int width, int height)
{
    AVPacket pkt = {0, 0};
    
    AVOpenParam av_param;
    av_param.stage_time      = 0;
    av_param.has_video       = true;
    
    av_param.render_width    = width;
    av_param.render_height   = height;
    
    av_param.has_audio       = false;
    av_param.render_freq     = 0;
    av_param.render_channels = 0;
    
    BaseReader *reader = BaseReader::OpenReader(file_path, &av_param);
    if (reader) {
        reader->ReadPakcet(&pkt);
        if (pkt.stream_index == 0) {
            pkt.pts = reader->GetWidth();
            pkt.dts = reader->GetHeight();
        }else {
            av_free_packet(&pkt);
        }
        
        reader->Close();
    }
    
    return pkt;
}

BaseReader *BaseReader::OpenReader(const char *file_path, AVOpenParam *param)
{
    BaseReader *reader = CAVReader::CreateReader(file_path, param);
    
    return reader;
}


CAVReader::CAVReader():
m_ctx(NULL),
m_file_path(NULL),
m_ts_offset(0),
m_video_index(0),
m_video_stream(NULL),
m_video_frame(NULL),
m_video_width(0),
m_video_height(0),
m_video_frame_duration(0),
m_sws(NULL),
m_render_width(0),
m_render_height(0),
m_audio_index(0),
m_audio_stream(NULL),
m_audio_frame(NULL),
m_swr(NULL),
m_audio_finished(0),
m_video_finished(0),
m_audio_current_pts(0),
m_eof(0),
m_last_index_for_eof(0),
m_current_video_frame_duration(0)
{
    
}

CAVReader::~CAVReader()
{
    if (m_file_path) {
        free(m_file_path);
    }
}

CAVReader* CAVReader::CreateReader(const char *file_path, AVOpenParam *param)
{
    //"error:allocationg an object of abstract class"
    //错误原因:子类中有未实现的虚函数
    CAVReader *p_reader = new CAVReader();
    if (p_reader) {
        if (p_reader->OpenFile(file_path, param) >= 0) {
            return p_reader;
        }else{
            p_reader->Close();
            p_reader = NULL;
        }
    }
    
    return p_reader;
}

void CAVReader::Close()
{
    if (m_video_frame) {
        av_frame_free(&m_video_frame);
    }
    
    if (m_audio_frame) {
        av_frame_free(&m_audio_frame);
    }
    
    if (m_sws) {
        sws_freeContext(m_sws);
        m_sws = NULL;
    }
    
    if (m_swr) {
        swr_free(&m_swr);
    }
    
    if (m_video_stream) {
        avcodec_close(m_video_stream->codec);
    }
    
    if (m_audio_stream) {
        avcodec_close(m_audio_stream->codec);
    }
    
    avformat_close_input(&m_ctx);
    
    m_video_index = -1;
    m_video_stream = NULL;
    m_audio_index = -1;
    m_audio_stream = NULL;
    
    delete this;
}

int CAVReader::OpenFile(const char *file_path, AVOpenParam *param)
{
    int result = 0;
    ffmpeg_init();
    
    m_ctx = avformat_alloc_context();
    if (m_ctx) {
        int ret;
        AVDictionary *format_opts = NULL;
        
        //Do not block when reading packets from input.
        //从输入读取packets不设置回调,回调设为NULL
        m_ctx->flags |= AVFMT_FLAG_NONBLOCK;
        m_ctx->interrupt_callback.callback = NULL;
        
        //内部调用了malloc()为变量分配内存,需调用free()释放内存
        m_file_path = strdup(file_path);
        
        //打开输入媒体(如果需要的话)初始化所有与媒体读写有关的结构们,比如
        //AVIOContext，AVInputFormat等等,参见avformat_open_input.txt
        ret = avformat_open_input(&m_ctx, m_file_path, NULL, &format_opts);
        if (ret >= 0) {
            AVDictionaryEntry *entry = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX);
            if (!entry) {
                AVDictionary *codec_opts = NULL;
                AVDictionary **opts = setup_find_stream_info_opts(m_ctx, codec_opts);
                
                ret = avformat_find_stream_info(m_ctx, opts);
                if (ret >= 0) {
                    if (m_ctx->start_time != AV_NOPTS_VALUE) {
                        m_ts_offset = -m_ctx->start_time;
                    }
                    
                    for (int i = 0; i < m_ctx->nb_streams; i ++){
                        av_dict_free(&opts[i]);
                    }
//                    av_free(&opts);
                    
                    for (int i = 0; i < m_ctx->nb_streams; i ++) {
                        m_ctx->streams[i]->discard = AVDISCARD_ALL;
                    }
                    
                    m_video_index = av_find_best_stream(m_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
                    m_audio_index = av_find_best_stream(m_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
                    
                    if (m_video_index > 0 && param->has_video) {
                        if (open_video_stream(codec_opts) >= 0) {
                            set_pic_size(param->render_width, param->render_height);
                        }
                    }
                    
                    if (m_audio_index >= 0 && param->has_audio) {
                        open_audio_stream(codec_opts, param->render_freq, param->render_channels);
                    }
                    
                    result = 0;
                }
            }
        }else{
            printf("\n[CAVReader::openFile]:open file failed!\n");
        }
    }
    
    return result;
}

int CAVReader::open_video_stream(AVDictionary *codec_opts)
{
    int ret = -1;
    
    if (m_video_index >= 0) {
        if (stream_component_open(m_ctx, m_video_index, codec_opts) >= 0) {
            m_video_stream = m_ctx->streams[m_video_index];
            m_video_frame = av_frame_alloc();
            
            m_video_width = m_video_stream->codec->width;
            m_video_height = m_video_stream->codec->height;
            
            if (m_video_stream->avg_frame_rate.den) {
                m_video_frame_duration = AV_TIME_BASE * m_video_stream->avg_frame_rate.den / (double)m_video_stream->avg_frame_rate.num;
            }
            ret = 0;
        }
    }
    
    return ret;
}

void CAVReader::set_pic_size(int w, int h)
{
    if (w>0 && h>0) {
        m_render_width  = w;
        m_render_height = h;
    }
    else {
        m_render_width = m_video_width;
        m_render_height = m_video_height;
    }
    
    if (m_ctx) {
        m_sws = sws_getContext(m_video_width,
                               m_video_height,
                               m_video_stream->codec->pix_fmt,
                               m_render_width,
                               m_render_height,
                               PIX_FMT_RGBA,
                               SWS_BILINEAR,
                               NULL, NULL, NULL);
    }else{
        printf("\n[CAVReader::SetPictureSize]:file not be open!\n");
    }
}

int CAVReader::open_audio_stream(AVDictionary *codec_opts, int out_freq, int out_channels)
{
    int ret = -1;
    
    if (m_audio_index >= 0) {
        if (stream_component_open(m_ctx, m_audio_index, codec_opts) >= 0) {
            m_audio_stream = m_ctx->streams[m_audio_index];
            
            if (m_audio_stream) {
                m_audio_frame = av_frame_alloc();
                
                if (m_audio_frame) {
                    if (m_audio_stream->codec->channels != 2 ||
                        m_audio_stream->codec->sample_fmt != AV_SAMPLE_FMT_S16 ||
                        m_audio_stream->codec->sample_rate != 44100) {
                            m_swr = swr_alloc_set_opts(NULL,
                                                       av_get_default_channel_layout(2),
                                                       AV_SAMPLE_FMT_S16,
                                                       44100,
                                                       m_audio_stream->codec->channel_layout,
                                                       m_audio_stream->codec->sample_fmt,
                                                       m_audio_stream->codec->sample_rate,
                                                       0,
                                                       NULL);
                        if (swr_init(m_swr) > 0) {
                            ret = 0;
                        }
                    }else{
                        ret = 0;
                    }
                }
            }
        }
    }
    
    return ret;
}

int CAVReader::Seek(int64_t seekpos)
{
    int result = -1;
    
    if (m_ctx) {
        if (m_video_stream) {
            avcodec_flush_buffers(m_video_stream->codec);
        }
        
        if (m_audio_stream) {
            avcodec_flush_buffers(m_audio_stream->codec);
        }
        
        m_audio_current_pts = GetAudioBufferLengthEx(seekpos);
        
        result = avformat_seek_file(m_ctx, -1, INT64_MIN, seekpos, seekpos, 0);
        
        m_eof = 0;
        m_last_index_for_eof = 0;
        m_audio_finished = 0;
        m_audio_finished = 0;
    }
    
    return result;
}

int CAVReader::read_for_eof(AVPacket *pkt)
{
    AVPacket tmp_pkt = {0, };
    
    while (1) {
        if (m_last_index_for_eof==m_video_index) {
            if (m_audio_finished==0 && m_audio_index>=0) {
                m_last_index_for_eof = m_audio_index;
            }else if (m_video_finished) {
                return -2;
            }
            
            if (!m_video_finished) {
                if (decode_video(&tmp_pkt, pkt)>=0) {
                    pkt->stream_index = 0;
                    return 0;
                }else {
                    m_video_finished = 1;
                    if (m_last_index_for_eof==m_video_index) {
                        return -1;
                    }
                }
            }
        }else if (m_last_index_for_eof==m_audio_index) {
            if (m_video_finished==0 && m_video_index>=0) {
                m_last_index_for_eof = m_video_index;
            }else if (m_audio_finished) {
                return -1;
            }
            
            if (decode_audio(&tmp_pkt, pkt)>0) {
                pkt->stream_index = 1;
                return 0;
            }else {
                m_audio_finished = 1;
            }
        }
    }
    
    return -1;
}


int CAVReader::ReadPakcet(AVPacket *pkt)
{
    int result = -1;
    
    if (m_ctx == NULL) {
        return -1;
    }
    
    if (m_eof) {
        return read_for_eof(pkt);
    }
    
    while (result == -1) {
        AVPacket p_pkt;
        int ret = av_read_frame(m_ctx, &p_pkt);
        if (ret >= 0) {
            if (m_video_stream && p_pkt.stream_index == m_video_index) {
                if (p_pkt.dts != AV_NOPTS_VALUE) {
                    p_pkt.dts += av_rescale_q(m_ts_offset, AV_TIME_BASE_Q, m_video_stream->time_base);
                }
                if (p_pkt.pts != AV_NOPTS_VALUE) {
                    p_pkt.pts += av_rescale_q(m_ts_offset, AV_TIME_BASE_Q, m_video_stream->time_base);
                }
                if (m_video_stream && decode_video(&p_pkt, pkt) > 0) {
                    pkt->stream_index = 0;
                    result = 0;
                }
            }else if (m_audio_stream && p_pkt.stream_index == m_audio_index){
                if (p_pkt.dts != AV_NOPTS_VALUE) {
                    p_pkt.dts += av_rescale_q(m_ts_offset, AV_TIME_BASE_Q, m_audio_stream->time_base);
                }
                if (p_pkt.pts != AV_NOPTS_VALUE) {
                    p_pkt.pts += av_rescale_q(m_ts_offset, AV_TIME_BASE_Q, m_audio_stream->time_base);
                }
                if (m_audio_stream && decode_audio(&p_pkt, pkt) > 0) {
                    pkt->stream_index = 1;
                    result = 0;
                }
            }
            av_free_packet(&p_pkt);
        }else{
            m_eof = 1;
            if (m_video_index >= 0) {
                m_last_index_for_eof = m_video_index;
            }else if (m_audio_index >= 0){
                m_last_index_for_eof = m_audio_index;
            }
            
            return read_for_eof(pkt);
        }
    }
    
    return result;
}

int CAVReader::decode_video(AVPacket *src_pkt, AVPacket *dst_pkt)
{
    int got_picture = 0;
    
    if (src_pkt->duration) {
        m_current_video_frame_duration = av_rescale_q(src_pkt->duration,
                                                      m_video_stream->time_base,
                                                      AV_TIME_BASE_Q);
    }
    
    int len1 = avcodec_decode_video2(m_video_stream->codec, m_video_frame, &got_picture, src_pkt);
    if (got_picture && len1>=0) {
        m_video_frame->pkt_duration = 0;
        av_pakcet_calculate_pts(m_ctx, m_video_stream, src_pkt);
        av_new_packet(dst_pkt, m_render_width*m_render_height*4);
        
        int linesize[4]     = {m_render_width * 4, 0, 0, 0};
        uint8_t* data[4]    = {dst_pkt->data, NULL,};
        sws_scale(m_sws,
                  (const uint8_t** const)m_video_frame->data,
                  m_video_frame->linesize,
                  0,
                  m_video_stream->codec->height,
                  data,
                  linesize);
        
        dst_pkt->pts = src_pkt->pts;
        dst_pkt->pts = m_video_frame->pts;
        dst_pkt->pts = av_frame_get_best_effort_timestamp(m_video_frame);
        dst_pkt->pts = dst_pkt->pts * av_q2d(m_video_stream->time_base) * 1000000;
        
        if (m_current_video_frame_duration>0) {
            dst_pkt->duration = (int)m_current_video_frame_duration;
            m_current_video_frame_duration = 0;
        }
        else {
            dst_pkt->duration = src_pkt->duration;
        }
        
        if (src_pkt->size>len1) {
            len1 = src_pkt->size;
        }
        
        //        av_frame_unref(m_video_frame);
        
        return 0;
    }
    
    if (src_pkt->size>len1) {
        len1 = src_pkt->size;
    }
    
    return -1;
}

int CAVReader::decode_audio(AVPacket *src_pkt, AVPacket *dst_pkt)
{
    AVPacket temp_pkt = *src_pkt;
    //    av_pakcet_calculate_pts(m_ic, m_audio_st, src_pkt);
    int64_t pts = src_pkt->pts;
    
    int audio_buffer_size = 0;
    
    uint8_t *out_buffer = m_audio_buf;
    int out_buffer_szie = sizeof(m_audio_buf);
    
    while (temp_pkt.size>=0) {
        int got_frame = 0;
        int len1 = avcodec_decode_audio4(m_audio_stream->codec, m_audio_frame, &got_frame, &temp_pkt);
        
        if (len1>0) {
            temp_pkt.data += len1;
            temp_pkt.size -= len1;
            
            if (got_frame) {
                if (m_swr) {
                    int out_count = out_buffer_szie / 2 / av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
                    int len2 = swr_convert(m_swr,
                                           &out_buffer,
                                           out_count,
                                           (const uint8_t**)m_audio_frame->extended_data,
                                           m_audio_frame->nb_samples);
                    
                    if (len2>0) {
                        int duration = (int)av_frame_get_pkt_duration(m_audio_frame);
                        
                        if (duration==0) {
                            duration = len2*2*4 / (sizeof(float) * 2 * 44100);
                        }
                        
                        const int bufSize = av_samples_get_buffer_size(NULL,
                                                                       2,
                                                                       len2,
                                                                       AV_SAMPLE_FMT_S16,
                                                                       1);
                        
                        int out_size        = len2 * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
                        out_size            = bufSize;
                        out_buffer          += out_size;
                        out_buffer_szie     -= out_size;
                        audio_buffer_size   += out_size;
                    }
                }
                else {
                    
                }
            }
        }
        else {
            break;
        }
    }
    
    if (audio_buffer_size>0) {
        av_new_packet(dst_pkt, audio_buffer_size);
        memcpy(dst_pkt->data, m_audio_buf, audio_buffer_size);
        dst_pkt->pts = pts;
        dst_pkt->pts = m_audio_current_pts;
        dst_pkt->duration = audio_buffer_size;
        m_audio_current_pts += audio_buffer_size;
    }
    
    return audio_buffer_size;

}

int64_t CAVReader::GetDuration()
{
    return m_ctx->duration;
}

float CAVReader::GetFrameRate()
{
    if (m_video_stream) {
        return av_q2d(m_video_stream->r_frame_rate);
    }
    return 0.0;
}

int CAVReader::GetFreq()
{
    if (m_audio_stream) {
        return m_audio_stream->codec->sample_rate;
    }
    return 0;
}

int CAVReader::GetChannels()
{
    if (m_audio_stream) {
        return m_audio_stream->codec->channels;
    }
    
    return 0;
}
