//
//  ffmpeg_interface.h
//  VideoEditeDemo
//
//  Created by ronglei on 14-9-24.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#ifndef __VideoEditeDemo__ffmpeg_interface__
#define __VideoEditeDemo__ffmpeg_interface__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>

#endif /* defined(__VideoEditeDemo__ffmpeg_interface__) */

int metadata(const char* input_file);
int resample_audio(const char *output_audio);
int scaling_video(const char *output_video, const char *output_size);

#ifdef __cplusplus
}
#endif