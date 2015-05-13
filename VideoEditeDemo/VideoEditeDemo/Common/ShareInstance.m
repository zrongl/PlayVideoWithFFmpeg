//
//  ShareInstance.m
//  Algorithm
//
//  Created by ronglei on 14-7-3.
//  Copyright (c) 2014年 ronglei. All rights reserved.
//

#import "ShareInstance.h"
#import "Common.h"

@implementation SharedInstance

+ (SharedInstance *)sharedInstance
{
    //synchronized 这个主要是考虑多线程的程序，这个指令可以将{ } 内的代码限制在一个线程执行，如果某个线程没有执行完，其他的线程如果需要执行就得等着。
    @synchronized(self){
        if (sharedInstance == nil){
            sharedInstance = [[self alloc] init];
        }
    }
    return sharedInstance;
}

//allocWithZone 这个是重载的，因为这个是从制定的内存区域读取信息创建实例，所以如果需要的单例已经有了，就需要禁止修改当前单例，所以返回 nil
+(id) allocWithZone:(NSZone *)zone
{
    @synchronized(self){
        if (sharedInstance == nil){
            sharedInstance = [super allocWithZone:zone];
            return sharedInstance;
        }
    }
    return nil;
}

@end
