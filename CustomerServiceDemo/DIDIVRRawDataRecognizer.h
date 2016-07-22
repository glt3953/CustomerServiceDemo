//
//  DIDIVRRawDataRecognizer.h
//  DIDIVoiceRecognitionClient
//

#import <Foundation/Foundation.h>
#import "DIDIVoiceRecognitionClient.h"

@interface DIDIVRRawDataRecognizer : NSObject

@property (nonatomic) BOOL isStarted;
@property (nonatomic) NSInteger sampleRate;
@property (nonatomic, copy) NSArray* prop_list;
@property (nonatomic) NSInteger city_id;
@property (nonatomic, weak) id<MVoiceRecognitionClientDelegate> delegate;

/**
 * @brief 初始化识别器
 *
 * @param filePath 文件路径
 *
 * @param sampleRate 采样率
 *
 * @param property 识别模式
 */
- (id)initRecognizerWithSampleRate:(int)rate property:(TDDVoiceRecognitionProperty)property delegate:(id<MVoiceRecognitionClientDelegate>)delegate __attribute__((deprecated));

/**
 * @brief 初始化识别器
 *
 * @param rate 采样率
 * @param propList 识别属性数组
 * @param cid 城市ID
 *
 * @return 识别器实例
 */
- (id)initRecognizerWithSampleRate:(NSInteger)rate
                     propertyGroup:(NSArray*)propList
                            cityID:(NSInteger)cid
                          delegate:(id<MVoiceRecognitionClientDelegate>)delegate;

/**
 * @brief 开始识别
 *
 * @return 状态码，同[DIDIVoiceRecognitionClient startVoiceRecognition]
 */
- (int)startDataRecognition;

/**
 * @brief 向识别器发送数据
 *
 * @param data 发送的数据
 */
- (void)sendDataToRecognizer:(NSData *)data;

/**
 * @brief 数据发送完成
 */
- (void)allDataHasSent;

@end
