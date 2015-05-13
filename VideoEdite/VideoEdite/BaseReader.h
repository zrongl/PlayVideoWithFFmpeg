//
//  BaseReader.h
//  VideoEdite
//
//  Created by ronglei on 14-5-8.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#ifndef VideoEdite_BaseReader_h
#define VideoEdite_BaseReader_h



struct AVOpenParam
{
    int64_t stage_time;
    bool    has_video;
    int     render_width;
    int     render_height;
    
    bool    has_audio;
    int     render_freq;
    int     render_channels;
    
    AVOpenParam()
    {
        stage_time      = 0;
        has_video       = true;
        render_width    = -1;
        render_height   = -1;
        
        has_audio       = true;
        render_freq     = -1;
        render_channels = -1;
    }
};

// c++中的struct能够包含成员函数,并具备继承和多态特性
/**
 关于使用大括号初始化
 　　class和struct如果定义了构造函数的话，都不能用大括号进行初始化
 　　如果没有定义构造函数，struct可以用大括号初始化。
 　　如果没有定义构造函数，且所有成员变量全是public的话，可以用大括号初始化。
 关于默认访问权限
 　　class中默认的成员访问权限是private的，而struct中则是public的。
 关于继承方式
 　　class继承默认是private继承，而struct继承默认是public继承。
 */
struct BaseReader
{
    static BaseReader* OpenReader(const char *file_path, AVOpenParam *parm);
    virtual int OpenFile(const char *file_path, AVOpenParam *parm) = 0;
    virtual void Close() = 0;
    virtual int Seek(int64_t seekpos) = 0;
    
    virtual int ReadPakcet(AVPacket *pkt) = 0;
    
    virtual int64_t GetDuration() = 0;
    virtual int GetWidth() = 0;
    virtual int GetHeight() = 0;
    virtual float GetFrameRate() = 0;
    virtual int GetFreq() = 0;
    virtual int GetChannels() = 0;

    virtual int VideoIsValid() = 0;
    virtual int AudioIsValid() = 0;
    
};

AVPacket avReadImage(const char *file_path, int width=-1, int height=-1);

#endif
