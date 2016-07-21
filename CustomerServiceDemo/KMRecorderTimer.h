//
//  KMRecorderTimer.h
//  DIDIVRClientSample
//
//  Created by Joseph on 16/3/25.
//  Copyright © 2016年 DIDI. All rights reserved.
//

#import <Foundation/Foundation.h>
@class KMRecorderTimer;

#define VOICE_LEVEL_INTERVAL 0.1

@protocol KMRecorderTimerDelegate <NSObject>

- (void)TimerActionValueChange:(int)time; //时间改变

@end

@interface KMRecorderTimer : NSObject
{
    NSTimer * _timer;
}

@property (nonatomic) int timeValue;
@property (nonatomic, assign) id<KMRecorderTimerDelegate>delegate;

+ (KMRecorderTimer *)DefaultManager;
- (void)startTimer;
- (void)stopTimer;

@end