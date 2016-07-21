//
//  rootViewController.h
//  DIDIVRClientSample
//
//  Created by Joseph on 16/3/25.
//  Copyright © 2016年 DIDI. All rights reserved.
//

#import <UIKit/UIKit.h>
@class KefuMsgClient;
@protocol DDASRKefuDegegate <NSObject>

/**
 根据返回服务器状态码的返回相应的结果
 */

- (void)GetMessgeFromKefu:(NSDictionary *)dic withStatus:(int)status;

/**
  录音文件保存后返回文件id以及错误码返回给语音客服助手的H5
 */
- (void)getFileName:(NSString *)str AndPath:(NSString *)path  AndERRORNumber:(NSString *)number AndStatus:(NSInteger)astatus;
/**
   获取正确返回的解析结果并返回给H5智能语音客服
 */
- (void)getFinishMsgWhenFinishTranslate:(NSString *)str withErrornum:(NSString *)number AndStatus:(NSInteger)astatus;

/**
 
 */
- (void)getErrorMsgWhenFinishTranslate:(NSString *)str withErrornum:(NSString *)number AndStatus:(NSInteger)astatus;
//- (void)getStatuesWhenFinishRecord:(int)astatues;


@end
@interface KefuMsgClient : UIViewController


@property (nonatomic,assign) id<DDASRKefuDegegate> delegate;

- (instancetype)init;
/**
 进行
 */
- (void)startRecWithPid:(NSDictionary *)dic;

- (void)finishSpeak;
- (void)cancelVocRec;

@end
