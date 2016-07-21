//
//  KMRecorderTimer.m
//  DIDIVRClientSample
//
//  Created by Joseph on 16/3/25.
//  Copyright © 2016年 DIDI. All rights reserved.
//

#import "KMRecorderTimer.h"

static KMRecorderTimer * _instance = nil;

@implementation KMRecorderTimer

+ (KMRecorderTimer *)DefaultManager {
    static dispatch_once_t onceinit;
    dispatch_once(&onceinit, ^{
        _instance = [[self alloc] init];
    });
    return _instance;
}

- (void)startTimer {
    [self stopTimer];
    if (_timer == nil) {
        self.timeValue = 0;
        NSDate *tmpDate = [[NSDate alloc] initWithTimeIntervalSinceNow:VOICE_LEVEL_INTERVAL];
        NSTimer *tmpTimer = [[NSTimer alloc] initWithFireDate:tmpDate interval:VOICE_LEVEL_INTERVAL target:self selector:@selector(TimerAction) userInfo:nil repeats:YES];
        
        _timer = tmpTimer;
        NSRunLoop *main=[NSRunLoop currentRunLoop];
        [main addTimer:_timer forMode:NSRunLoopCommonModes];
    }
}

- (void)stopTimer {
    if (_timer) {
        if ([_timer isValid]) {
            [_timer invalidate];
            _timer = nil;
        }
    }
    if (_timer != nil) {
        _timer = nil;
        [_timer invalidate];
    }
}

- (void)TimerAction {
    self.timeValue++;
    if ([self.delegate respondsToSelector:@selector(TimerActionValueChange:)]) {
        [self.delegate TimerActionValueChange:self.timeValue];
    }
}

@end