//
//  WaverView.h
//  WaverView
//
//  Created by kevinzhow on 14/12/14.
//  Copyright (c) 2014年 Catch Inc. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface WaverView : UIView

@property (nonatomic, copy) void (^waverLevelCallback)(WaverView *waverView);

//

@property (nonatomic) NSUInteger numberOfWaves;

@property (nonatomic) UIColor * waveColor;

@property (nonatomic) CGFloat level; //取值范围：0--0.3

@property (nonatomic) CGFloat mainWaveWidth; //主波纹宽度

@property (nonatomic) CGFloat decorativeWavesWidth; //辅助波纹宽度

@property (nonatomic) CGFloat idleAmplitude;

@property (nonatomic) CGFloat frequency;

@property (nonatomic, readonly) CGFloat amplitude; //振幅

@property (nonatomic) CGFloat density; //密度
@property (nonatomic) CGFloat phaseShift; //相移

//

@property (nonatomic, readonly) NSMutableArray * waves;

@end
