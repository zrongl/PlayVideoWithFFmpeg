//
//  CAVReader.h
//  VideoEdite
//
//  Created by ronglei on 14-5-8.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#ifndef __VideoEdite__CAVReader__
#define __VideoEdite__CAVReader__

#include <iostream>
#include "ffmpegUtil.h"
#include "BaseReader.h"

class CAVReader : public BaseReader
{
    CAVReader();
    ~CAVReader();
public:
    static CAVReader* CreateReader(const char *file_path, AVOpenParam *param);
    
    int OpenFile(const char *file_path, AVOpenParam *param);
    void Close();
    
    int Seek(int64_t seekpos);
    int ReadPakcet(AVPacket *pkt);
    
    int64_t GetDuration();
    int GetWidth() { return m_video_width; }
    int GetHeight() { return m_video_height; }
    float GetFrameRate();
    int GetFreq();
    int GetChannels();
    
    int AudioIsValid() { return m_audio_stream ? 1 : 0; }
    int VideoIsValid() { return m_video_stream ? 1 : 0; }
    
    int Demuxing(const char*src_file, const char*dst_video, const char*dst_audio);
protected:
    int open_video_stream(AVDictionary *codec_opts);
    void set_pic_size(int w, int h);
    
    int open_audio_stream(AVDictionary *codec_opts, int out_freq, int out_channels);
    
    int read_for_eof(AVPacket *pkt);
    int decode_video(AVPacket *src_pkt, AVPacket *dst_pkt);
    int decode_audio(AVPacket *src_pkt, AVPacket *dst_pkt);
    
protected:
    
    AVFormatContext             *m_ctx;
    char                        *m_file_path;
    
    int                         m_video_index;
    AVStream                    *m_video_stream;
    AVFrame                     *m_video_frame;
    int                         m_video_width;
    int                         m_video_height;
    int64_t                     m_video_frame_duration;
    struct SwsContext           *m_sws;
    int                         m_render_width;
    int                         m_render_height;
    
    int                         m_audio_index;
    AVStream                    *m_audio_stream;
    AVFrame                     *m_audio_frame;
    struct SwrContext           *m_swr;
    
    static const int    AVCODEC_MAX_AUDIO_FRAME_SIZE = 192000;
    DECLARE_ALIGNED(16, uint8_t, m_audio_buf)[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];

    int                         m_video_finished;       //  标志视频已经读取完了
    int                         m_audio_finished;
    
    int                         m_eof;
    int                         m_last_index_for_eof;
    int64_t                     m_ts_offset;
    
    int                         m_audio_current_pts;    //  bytes
    int64_t                     m_current_video_frame_duration;
};


#endif /* defined(__VideoEdite__CAVReader__) */


