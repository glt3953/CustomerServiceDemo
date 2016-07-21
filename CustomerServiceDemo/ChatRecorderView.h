//
//  ChatRecorderView.h
//  Jeans
//
//  Created by Jeans on 3/24/13.
//  Copyright (c) 2013 Jeans. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ChatRecorderView : UIView

@property (retain, nonatomic)  UIImageView * leftImageView;
@property (retain, nonatomic)  UIImageView * rightImageView;
@property (retain, nonatomic)  UIImageView *trashCanIV;
@property (retain, nonatomic)  UILabel * textView;

//还原界面
- (void)restoreDisplay;

- (void)setTextViewText:(NSString *)text;
//是否准备删除
- (void)prepareToDelete:(BOOL)_preareDelete;

//是否摇晃垃圾桶
- (void)rockTrashCan:(BOOL)_isTure;

//更新音频峰值
- (void)updateMetersByAvgPower:(float)_avgPower;

@end