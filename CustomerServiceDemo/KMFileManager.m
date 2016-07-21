//
//  KMFileManager.m
//  DIDIVRClientSample
//
//  Created by Joseph on 16/3/25.
//  Copyright © 2016年 DIDI. All rights reserved.
//


#import "KMFileManager.h"
#import "lame.h"
#import "CreatePCMheaderToWav.h"
#import <CoreTelephony/CTCallCenter.h>
#import <CoreTelephony/CTCall.h>
#import <UIKit/UIKit.h>
#import "Log.h"

#define DefaultSubPath @"RecordVoice"

@implementation KMFileManager
static KMFileManager * _instance = nil;
static NSUInteger secondsOfSevenDays = 60*60*24*7;

+ (KMFileManager *)shareManager {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
         _instance = [[self alloc]init];
    });
    return _instance;
}

- (void)SaveMP3FileWithData:(NSMutableData *)data {
    [self createFilePath];
    NSArray *dirPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docsDir = [dirPaths objectAtIndex:0];
    //录音文件名采用时间标记 例如"2015-01-06_12:41"
    NSString *WavfileName = [self createFilename];
    
    NSString *Mp3FileName = WavfileName;
//    self.filename = @"RecordingFile";
    NSString *WavsoundFilePath = [docsDir
                               stringByAppendingPathComponent:[NSString stringWithFormat:@"%@/%@.wav", DefaultSubPath, WavfileName]];
    NSString *Mp3soundFilePath = [docsDir stringByAppendingPathComponent:[NSString stringWithFormat:@"%@/%@.mp3", DefaultSubPath, Mp3FileName]];
    [self isWriteFilePathExit:WavsoundFilePath];
    [self isWriteFilePathExit:Mp3soundFilePath];
    
    NSMutableData * wavDatas = [[NSMutableData alloc] init];
    
    int fileLength = data.length;
    
    void *header = createWaveHeader(fileLength, 1, 16000, 16);
    
    [wavDatas appendBytes:header length:44];
    [wavDatas appendData:data];
    BOOL writeWavDataSuccess = [wavDatas writeToFile:WavsoundFilePath atomically:YES];
    LogDebug(@"WavFileName----:%@",WavsoundFilePath);
    if (writeWavDataSuccess == YES) {
        @try {
            int read, write;
            
            FILE *pcm = fopen([WavsoundFilePath cStringUsingEncoding:1], "rb");  //source 被转换的音频文件位置
            fseek(pcm, 4*1024, SEEK_CUR);                                   //skip file header
            FILE *mp3 = fopen([Mp3soundFilePath cStringUsingEncoding:1], "wb");  //output 输出生成的Mp3文件位置
            
            
            
            const int PCM_SIZE = 8192;
            const int MP3_SIZE = 8192;
            short int pcm_buffer[PCM_SIZE*2];
            unsigned char mp3_buffer[MP3_SIZE];
            
            lame_t lame = lame_init();
            lame_set_in_samplerate(lame, 8000);
            lame_set_VBR(lame, vbr_default);
            lame_init_params(lame);
            
            do {
                read = fread(pcm_buffer, 2 * sizeof(short int), PCM_SIZE, pcm);
                if (read == 0) {
                    write = lame_encode_flush(lame, mp3_buffer, MP3_SIZE);
                } else {
                    write = lame_encode_buffer_interleaved(lame, pcm_buffer, read, mp3_buffer, MP3_SIZE);
                }
                
                fwrite(mp3_buffer, write, 1, mp3);
            } while (read != 0);
            
            lame_close(lame);
            fclose(mp3);
            fclose(pcm);
        }
        @catch (NSException *exception) {
            LogDebug(@"%@", [exception description]);
        }
        @finally {
            [self isWriteFilePathExit:WavsoundFilePath];
            if ([self.delegate respondsToSelector:@selector(KMFileManagergetFilePath:AndFileName:)]) {
                [self.delegate KMFileManagergetFilePath:Mp3soundFilePath AndFileName:Mp3FileName];
            }
        }
    }
}

- (void)registISCService {
    NSString *ALLPath = [self createFilePath];
    NSString *path = [[NSString alloc] init];
    NSFileManager *myFileManager = [NSFileManager defaultManager];
    
    NSDirectoryEnumerator *myDirectoryEnumerator;
    
    myDirectoryEnumerator = [myFileManager enumeratorAtPath:path];
    
    while((path = [myDirectoryEnumerator nextObject]) != nil) {
        NSDictionary *fileAttributes = [myFileManager fileAttributesAtPath:[ALLPath stringByAppendingPathComponent:path] traverseLink:YES];
        if (fileAttributes != nil) {
            NSString *creationDate;
            //NSString *NSFileCreationDate
            
            //文件创建日期
            if ((creationDate = [fileAttributes objectForKey:NSFileCreationDate])) {
//                LogDebug(@"File creationDate: %@\n", creationDate);
                
                NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
                //设定时间格式,
                
                [dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
                
                NSDate *modificationDate = (NSDate *)[fileAttributes objectForKey:NSFileModificationDate];
                NSString *B = [dateFormatter stringFromDate: modificationDate];
                
                NSDate *date1 = [NSDate date];
                
                date1 = [date1 dateByAddingTimeInterval:-(secondsOfSevenDays)];
                
                if ([self timeDifference:[dateFormatter dateFromString:B]] > secondsOfSevenDays) {
                    //大于七天
                    LogDebug(@"在七天外");
                    if([myFileManager removeItemAtPath:[ALLPath stringByAppendingPathComponent:path] error:nil] == YES) {
                        LogDebug(@"success");
                    }
                }
                if ([self timeDifference:[dateFormatter dateFromString:B]] < secondsOfSevenDays) {
                    //小于七天
                    LogDebug(@"在七天内");
                }
                if ([self timeDifference:[dateFormatter dateFromString:B]] == secondsOfSevenDays) {
                    //刚好七天
                    LogDebug(@"ping deng");
                }
            }
        }
    }
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(PhoneNumberIsCallIn) name:UIApplicationWillResignActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(PhoneNumberIsCallIn) name:UIApplicationDidEnterBackgroundNotification object:nil];
}

//创建录音文件名字
- (NSString *)createFilename {
    NSDate *date_ = [NSDate date];
    NSDateFormatter *dateformater = [[NSDateFormatter alloc] init];
    [dateformater setDateFormat:@"yyyy_MM_dd_HH_mm_ss"];
    NSString *timeFileName = [dateformater stringFromDate:date_];
    
    return timeFileName;
}

//创建存储路径
- (NSString *)createFilePath {
    NSArray *dirPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docsDir = [dirPaths objectAtIndex:0];
    NSString *savedPath = [docsDir stringByAppendingPathComponent:DefaultSubPath];
    BOOL isDir = NO;
    NSFileManager *fileManager = [NSFileManager defaultManager];
    BOOL existed = [fileManager fileExistsAtPath:savedPath isDirectory:&isDir];
    if (!(isDir == YES && existed == YES)) {
        [fileManager createDirectoryAtPath:savedPath withIntermediateDirectories:YES attributes:nil error:nil];
    }
    
    return savedPath;
}

- (void)isWriteFilePathExit:(NSString *)writeFilePath {
    BOOL isExist = [[NSFileManager defaultManager] fileExistsAtPath:writeFilePath];
    if (isExist) {
        //如果存在则移除，防止文件冲突
        NSError *err = nil;
        [[NSFileManager defaultManager] removeItemAtPath:writeFilePath error:&err];
    }
}

//返回与现在时间的时间差
- (long)timeDifference:(NSDate *)date {
    NSDate *localeDate = [NSDate date];
    long difference = fabs([localeDate timeIntervalSinceDate:date]);
    return difference;
}

//对比两个Date的时间是否一致
- (int)compareOneDay:(NSDate *)oneDay withAnotherDay:(NSDate *)anotherDay {
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
    NSString *oneDayStr = [dateFormatter stringFromDate:oneDay];
    NSString *anotherDayStr = [dateFormatter stringFromDate:anotherDay];
    NSDate *dateA = [dateFormatter dateFromString:oneDayStr];
    NSDate *dateB = [dateFormatter dateFromString:anotherDayStr];
    NSComparisonResult result = [dateA compare:dateB];
    LogDebug(@"date1 : %@, date2 : %@", oneDay, anotherDay);
    if (result == NSOrderedDescending) {
        //oneDay晚于anotherDay
        LogDebug(@"Date1 is in the future");
        return 1;
    } else if (result == NSOrderedAscending) {
        //oneDay早于anotherDay
        //LogDebug(@"Date1 is in the past");
        return -1;
    }
    //LogDebug(@"Both dates are the same");
    return 0;
}

- (void)checkFileIsOverToSevenDays {
    [self createFilePath];
}

- (void)PhoneNumberIsCallIn {
    LogDebug(@"%@", @"我被调用了～～～～～～～～～");
    [[NSNotificationCenter defaultCenter] postNotificationName:@"ONEKFOnlineIsPhoneCallInNotication" object:nil];
}

@end
