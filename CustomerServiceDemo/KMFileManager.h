//
//  KMFileManager.h
//  DIDIVRClientSample
//
//  Created by Joseph on 16/3/25.
//  Copyright © 2016年 DIDI. All rights reserved.
//

#import <Foundation/Foundation.h>

@class KMFileManager;

@protocol KMFileManagerDelegate <NSObject>

- (void)KMFileManagergetFilePath:(NSString *)path AndFileName:(NSString *)fileName;

@end

@interface KMFileManager : NSObject

@property (nonatomic,assign) id<KMFileManagerDelegate>delegate;

+ (KMFileManager *)shareManager;

- (void)isWriteFilePathExit:(NSString *)writeFilePath;

/**
 将MP3数据保存至本地
 */
- (void)SaveMP3FileWithData:(NSMutableData *)data;

/**
 获取本地文件夹路径
 */
- (NSString *)createFilePath;

/**
 判断录音存储文件夹里的文件是否大于7天，如果大于7日，则删除
 */
//判断文件是否大于7天
- (void)registISCService;

- (void)PhoneNumberIsCallIn;

@end
