//
//  rootViewController.m
//  DIDIVRClientSample
//
//  Created by Joseph on 16/3/25.
//  Copyright © 2016年 DIDI. All rights reserved.
//
#define ZKMRecorderViewRect CGRectMake([UIScreen mainScreen].bounds.size.width/2-100, [UIScreen mainScreen].bounds.size.height/2-85, 200, 170)
#define kCancelOriginY          ([[UIScreen mainScreen]bounds].size.height-70)
#define MYDDVR_SEARCH_SEVER @"http://didiiml.xiaojukeji.com/asr"

#import "KefuMsgClient.h"
//#import "DIDIVoiceRecognitionClient.h"
#import "lame.h"
#import <Availability.h>
#import "KMRecorderTimer.h"
#import "ChatRecorderView.h"
#import "KMFileManager.h"
#import "Log.h"
#import "Macros.h"

@interface KefuMsgClient () <KMFileManagerDelegate, KMRecorderTimerDelegate>
{
    NSMutableData *recData;
    KMRecorderTimer *_RecLevetimer;
//    KMVoiceManageVC * recManager;
    ChatRecorderView *ctView;
    BOOL ISERROR;
    BOOL ISCONNECTEDTOSEVER;
    NSInteger RecordTimeCount;
    
    NSInteger errMsgStatus;
    NSString *errMsgStr;
}

@end

@implementation KefuMsgClient

- (instancetype)init {
    if (self = [super init]) {
//        self.TimeValue = 0;
        ISERROR = NO;
        ISCONNECTEDTOSEVER = NO;
        ctView  = [[ChatRecorderView alloc] init];
        recData = [[NSMutableData alloc] init];
        [super viewDidLoad];
        _RecLevetimer = [KMRecorderTimer DefaultManager];
        _RecLevetimer.delegate = self;
        RecordTimeCount = 0;
        errMsgStatus = 0;
        errMsgStr = [[NSString alloc] init];
    }
    
    return self;
}

//开始解析
- (void)startRecWithPid:(NSDictionary *)pid {
    errMsgStatus = 0;
    errMsgStr = @"";
    RecordTimeCount = 0;
    ISCONNECTEDTOSEVER = NO;
    ISERROR = NO;
    LogDebug(@"1111%@", pid[@"pid"]);
    LogDebug(@"dadd%@", pid[@"chatInfo"]);
    [self ShowChatview];
    [self startVocRecWithdic:pid];
    [[KMFileManager shareManager] createFilePath];
    
    [[UIApplication sharedApplication].keyWindow addSubview:self.view];
}

//检测到用户超过1S无录音，则结束
- (void)cancelVocRec {
//    self.TimeValue = 0;
    [self HidechatView];
    
//    [[DIDIVoiceRecognitionClient sharedInstance] stopVoiceRecognition];
}

- (void)viewDidLoad {
    
}

- (void)ShowChatview {
    [ctView setFrame:ZKMRecorderViewRect];
    [ctView restoreDisplay];
    [self.view addSubview:ctView];
}

//用户主动结束录音
- (void)finishSpeak {
    [self HidechatView];
//    [[DIDIVoiceRecognitionClient sharedInstance] speakFinish];

    [self.view removeFromSuperview];
}

- (void)HidechatView {
    [UIView animateWithDuration:0.1 animations:^{
            [ctView setFrame:CGRectMake(0, 0, 0, 0)];
        }];
    [ctView removeFromSuperview];
}

- (void)startVocRecWithdic:(NSDictionary *)pid {
//    NSDictionary * dic = pid;
    LogDebug(@"~~~~~~~~~~~~~~~~~~~~~~%@~~~~~~~~~~~~~~~~~~~~~~~", pid);
//    [[DIDIVoiceRecognitionClient sharedInstance] stopVoiceRecognition];
    NSString * str = pid[@"url"];
    if(str == nil) {
        LogDebug(@"-------------nil----------------");
//        [[DIDIVoiceRecognitionClient sharedInstance] setServerURL:MYDDVR_SEARCH_SEVER withMode:EVoiceRecognitionPropertySearch];
//        [[DIDIVoiceRecognitionClient sharedInstance] setServerURL:MYDDVR_SEARCH_SEVER withMode:EVoiceRecognitionPropertyInput];
    } else {
        LogDebug(@"-------------YES－－－－－－－－－－－－");
//        [[DIDIVoiceRecognitionClient sharedInstance] setServerURL:pid[@"url"] withMode:EVoiceRecognitionPropertySearch];
//        [[DIDIVoiceRecognitionClient sharedInstance] setServerURL:pid[@"url"] withMode:EVoiceRecognitionPropertyInput];
    }
//    [[DIDIVoiceRecognitionClient sharedInstance] setProductId:[NSString stringWithFormat:@"%@", pid[@"pid"]]];
//    [[DIDIVoiceRecognitionClient sharedInstance] setParamForKey:@"app_param" withValue:[self toJSONData:pid[@"chatInfo"]]];
//    [[DIDIVoiceRecognitionClient sharedInstance]setLocalVAD:NO];
//    [[DIDIVoiceRecognitionClient sharedInstance]setServerVAD:NO];
    
//    [[DIDIVoiceRecognitionClient sharedInstance]setNeedCompressFlag:YES];
    
//    [[DIDIVoiceRecognitionClient sharedInstance] set_max_wait_duration:];
//    [[DIDIVoiceRecognitionClient sharedInstance] setRecordSampleRate:16000];
//    [[DIDIVoiceRecognitionClient sharedInstance] listenCurrentDBLevelMeter];
    
    int startStatus = -1;
//    startStatus = [[DIDIVoiceRecognitionClient sharedInstance] startVoiceRecognition:self];
//    if (startStatus != EVoiceRecognitionStartWorking) {
//        // 创建失败则报告错误
//        NSString *statusString = [NSString stringWithFormat:@"%d", startStatus];
//        [self performSelector:@selector(firstStartError:) withObject:statusString afterDelay:0.3];  // 延迟0.3秒，以便能在出错时正常删除view
//        
//        [self HidechatView];
//        [self.view removeFromSuperview];
//        return;
//    }
}

- (void)firstStartError:(NSString *)str {
    LogDebug(@"错误");
    errMsgStatus = 2;
    errMsgStr = lang(@"CheckNetwork");
    
    [KMFileManager shareManager].delegate = self;
    [[KMFileManager shareManager]SaveMP3FileWithData:recData];
    LogDebug(@"%@",[KMFileManager shareManager].createFilePath);
    
//    if (str.integerValue == EVoiceRecognitionStartWorkNOMicrophonePermission) {
//        errMsgStatus = 2;
//        [self HidechatView];
//        errMsgStr = lang(@"AllowAccessMicrophone");
//        [_RecLevetimer stopTimer];
//        [[DIDIVoiceRecognitionClient sharedInstance] stopVoiceRecognition];
//    } else if (str.integerValue == EVoiceRecognitionStartWorkNetUnusable) {
//        [self HidechatView];
//        errMsgStatus = 2;
//        [_RecLevetimer stopTimer];
//        errMsgStr = [lang(@"SendFailed") stringByAppendingString:lang(@"CheckNetwork")];
//        [[DIDIVoiceRecognitionClient sharedInstance] stopVoiceRecognition];
//    } else {
//        [_RecLevetimer stopTimer];
//        ISERROR = YES;
//        [_RecLevetimer stopTimer];
//        [[DIDIVoiceRecognitionClient sharedInstance] stopVoiceRecognition];
//    }
    
    [self KMFileManagergetFilePath:@"" AndFileName:@""];
    [self HidechatView];
    [self.view removeFromSuperview];
}

- (void)VoiceRecognitionClientErrorStatus:(int)aStatus subStatus:(int)aSubStatus {
    ISERROR = YES;
//    switch (aSubStatus) {
//        case EVoiceRecognitionClientErrorStatusChangeNotAvailable:
//        {
//            errMsgStatus = 1;
//            errMsgStr = lang(@"AllowAccessMicrophone");
//
//            break;
//        }
//        case EVoiceRecognitionClientErrorStatusUnKnow:
//        {
//            errMsgStatus = 3;
//            errMsgStr = lang(@"RequestTimeOut");
//            
//            break;
//        }
//        case EVoiceRecognitionClientErrorStatusNoSpeech:
//        {
//            errMsgStatus = 4;
//            errMsgStr = lang(@"TalkTimeIsTooShort");
////            errorMsg = NSLocalizedString(@"StringVoiceRecognitonNoSpeech", nil);
//            break;
//        }
//        case EVoiceRecognitionClientErrorStatusShort:
//        {
//
//            errMsgStatus = 4;
//            errMsgStr = lang(@"TalkTimeIsTooShort");
//            break;
//        }
//        case EVoiceRecognitionClientErrorStatusException:
//        {
////            errorMsg = NSLocalizedString(@"StringVoiceRecognitonException", nil);
//            errMsgStatus = 3;
//            errMsgStr = lang(@"RequestTimeOut");
//            break;
//        }
//        case EVoiceRecognitionClientErrorNetWorkStatusError:
//        {
////            errorMsg = NSLocalizedString(@"StringVoiceRecognitonNetWorkError", nil);
//            break;
//        }
//        case EVoiceRecognitionClientErrorNetWorkStatusUnusable:
//        {
//            errMsgStatus = 2;
//            errMsgStr = [lang(@"SendFailed") stringByAppendingString:lang(@"CheckNetwork")];
////            errorMsg = NSLocalizedString(@"StringVoiceRecognitonNoNetWork", nil);
//            break;
//        }
//        case EVoiceRecognitionClientErrorNetWorkStatusTimeOut:
//        {
//            errMsgStatus = 3;
//            errMsgStr = lang(@"RequestTimeOut");
////            errorMsg = NSLocalizedString(@"StringVoiceRecognitonNetWorkTimeOut", nil);
//            break;
//        }
//        case EVoiceRecognitionClientErrorNetWorkStatusParseError:
//        {
//            
////            errorMsg = NSLocalizedString(@"StringVoiceRecognitonParseError", nil);
//            errMsgStatus = 3;
//            errMsgStr = lang(@"RequestTimeOut");
//            break;
//        }
//        case EVoiceRecognitionStartWorkNoAPIKEY:
//        {
////            errorMsg = NSLocalizedString(@"StringAudioSearchNoAPIKEY", nil);
//            break;
//        }
//        case EVoiceRecognitionStartWorkGetAccessTokenFailed:
//        {
////            errorMsg = NSLocalizedString(@"StringAudioSearchGetTokenFailed", nil);
//            break;
//        }
//        case EVoiceRecognitionStartWorkDelegateInvaild:
//        {
//            errMsgStatus = 3;
//            errMsgStr = lang(@"RequestTimeOut");
////            errorMsg = NSLocalizedString(@"StringVoiceRecognitonNoDelegateMethods", nil);
//            break;
//        }
//        case EVoiceRecognitionStartWorkNetUnusable:
//        {
////            errorMsg = NSLocalizedString(@"StringVoiceRecognitonNoNetWork", nil);
//            errMsgStr = lang(@"AllowAccessMicrophone");
//            errMsgStatus =1;
//            break;
//        }
//        case EVoiceRecognitionStartWorkRecorderUnusable:
//        {
//            errMsgStr = lang(@"AllowAccessMicrophone");
//            errMsgStatus = 1;
////            errorMsg = NSLocalizedString(@"StringVoiceRecognitonCantRecord", nil);
//            break;
//        }
//        case EVoiceRecognitionStartWorkNOMicrophonePermission:
//        {
//            errMsgStr = lang(@"AllowAccessMicrophone");
//            errMsgStatus = 1;
////            errorMsg = NSLocalizedString(@"StringAudioSearchNOMicrophonePermission", nil);
//            break;
//        }
//            //服务器返回错误
//        case EVoiceRecognitionClientErrorNetWorkStatusServerNoFindResult:{
//            errMsgStatus = 3;
//            errMsgStr = lang(@"RequestTimeOut");
//            break;
//        }//没有找到匹配结果
//        case EVoiceRecognitionClientErrorNetWorkStatusServerSpeechQualityProblem:{
//            errMsgStatus = 4;
//            errMsgStr = lang(@"TalkTimeIsTooShort");
//            break;
//        }
//            //声音过小
//        case EVoiceRecognitionClientErrorNetWorkStatusServerParamError:{
//            errMsgStatus = 3;
//            errMsgStr = lang(@"SendFailed");
//            break;
//        }//协议参数错误
//        case EVoiceRecognitionClientErrorNetWorkStatusServerRecognError:
//        {
//            errMsgStatus = 3;
//            errMsgStr = lang(@"SendFailed");
//            break;
//        }//识别过程出错
//        case EVoiceRecognitionClientErrorNetWorkStatusServerAppNameUnknownError:{
//            errMsgStatus = 3;
//            errMsgStr = lang(@"RequestTimeOut");
//            break;
//        }//appName验证错误
//        case EVoiceRecognitionClientErrorNetWorkStatusServerUnknownError:      //未知错误
//        {
////            errorMsg = [NSString stringWithFormat:@"%@-%d",NSLocalizedString(@"StringVoiceRecognitonServerError", nil),aStatus] ;
//            errMsgStr = lang(@"SendFailed");
//            errMsgStatus = 3;
//            break;
//        }
//        default:
//        {
//            
//            break;
//        }
//    }
    
    [KMFileManager shareManager].delegate = self;
    [[KMFileManager shareManager]SaveMP3FileWithData:recData];
    LogDebug(@"%@",[KMFileManager shareManager].createFilePath);
    
    dispatch_async(dispatch_get_main_queue(), ^{
    if ([self.delegate respondsToSelector:@selector(getFinishMsgWhenFinishTranslate:withErrornum:AndStatus:)]) {
            [self.delegate getFinishMsgWhenFinishTranslate:@"" withErrornum:errMsgStr AndStatus:errMsgStatus];
    }
    });
    if ([self.delegate respondsToSelector:@selector(getErrorMsgWhenFinishTranslate:withErrornum:AndStatus:)]) {
        [self.delegate getErrorMsgWhenFinishTranslate:@"" withErrornum:errMsgStr AndStatus:errMsgStatus];
    }
//    [self KMFileManagergetFilePath:@"" AndFileName:@""];
    [_RecLevetimer stopTimer];

//    UIAlertView * alert = [[UIAlertView alloc]initWithTitle:@"cuowu" message:[NSString stringWithFormat:@"%@",errMsgStr] delegate:nil cancelButtonTitle:@"确定" otherButtonTitles: nil];
//    [alert show];
    

    [_RecLevetimer stopTimer];
//    [[DIDIVoiceRecognitionClient sharedInstance] stopVoiceRecognition];
    [self HidechatView];
    [self.view removeFromSuperview];
}

//- (void)VoiceRecognitionClientWorkStatus:(int)aStatus obj:(id)aObj {
//    switch (aStatus) {
//        case EVoiceRecognitionClientWorkStatusFlushData: // 连续上屏中间结果
//        {
//            LogDebug(@"nafanlakngrklangklnagnaekgn");
//            LogDebug(@"jkanejfkanwlgnjalnglawn_________________");
//            NSString *text = [aObj objectAtIndex:0];
//            if ([text length] > 0) {
//                LogDebug(@"%@",text);
////                ctView.textView.text = text;
//                [ctView setTextViewText:text];
//            }
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusFinish: // 识别正常完成并获得结果
//        {
//            [self createRunLogWithStatus:aStatus];
//            
//            [_RecLevetimer stopTimer];
//            [self HidechatView];
//            
//            if ([[DIDIVoiceRecognitionClient sharedInstance] getRecognitionProperty] != EVoiceRecognitionPropertyInput) {
//                NSMutableArray *audioResultData = (NSMutableArray *)aObj;
//                NSMutableString *tmpString = [[NSMutableString alloc] initWithString:@""];
//                
//                for (int i = 0; i < [audioResultData count]; i++) {
//                    [tmpString appendFormat:@"%@\r\n",[audioResultData objectAtIndex:i]];
//                }
//                LogDebug(@"%@", tmpString);
//            } else {
////                if ((ISCONNECTEDTOSEVER == YES && ISERROR == NO)){
//                if (RecordTimeCount >= 3) {
//                    if ([self.delegate respondsToSelector:@selector(getFinishMsgWhenFinishTranslate:withErrornum:AndStatus:)]) {
//                        [self.delegate getFinishMsgWhenFinishTranslate:[NSString stringWithFormat:@"%@", [aObj objectAtIndex:0]] withErrornum:errMsgStr AndStatus:0];
//                    }
//                }
//            }
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusReceiveData:
//        {
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusEnd:
//        {
//            // 用户说话完成，等待服务器返回识别结果
//            if ((ISCONNECTEDTOSEVER == YES && ISERROR == NO) || RecordTimeCount >= 3) {
//                [KMFileManager shareManager].delegate = self;
//                [[KMFileManager shareManager]SaveMP3FileWithData:recData];
//                LogDebug(@"%@",[KMFileManager shareManager].createFilePath);
//                [_RecLevetimer stopTimer];
//            }
//            
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusCancel:
//        {
//            [_RecLevetimer stopTimer];
//
////            self.TimeValue = 0;
////
////            [self createRunLogWithStatus:aStatus];
////            
////            if (self.view.superview)
////            {
////                [self.view removeFromSuperview];
////            }
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusStartWorkIng:
//        {
//            // 识别库开始识别工作，用户可以说话
//            LogDebug(@"开始工作");
//   // 如果播放了提示音，此时再给用户提示可以说话
//            NSData * data = [[NSData alloc]init];
//            [recData setData:data];
//            [_RecLevetimer stopTimer];
//            [_RecLevetimer startTimer];
//            
//            [self createRunLogWithStatus:aStatus];
//            
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusNone:
//        case EVoiceRecognitionClientWorkPlayStartTone:
//        case EVoiceRecognitionClientWorkPlayStartToneFinish:
//        case EVoiceRecognitionClientWorkStatusStart:
//        case EVoiceRecognitionClientWorkPlayEndToneFinish:
//        {
//            break;
//        }
//        case EVoiceRecognitionClientWorkPlayEndTone:
//        {
//            [self createRunLogWithStatus:aStatus];
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusNewRecordData:
//        {
//            [recData appendData:aObj];
////            LogDebug(@"我是data－－－－－－－－－data%@",aObj);
//            
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusError:
//        {
//            break;
//        }
//        default:
//        {
//            [self createRunLogWithStatus:aStatus];
//            
//            [_RecLevetimer stopTimer];
//            
//            break;
//        }
//    }
//}

- (void)KMFileManagergetFilePath:(NSString *)path AndFileName:(NSString *)fileName {
    if ([self.delegate respondsToSelector:@selector(getFileName:AndPath: AndERRORNumber:AndStatus:)]) {
        [self.delegate getFileName:fileName AndPath:path AndERRORNumber:errMsgStr AndStatus:errMsgStatus];
    }
    
    LogDebug(@"filePath:%@", path);
    LogDebug(@"fileName%@", fileName);
}

//实时音量检测
- (void)TimerActionValueChange:(int)time {
//    self.TimeValue = time/10;
    RecordTimeCount = time;
//    int voiceLevel = [[DIDIVoiceRecognitionClient sharedInstance] getCurrentDBLevelMeter];
//    LogDebug(@"111112131adada~~~%.2f",(float)voiceLevel/100);
//    [ctView updateMetersByAvgPower:(float)voiceLevel/100];
    
//    NSString *statusMsg = [NSLocalizedString(@"StringLogVoiceLevel", nil) stringByAppendingFormat:@": %d", voiceLevel];
//    LogDebug(@"%@", statusMsg);
//    [clientSampleViewController logOutToLogView:statusMsg];

}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

//- (void)createRunLogWithStatus:(int)aStatus {
//    NSString *statusMsg = nil;
//    switch (aStatus) {
//        case EVoiceRecognitionClientWorkStatusNone:
//        {
//            //空闲
//            statusMsg = NSLocalizedString(@"StringLogStatusNone", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkPlayStartTone:
//        {
//            //播放开始提示音
//            statusMsg = NSLocalizedString(@"StringLogStatusPlayStartTone", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkPlayStartToneFinish:
//        {
//            //播放开始提示音完成
//            statusMsg = NSLocalizedString(@"StringLogStatusPlayStartToneFinish", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusStartWorkIng:
//        {
//            //识别工作开始，开始采集及处理数据
//            statusMsg = NSLocalizedString(@"StringLogStatusStartWorkIng", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusStart:
//        {
//            //检测到用户开始说话
//            statusMsg = NSLocalizedString(@"StringLogStatusStart", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkPlayEndTone:
//        {
//            //播放结束提示音
//            statusMsg = NSLocalizedString(@"StringLogStatusPlayEndTone", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkPlayEndToneFinish:
//        {
//            //播放结束提示音完成
//            statusMsg = NSLocalizedString(@"StringLogStatusPlayEndToneFinish", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusReceiveData:
//        {
//            //语音识别功能完成，服务器返回正确结果
//            statusMsg = NSLocalizedString(@"StringLogStatusSentenceFinish", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusFinish:
//        {
//            //语音识别功能完成，服务器返回正确结果
//            statusMsg = NSLocalizedString(@"StringLogStatusFinish", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusEnd:
//        {
//            //本地声音采集结束结束，等待识别结果返回并结束录音
//            statusMsg = NSLocalizedString(@"StringLogStatusEnd", nil);
//            break;
//        }
//        case EVoiceRecognitionClientNetWorkStatusStart:
//        {
//            //网络开始工作
//            statusMsg = NSLocalizedString(@"StringLogStatusNetWorkStatusStart", nil);
//            break;
//        }
//        case EVoiceRecognitionClientNetWorkStatusEnd:
//        {
//            //网络工作完成
//            statusMsg = NSLocalizedString(@"StringLogStatusNetWorkStatusEnd", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusCancel:
//        {
//            // 用户取消
//            statusMsg = NSLocalizedString(@"StringLogStatusNetWorkStatusCancel", nil);
//            break;
//        }
//        case EVoiceRecognitionClientWorkStatusError:
//        {
//            // 出现错误
//            statusMsg = NSLocalizedString(@"StringLogStatusNetWorkStatusErorr", nil);
//            break;
//        }
//        default:
//        {
//            statusMsg = NSLocalizedString(@"StringLogStatusNetWorkStatusDefaultErorr", nil);
//            break;
//        }
//    }
//    
//    if (statusMsg) {
////        NSString *logString = self.clientSampleViewController.logCatView.text;
////        if (logString && [logString isEqualToString:@""] == NO)
////        {
////            self.clientSampleViewController.logCatView.text = [logString stringByAppendingFormat:@"\r\n%@", statusMsg];
////        }
////        else 
////        {
////            self.clientSampleViewController.logCatView.text = statusMsg;
////        }
//    }
//}

- (NSString *)toJSONData:(id)theData {
    NSError *error = nil;
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:theData
                                                       options:NSJSONWritingPrettyPrinted
                                                         error:&error];
    
    if ([jsonData length] > 0 && error == nil) {
        return [[NSString alloc] initWithData:jsonData
                                              encoding:NSUTF8StringEncoding];
    } else {
        return nil;
    }
}

- (void)showAlertWith:(NSString *)msg {
//    UIAlertView * alert = [[UIAlertView alloc]initWithTitle:@"提示" message:msg delegate:nil cancelButtonTitle:@"确定" otherButtonTitles: nil];
//    [alert show];
}

#pragma mark - 是否摇晃垃圾桶


/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
