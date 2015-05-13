//
//  ViewController.m
//  VideoEditeDemo
//
//  Created by ronglei on 14-5-8.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#import "ViewController.h"
#include "VideoEdite.h"
#include "ffmpeg_interface.h"

static int init_status = 0;
bool is_stopping = true;

AVFormatContext *pFormatCtx;
AVCodecContext  *pCodecCtx;
AVCodec         *pCodec;
AVPacket        packet;
AVFrame         *pFrame;
AVPicture       picture;
SwsContext      *img_convert_ctx;
int video_st_index=-1;
int nWidth;
int nHeight;


@interface ViewController (){

}

@property (weak, nonatomic) IBOutlet UIImageView *imageView;
@property (strong, nonatomic) NSTimer *timer;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self initPlayer];
	// Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)onMediaInfoBtn:(id)sender
{
    NSString *path = [[NSBundle mainBundle] pathForResource:@"begin_again" ofType:@"mp4"];
    const char *file_path = [path cStringUsingEncoding:NSUTF8StringEncoding];
//    MediaInfo info = {0, };
//    MediaInfo_Get(file_path, &info);
    
    metadata(file_path);
}

- (void)initPlayer
{
    NSString *path = [[NSBundle mainBundle] pathForResource:@"begin_again" ofType:@"mp4"];
    if (0 == init_status) {
        [self initFFmpegWith:path];
        init_status = 1;
    }
    
    CADisplayLink* displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(packetDecodeAndShow)];
    displayLink.frameInterval = 3;
    [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

- (IBAction)onPlayButton:(id)sender
{
    is_stopping = NO;
}

- (IBAction)onPauseButton:(id)sender
{
    is_stopping = YES;
}

- (IBAction)onOutputVideo:(id)sender
{
    const char *output_file = "/Users/issuser/Downloads/video.rgb24";
    scaling_video(output_file, "320x320");
}

- (IBAction)onOutputAudio:(id)sender
{
    const char *output_audio = "/Users/issuser/Downloads/audio.s16";
    resample_audio(output_audio);
}

- (void)initFFmpegWith:(NSString *)fLocalFileName
{
    av_register_all();
	
	if( avformat_open_input(&pFormatCtx, [fLocalFileName cStringUsingEncoding:NSUTF8StringEncoding], NULL, NULL) != 0){
		NSLog(@"av  open  input  file  failed!\n");
		return ;
	}
	//从文件中提取流信息
	if( avformat_find_stream_info(pFormatCtx, NULL) < 0 ){
		NSLog(@"av  find  input  file  failed!\n");
		return ;
	}
	
	//获取视频流索引
    if((video_st_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, NULL, NULL, &pCodec, 0) )  < 0 ){
        NSLog(@"Cannot find a video stream in the input file\n");
        return ;
    }
    //查找对应的解码器
	pCodecCtx = pFormatCtx->streams[video_st_index]->codec ;
	pCodec = avcodec_find_decoder( pCodecCtx->codec_id );
    if (!pCodec) {
        NSLog(@"Cannot find a decoder\n");
    }
    av_init_packet(&packet);
    
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec,NULL) < 0)
        goto initError; // Could not open codec
    
    pFrame = av_frame_alloc();
    NSLog(@"init success");
    return;
    
initError:
    //error action
    NSLog(@"init failed");
    [self releaseFFMPEG];
    return ;
}

- (void)releaseFFMPEG
{
    // Free scaler
    sws_freeContext(img_convert_ctx);
    
    // Free RGB picture
    avpicture_free(&picture);
    
    // Free the YUV frame
    av_free(pFrame);
    
    // Close the codec
    if (pCodecCtx) avcodec_close(pCodecCtx);
}

- (void)dealloc
{
    // Free scaler
    sws_freeContext(img_convert_ctx);
    
    // Free RGB picture
    avpicture_free(&picture);
    
    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);
    
    // Free the YUV frame
    av_free(pFrame);
    
    // Close the codec
    if (pCodecCtx) avcodec_close(pCodecCtx);
}

- (void)setupScaler
{
    // Release old picture and scaler
    avpicture_free(&picture);
    sws_freeContext(img_convert_ctx);
    
    // Allocate RGB picture
    if (avpicture_alloc(&picture, PIX_FMT_RGB24,pCodecCtx->width,pCodecCtx->height) < 0){
        NSLog(@"avpicture_alloc error\n");
        return;
    }
    
    // Setup scaler
    static int sws_flags =  SWS_FAST_BILINEAR;
    img_convert_ctx = sws_getContext(pCodecCtx->width,
                                     pCodecCtx->height,
                                     pCodecCtx->pix_fmt,
                                     pCodecCtx->width,
                                     pCodecCtx->height,
                                     PIX_FMT_RGB24,
                                     sws_flags, NULL, NULL, NULL);
    if (img_convert_ctx == NULL) {
        NSLog(@"sws_getContext error\n");
        return;
    }
}

- (void)convertFrameToRGB
{
    [self setupScaler];
    sws_scale (img_convert_ctx,pFrame->data, pFrame->linesize,
               0, pCodecCtx->height,
               picture.data, picture.linesize);
}

- (void)packetDecodeAndShow
{
//    AVStream *v_st;
//    if (video_st_index >= 0) {
//        v_st = pFormatCtx->streams[video_st_index];
//        if (av_seek_frame(pFormatCtx, video_st_index, 2*AV_TIME_BASE, AVSEEK_FLAG_ANY) < 0) {
//            NSLog(@"av_seek_frame error\n");
//            return;
//        }
//    }
    
    //av_read_frame()填充的是packet,对packet利用avcodec_decode_video2()解码后得到frame
    //一个frame可能包含为多个packet,
    //例如一个frame包含5个packet的时候,avcodec_decode_video2()对前4个packet进行解码的时候got_frame都会被填充0值,
    //标示未解码出一个完整的frame,当avcodec_decode_video2()对第5个packet进行解码时可以获取解码出一个完整的frame,
    //此时got_frame会被填充1,标示已经解码出一个完整的frame.
    
    while (!is_stopping && av_read_frame(pFormatCtx, &packet) >= 0) {
        if (packet.stream_index == video_st_index) {
            int got_frame = 0;
            if (avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, &packet) >= 0) {
                if (got_frame) {
                    [self convertFrameToRGB];
                    nWidth = pCodecCtx->width;
                    nHeight = pCodecCtx->height;
                    CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
                    CFDataRef data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, picture.data[0], nWidth*nHeight*3,kCFAllocatorNull);
                    CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
                    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
                    
                    CGImageRef cgImage = CGImageCreate(nWidth,
                                                       nHeight,
                                                       8,
                                                       24,
                                                       nWidth*3,
                                                       colorSpace,
                                                       bitmapInfo,
                                                       provider,
                                                       NULL,
                                                       YES,
                                                       kCGRenderingIntentDefault);
                    CGColorSpaceRelease(colorSpace);
                    UIImage* image = [[UIImage alloc] initWithCGImage:cgImage];
                    _imageView.image = image;
                    CGImageRelease(cgImage);
                    CGDataProviderRelease(provider);
                    CFRelease(data);
                    break;
                }
            }
        }
    }
    return;
}

- (UIImage*)imageWithData:(uint8_t*)buffer withWidth:(int)width withHeight:(int)height
{
    NSData *data = [NSData dataWithBytes:buffer length:width*height*4];
    if (data) {
        CIImage *ci = [CIImage imageWithBitmapData:data
                                       bytesPerRow:width*4
                                              size:CGSizeMake(width, height)
                                            format:kCIFormatRGBA8
                                        colorSpace:NULL];
        if (ci) {
            UIImage *img = [UIImage imageWithCIImage:ci];
            return img;
        }
    }
    
    return nil;
}

/*
AVCodec ff_libx264_encoder = {
    .name             = "libx264",
    .type             = AVMEDIA_TYPE_VIDEO,
    .id               = AV_CODEC_ID_H264,
    .priv_data_size   = sizeof(X264Context),
    .init             = X264_init,
    .encode2          = X264_frame,
    .close            = X264_close,
    .capabilities     = CODEC_CAP_DELAY | CODEC_CAP_AUTO_THREADS,
    .long_name
    = NULL_IF_CONFIG_SMALL("libx264 H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"),
    .priv_class       = &class,
    .defaults         = x264_defaults,
    .init_static_data =  X264_init_static,
};
 */
@end
