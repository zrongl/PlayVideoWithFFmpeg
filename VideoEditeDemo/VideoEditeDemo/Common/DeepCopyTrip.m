//
//  DeepCopyTrip.m
//  VodOne
//
//  Created by ronglei on 14-3-13.
//
//

#import "DeepCopyTrip.h"

#define DeepCloneTrip @"DeepCloneTrip"

@implementation DeepCopyTrip

+ (Trip *)getDeepCloneTrip:(Trip *)currentTrip
{
    NSMutableData *data = [[NSMutableData alloc] init];
    NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc]
                                 initForWritingWithMutableData:data];
    [archiver encodeObject:currentTrip forKey:DeepCloneTrip];
    [archiver finishEncoding];
    [archiver release];
    NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData:data];
    Trip *deepCloneTrip = [unarchiver decodeObjectForKey:DeepCloneTrip];
    [unarchiver finishDecoding];
    Trip *trips = [deepCloneTrip retain];
    [unarchiver release];
    [data release];
    
    return trips;
}

@end
