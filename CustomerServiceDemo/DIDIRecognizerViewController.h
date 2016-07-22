//
//  DIDIRecognizerViewController.h
//  DIDIVoiceRecognitionClient
//
//  Created by didi on 13-9-25.
//  Copyright (c) 2013 didi Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import "DIDIRecognizerViewDelegate.h"
#import "DIDIVoiceRecognitionClient.h"
#import "DIDIRecognizerViewParamsObject.h"
#import "DIDITheme.h"

/**
 * @brief 语音识别弹窗视图控制类
 */
@interface DIDIRecognizerViewController : UIViewController<MVoiceRecognitionClientDelegate, DIDIRecognizerDialogDelegate, AVAudioPlayerDelegate>
{
    id<DIDIRecognizerViewDelegate> delegate;
}

@property (assign) id<DIDIRecognizerViewDelegate> delegate; // 识别结果delegate
@property (nonatomic, readonly) DIDITheme *recognizerViewTheme; // 得到当前的主题
@property (nonatomic, assign) BOOL enableFullScreenMode; // 全屏模式

/**
 * @brief 创建弹窗实例
 * @param origin 控件左上角的坐标
 * @param theme 控件的主题，如果为nil，则为默认主题
 *
 * @return 弹窗实例
 */
- (id)initWithOrigin:(CGPoint)origin withTheme:(DIDITheme *)theme;

/**
 * @brief 启动识别
 * @param params 识别过程的参数，具体项目参考DIDIRecognizerViewParamsObject类声明，注意参数不能为空，必须要设置apiKey和secretKey
 *
 * @return 开始识别是否成功，成功为YES，否则为NO
 */
- (BOOL)startWithParams:(DIDIRecognizerViewParamsObject *)params;

/**
 * @brief - 取消本次识别，并移除View
 */
- (void)cancel;

/**
 * @brief 屏幕旋转后调用设置识别弹出窗位置
 *
 */
- (void)changeFrameAfterOriented:(CGPoint)origin;
@end // DIDIRecognizerViewController
