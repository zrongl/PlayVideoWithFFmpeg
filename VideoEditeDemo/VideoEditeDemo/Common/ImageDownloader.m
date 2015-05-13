//
//  ImageDownloader.m
//  VideoEditeDemo
//
//  Created by ronglei on 14-5-21.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#import "ImageDownloader.h"

@interface ImageDownloader()

@property (nonatomic, strong) NSString *imageURL;
@property (nonatomic, strong) NSMutableData *activeDownload;
@property (nonatomic, strong) NSURLConnection *imageConnection;

@end

@implementation ImageDownloader

- (void)startDownloadWithURL:(NSURL *)url
                  onFinished:(void(^)(UIImage *image))onFinished
                    onFailed:(void(^)(NSError *error))onFailed
{
    self.downloadFinished = onFinished;
    self.downloadFailed = onFailed;
    
    self.activeDownload = [NSMutableData data];
    NSURLConnection *conn = [[NSURLConnection alloc] initWithRequest:
                             [NSURLRequest requestWithURL:
                              url] delegate:self];
    if (conn) {
        [conn start];
        self.imageConnection = conn;
    }
}

- (void)cancelDownload
{
    [self.imageConnection cancel];
    self.imageConnection = nil;
    self.activeDownload = nil;
}

#pragma mark -
#pragma mark Download support (NSURLConnectionDelegate)
- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    [self.activeDownload appendData:data];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    if (_downloadFailed) {
        _downloadFailed(error);
    }
    
    self.activeDownload = nil;
    self.imageConnection = nil;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    UIImage *image = [[UIImage alloc] initWithData:self.activeDownload];
    self.activeDownload = nil;
    self.imageConnection = nil;

    if (_downloadFinished) {
        _downloadFinished(image);
    }
}
@end
