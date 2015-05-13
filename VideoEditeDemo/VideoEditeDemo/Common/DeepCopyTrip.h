//
//  DeepCopyTrip.h
//  VodOne
//
//  Created by ronglei on 14-3-13.
//
//

#import <Foundation/Foundation.h>

typedef NSMutableArray Trip;

@interface DeepCopyTrip : NSObject

+ (Trip *)getDeepCloneTrip:(Trip *)currentTrip;

@end
