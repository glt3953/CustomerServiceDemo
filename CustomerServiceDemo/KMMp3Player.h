//
//  KMMp3Player.h
//  DIDIVRClientSample
//
//  Created by Joseph on 16/3/28.
//  Copyright © 2016年 DIDI. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface KMMp3Player : NSObject <AVAudioPlayerDelegate>

- (void)PlayWithContentFile:(NSString *)filename;
- (void)stopPlaying;

@end
