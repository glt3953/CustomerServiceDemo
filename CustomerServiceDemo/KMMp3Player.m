//
//  KMMp3Player.m
//  DIDIVRClientSample
//
//  Created by Joseph on 16/3/28.
//  Copyright © 2016年 DIDI. All rights reserved.
//
#import <UIKit/UIKit.h>
#import "KMMp3Player.h"
#import "Log.h"

@implementation KMMp3Player
{
    AVAudioPlayer * _audioPlayer;
}

- (instancetype)init {
    if (self = [super init]) {
        _audioPlayer = nil;
        _audioPlayer.delegate = nil;
    }
    
    return self;
}

- (void)PlayWithContentFile:(NSString *)filename {
    if (filename.length > 0) {
        [_audioPlayer stop];
        _audioPlayer = nil;
        _audioPlayer.delegate = nil;
        
        [[UIDevice currentDevice] setProximityMonitoringEnabled:YES];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(sensorStateChangea:)
                                                     name:@"UIDeviceProximityStateDidChangeNotification"
                                                   object:nil];
        //初始化播放器的时候如下设置
        UInt32 sessionCategory = kAudioSessionCategory_MediaPlayback;
        AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,
                                sizeof(sessionCategory),
                                &sessionCategory);
        
        UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
        AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute,
                                 sizeof (audioRouteOverride),
                                 &audioRouteOverride);
        
        //    _oldSessionCategory = [[AVAudioSession sharedInstance] category];
        AVAudioSession *audioSession = [AVAudioSession sharedInstance];
        //默认情况下扬声器播放
        [audioSession setCategory:AVAudioSessionCategoryPlayback error:nil];
        [audioSession setActive:YES error:nil];

        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:nil];
        NSError *error;
        _audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL fileURLWithPath:filename] error:&error];
        _audioPlayer.delegate = self;
        [_audioPlayer prepareToPlay];
        [_audioPlayer play];
    }
}

//判断手机距离人的远近
- (void)sensorStateChangea:(NSNotification *)notification {
    if ([[UIDevice currentDevice] proximityState] == YES) {
        LogDebug(@"Device is close to user");
        
        //        _oldSessionCategory = [[AVAudioSession sharedInstance]category];
        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord error:nil];
    } else {
        LogDebug(@"Device is faraway from user");
        //        _oldSessionCategory = [[AVAudioSession sharedInstance]category];
        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:nil];
    }
}

- (void)stopPlaying {
    [_audioPlayer stop];
    [[UIDevice currentDevice] setProximityMonitoringEnabled:NO];
}

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag {
    if (flag == YES) {
        [_audioPlayer stop];
        [[UIDevice currentDevice] setProximityMonitoringEnabled:NO];
        _audioPlayer = nil;
        _audioPlayer.delegate = nil;
    }
}

@end