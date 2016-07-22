//
//  DIDIVRLogger.h
//  VoiceRecognitionClient
//
//  Created by  段弘 on 14-9-11.
//  Copyright (c) 2014年 DD, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#define DIDIVRLogDebug(fmt, ...) [DIDIVRLogger logDebug:[NSString stringWithFormat:@"%s:%d " fmt, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__]]
#define DIDIVRLogError(fmt, ...) [DIDIVRLogger logError:[NSString stringWithFormat:@"%s:%d " fmt, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__]]

typedef enum DDVRLogLevel {
    BDVR_LOG_OFF = 0,
    BDVR_LOG_ERROR = 1,
    BDVR_LOG_WARN = 2,
    BDVR_LOG_INFO = 3,
    BDVR_LOG_DEBUG = 4,
    BDVR_LOG_VERBOSE = 5,
} DDVRLogLevel;

@interface DIDIVRLogger : NSObject
/**
 * @brief 设置logLevel
 *
 * @param logLevel
 *            日志级别
 */
+ (void)setLogLevel:(DDVRLogLevel)logLevel;

/**
 * @brief 设置是否写日志文件
 *
 * @param logToFile
 *            是否写日志文件
 */
+ (void)logToFileSwitch:(BOOL)logToFile;

/**
 * @brief 当日志级别>=BDVR_LOG_DEBUG时输出日志
 *
 * @param logMessage
 *            日志信息
 */
+ (void)logDebug:(NSString *)logMessage;

/**
 * @brief 当日志级别>=BDVR_LOG_ERROR时输出日志
 *
 * @param logMessage
 *            日志信息
 */
+ (void)logError:(NSString *)logMessage;

@end
