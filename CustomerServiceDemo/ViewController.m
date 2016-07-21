//
//  ViewController.m
//  CustomerServiceDemo
//
//  Created by guoliting on 16/7/21.
//  Copyright © 2016年 guoliting. All rights reserved.
//

#import "ViewController.h"
#import "KefuMsgClient.h"
#import "KMFileManager.h"
#import "KMMp3Player.h"

@interface ViewController () //<DDASRKefuDegegate>

@property (nonatomic, strong) KefuMsgClient *kefuClient;
@property (nonatomic, strong) KMMp3Player *mp3Player;
@property (nonatomic, copy) NSDictionary *userInfoDic;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
    _kefuClient = [[KefuMsgClient alloc] init];
    //    _kefuClient.delegate = self;
    _mp3Player = [[KMMp3Player alloc] init];
    _userInfoDic = @{@"chatInfo":@{@"businessType":@1, @"cell":@18800000004, @"cityId":@1, @"message":@"", @"mid":@"462d7806-3295-4355-a719-1c4a71e0bf7b", @"msgType":@0, @"orderId":@0, @"roleType":@3, @"skillType":@"common", @"source":@-1, @"uid":@564069099110401}, @"pid":@10001};
    
    UIButton *startRecButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [startRecButton setFrame:(CGRect){20, 240 + 150, 80, 30}];
    [startRecButton setTitle:@"开始录音" forState:UIControlStateNormal];
    [startRecButton addTarget:self action:@selector(startRecButtonDidClick:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:startRecButton];
    
    UIButton *stopRecButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [stopRecButton setFrame:(CGRect){150, 240 + 150, 80, 30}];
    [stopRecButton setTitle:@"结束录音" forState:UIControlStateNormal];
    [stopRecButton addTarget:self action:@selector(stopRecButtonDidClick:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:stopRecButton];
    
    UIButton *playRecButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [playRecButton setFrame:(CGRect){20, 240 + 200, 80, 30}];
    [playRecButton setTitle:@"播放录音" forState:UIControlStateNormal];
    [playRecButton addTarget:self action:@selector(playRecButtonDidClick:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:playRecButton];
    
    UIButton *stopPlayButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [stopPlayButton setFrame:(CGRect){150, 240 + 200, 80, 30}];
    [stopPlayButton setTitle:@"停止播放" forState:UIControlStateNormal];
    [stopPlayButton addTarget:self action:@selector(stopPlayButtonDidClick:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:stopPlayButton];
}

- (IBAction)startRecButtonDidClick:(id)sender {
    [_kefuClient startRecWithPid:_userInfoDic];
}

- (IBAction)stopRecButtonDidClick:(id)sender {
    [_kefuClient finishSpeak];
}

- (IBAction)playRecButtonDidClick:(id)sender {
    NSString *kefuClientPath = [[KMFileManager shareManager] createFilePath];
    
    NSString *keyPath = [kefuClientPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@",[NSString stringWithFormat:@"%@.mp3", _userInfoDic[@"id"]]]];
    [_mp3Player PlayWithContentFile:keyPath];
}

- (IBAction)stopPlayButtonDidClick:(id)sender {
    [_mp3Player stopPlaying];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
