//
//  WaverView.m
//  WaverView
//
//  Created by kevinzhow on 14/12/14.
//  Copyright (c) 2014年 Catch Inc. All rights reserved.
//

#import "WaverView.h"
#import "UIColor+NingXia.h"

@interface WaverView ()

@property (nonatomic) CGFloat phase;
@property (nonatomic) CGFloat amplitude; //振幅
@property (nonatomic) NSMutableArray * waves; //波纹
@property (nonatomic) CGFloat waveHeight;
@property (nonatomic) CGFloat waveWidth;
@property (nonatomic) CGFloat waveMid;
@property (nonatomic) CGFloat maxAmplitude; //最大振幅
@property (nonatomic, strong) CADisplayLink *displayLink;

@end

@implementation WaverView


- (id)init
{
    if(self = [super init]) {
        [self setup];
    }
    
    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame]) {
        [self setup];
    }
    
    return self;
}

- (void)awakeFromNib
{
    [self setup];
}

- (void)setup
{
    self.waves = [NSMutableArray new];
    
    self.frequency = 1.2f;
    
    self.amplitude = 1.0f;
    self.idleAmplitude = 0.01f;
    
    self.numberOfWaves = 5;
    self.phaseShift = -0.25f;
    self.density = 1.f;
    
    self.waveColor = [UIColor whiteColor];
    self.mainWaveWidth = 2.0f;
    self.decorativeWavesWidth = 1.0f;
    
	self.waveHeight = CGRectGetHeight(self.bounds);
    self.waveWidth  = CGRectGetWidth(self.bounds);
    self.waveMid    = self.waveWidth / 2.0f;
//    self.maxAmplitude = self.waveHeight - 4.0f;
    self.maxAmplitude = 90;
    
    //定制样式
    for (int i = 0; i < self.numberOfWaves; i++) {
        CAShapeLayer *waveline = [CAShapeLayer layer];
        waveline.lineCap       = kCALineCapButt; //指定线的边缘
        waveline.lineJoin      = kCALineJoinRound;
        waveline.fillColor     = [[UIColor clearColor] CGColor]; //波纹的填充色
        switch (i) {
            case 0:
                [waveline setLineWidth:3];
                waveline.strokeColor = [[UIColor colorFromHexString:@"#fcc080"] CGColor]; //指定path的渲染颜色
                break;
            case 1:
                [waveline setLineWidth:2.5];
                waveline.strokeColor = [[UIColor colorFromHexString:@"#ffb8b6" alpha:0.4] CGColor];
                break;
            case 2:
                [waveline setLineWidth:2];
                waveline.strokeColor = [[UIColor colorFromHexString:@"#fcc080"] CGColor];
                break;
            case 3:
                [waveline setLineWidth:1.5];
                waveline.strokeColor = [[UIColor colorFromHexString:@"#ffb8b6" alpha:0.4] CGColor];
                break;
            default:
                break;
        }
        [self.layer addSublayer:waveline];
        [self.waves addObject:waveline];
    }
}

- (void)setWaverLevelCallback:(void (^)(WaverView *waverView))waverLevelCallback {
    _waverLevelCallback = waverLevelCallback;

    [self.displayLink invalidate];
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(invokeWaveCallback)];
    [self.displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)invokeWaveCallback
{
    self.waverLevelCallback(self);
}

- (void)setLevel:(CGFloat)level
{
    _level = level;
    
    self.phase += self.phaseShift; // Move the wave
    
    self.amplitude = fmax(level, self.idleAmplitude);
//    NSLog(@"_level:%f, self.phaseShift:%f, self.phase:%f, self.idleAmplitude:%f, self.amplitude:%f", _level, self.phaseShift, self.phase, self.idleAmplitude, self.amplitude);
    
    [self updateMeters];
}


- (void)updateMeters
{
	self.waveHeight = CGRectGetHeight(self.bounds);
	self.waveWidth  = CGRectGetWidth(self.bounds);
	self.waveMid    = self.waveWidth / 2.0f;
//	self.maxAmplitude = self.waveHeight - 4.0f;
    self.maxAmplitude = 90;
	
    UIGraphicsBeginImageContext(self.frame.size);
    
    for(int i = 0; i < self.numberOfWaves; i++) {

        UIBezierPath *wavelinePath = [UIBezierPath bezierPath];

        // Progress is a value between 1.0 and -0.5, determined by the current wave idx, which is used to alter the wave's amplitude.
        CGFloat progress = 1.0f - (CGFloat)i / self.numberOfWaves;
        CGFloat normedAmplitude = (1.5f * progress - 0.5f) * self.amplitude;
//        NSLog(@"progress:%f, self.amplitude:%f, normedAmplitude:%f", progress, self.amplitude, normedAmplitude);
        CAShapeLayer *waveline = [self.waves objectAtIndex:i];
        
        //x初始值依赖于self.frame.origin.x
        for (CGFloat x = self.frame.origin.x; x < self.waveWidth + self.density; x += self.density) {
            //Thanks to https://github.com/stefanceriu/SCSiriWaveformView
            // We use a parable to scale the sinus wave, that has its peak in the middle of the view.
            //缩放
            //double pow (double base, double exponent);求base的exponent次方值
            CGFloat scaling = -pow(x / self.waveMid  - 1, 2) + 1; // make center bigger
            //sinf：计算正弦值和双曲线的正弦值
            CGFloat y = scaling * self.maxAmplitude * normedAmplitude * sinf(2 * M_PI *(x / self.waveWidth) * self.frequency + self.phase) + (self.waveHeight * 0.5);
            
            if (x == self.frame.origin.x) {
                /**
                 *  设置第一个起始点到接收器
                 *  @param point 起点坐标
                 */
                [wavelinePath moveToPoint:CGPointMake(x, y)];
            } else {
                /**
                 *  附加一条直线到接收器的路径
                 *  @param point 要到达的坐标
                 */
                [wavelinePath addLineToPoint:CGPointMake(x, y)];
            }
//            [wavelinePath strokeWithBlendMode:kCGBlendModeNormal alpha:0.5];
//            if (i % 2 == 0) {
//                waveline.strokeColor = [[UIColor colorFromHexString:@"#fcc080" alpha:x / self.waveWidth] CGColor];
//            } else {
//                waveline.strokeColor = [[UIColor colorFromHexString:@"#ffb8b6" alpha:x / self.waveWidth] CGColor];
//            }
            
            if (x >= 0) {
                waveline.opacity = x / self.waveWidth; //设置透明度
            }
//            NSLog(@"alpha:%f", x / self.waveWidth);
        }
        
        waveline.path = [wavelinePath CGPath];
    }
    
    UIGraphicsEndImageContext();
}

- (void)dealloc
{
    [_displayLink invalidate];
}

@end
