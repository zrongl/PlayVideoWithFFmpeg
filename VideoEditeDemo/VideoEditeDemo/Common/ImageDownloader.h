//
//  ImageDownloader.h
//  VideoEditeDemo
//
//  Created by ronglei on 14-5-21.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface ImageDownloader : NSObject

@property (strong, nonatomic) void (^downloadFinished)(UIImage *image);
@property (strong, nonatomic) void (^downloadFailed) (NSError *error);


- (void)startDownloadWithURL:(NSURL *)url
                  onFinished:(void(^)(UIImage *image))onFinished
                    onFailed:(void(^)(NSError *error))onFailed;

- (void)cancelDownload;

@end
