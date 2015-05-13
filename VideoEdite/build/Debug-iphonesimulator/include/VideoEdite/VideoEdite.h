//
//  VideoEdite.h
//  VideoEdite
//
//  Created by ronglei on 14-5-8.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

typedef struct MediaInfo
{
    int64_t     duration;
    struct VideoInfo
    {
        int     valid;          //标识是否有视频
        int     width;
        int     height;
        float   frame_rate;
    }video;
    
    struct AudioInfo
    {
        int valid;              //标识是否有音频
        int channels;
        int freq;
    }audio;
}MediaInfo;

#ifdef __cplusplus
extern "C" {
#endif

    int FFMPEG_INIT();
    int MediaInfo_Get(const char *file_path, MediaInfo *info);
    
#ifdef __cplusplus
}
#endif