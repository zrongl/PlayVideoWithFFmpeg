//
//  ShareInstance.h
//  Algorithm
//
//  Created by ronglei on 14-7-3.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#import <Foundation/Foundation.h>

@class SharedInstance;
static SharedInstance *sharedInstance = nil;

@interface SharedInstance : NSObject

+(SharedInstance *) sharedInstance;
+(id) allocWithZone:(NSZone *)zone;

@end
