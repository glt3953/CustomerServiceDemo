//
//  ChatRecorderView.m
//  Jeans
//
//  Created by Jeans on 3/24/13.
//  Copyright (c) 2013 Jeans. All rights reserved.
//

#import "ChatRecorderView.h"
#import "Log.h"

@interface ChatRecorderView() {
    NSMutableArray *peakImageLeftAry;
    NSMutableArray *peakImageRightAry;
}

@end

@implementation ChatRecorderView

- (BOOL)isValidString:(NSString *)aString {
    if ([aString isKindOfClass:[NSString class]] && [aString length] > 0) {
        return YES;
    } else {
        return NO;
    }
}

- (NSString *)oneimage_path:(NSString *)name {
    NSString *dir = @"ISCProject.bundle/";
    if (![self isValidString:name]) {
//        LogBreakpoint(@"图片名非法 [%@]", name);
        return nil;
    }
    NSString *path = [NSString stringWithFormat:@"%@/%@", dir, name];
    
    return path;
}

- (UIImage *)GetimageNamed:(NSString *)name {
//    UIImage *image = [UIImage imageNamed:[self oneimage_path:name]];
    UIImage * image = [UIImage imageNamed:name];
    return image;
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self initilization];
    }
    
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if (self) {
        [self initilization];
    }
    
    return self;
}

- (void)initilization {
    //初始化音量peak峰值图片数组
//    isTrashCanRocking = YES;
    peakImageLeftAry = [[NSMutableArray alloc] initWithCapacity:2];
    peakImageRightAry = [[NSMutableArray alloc] initWithCapacity:2];

    for (int i = 1; i <= 5; i++) {
        [peakImageLeftAry addObject:[self GetimageNamed:[NSString stringWithFormat:@"leftNow%d.png", i]]];
        [peakImageRightAry addObject:[self GetimageNamed:[NSString stringWithFormat:@"rightNow%d.png", i]]];
    }
    
    LogDebug(@" peakImageAry ==%zd", [peakImageLeftAry count]);
    
    //---------------------------
    UIImage *image  = [self GetimageNamed:@"voiceBg"];
    UIImageView *BgImageView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 200, 170)];
    BgImageView.image = image;
    
    UIImage *micImg = [self GetimageNamed:@"mic"];
    UIImageView *micImageView = [[UIImageView alloc] initWithFrame:CGRectMake((200 - micImg.size.width/2)/2, 19 + 17, micImg.size.width/2, micImg.size.height/2)];
    micImageView.image = micImg;
    
    UIImage *leftimage = [self GetimageNamed:@"leftNow1"];
    self.leftImageView = [[UIImageView alloc] initWithFrame:CGRectMake(micImageView.frame.origin.x - leftimage.size.width/2  - 10 , 28 + 15, leftimage.size.width/2, leftimage.size.height/2)];
    self.leftImageView.image = leftimage;
    
    UIImage *rightimage = [self GetimageNamed:@"rightNow1"];
    self.rightImageView = [[UIImageView alloc] initWithFrame:CGRectMake(micImageView.frame.origin.x + micImageView.frame.size.width + 10 , 28 + 15, rightimage.size.width/2, rightimage.size.height/2)];
    self.rightImageView.image = rightimage;
    [self addSubview:BgImageView];
    [BgImageView addSubview:self.leftImageView];
    [BgImageView addSubview:self.rightImageView];
    [BgImageView addSubview:micImageView];
    
    self.textView = [[UILabel alloc] initWithFrame:CGRectMake(16, 8 +20 + 10 + 15+ micImageView.frame.size.height, BgImageView.frame.size.width - 32, 60)];
    self.textView.backgroundColor = [UIColor clearColor];
    self.textView.numberOfLines = 2;
    //    textView.editable = NO;
    self.textView.font = [UIFont systemFontOfSize:14];
    self.textView.textColor = [UIColor whiteColor];
    self.textView.textAlignment = NSTextAlignmentCenter;
    self.textView.text = @"";
    [BgImageView addSubview:self.textView];

    //------------------------
//    self.backgroundColor=[UIColor colorWithPatternImage:[self GetimageNamed:@"voiceBg.png"]];
 
//    UIImageView *phoneIV=[[UIImageView alloc]initWithFrame:CGRectMake(30, 10, 36, 100)];
//    phoneIV.image=[self GetimageNamed:@"micBg"];
//    [self addSubview:phoneIV];
    
    
//    _peakMeterIV=[[UIImageView alloc]initWithFrame:CGRectMake(76,48, 18, 61)];
//    _peakMeterIV.image=[self GetimageNamed:@"line1"];
//    [self addSubview:_peakMeterIV];
}

- (void)setTextViewText:(NSString *)text {
    if (text.length > 24) {
        NSRange range = NSMakeRange(text.length - 24, 24);
        NSString *substr = [text substringWithRange:range];
        self.textView.text = substr;
    } else {
        self.textView.text = text;
    }
}

- (void)dealloc {
}

#pragma mark -还原显示界面
- (void)restoreDisplay {
    //停止震动
//    [self rockTrashCan:NO];
    //还原倒计时文本
    self.textView.text = @"请按住说话";
}

#pragma mark - 是否准备删除
//- (void)prepareToDelete:(BOOL)_preareDelete{
//    if (_preareDelete != isPrepareDelete) {
//        isPrepareDelete = _preareDelete;
////        [self rockTrashCan:isPrepareDelete];
//    }
//}
#pragma mark - 是否摇晃垃圾桶
//- (void)rockTrashCan:(BOOL)_isTure{
//    if (_isTure != isTrashCanRocking) {
//        isTrashCanRocking = _isTure;
//        if (isTrashCanRocking) {
//            //摇晃
//            _trashCanIV.animationImages = trashImageAry;
//            _trashCanIV.animationRepeatCount = 0;
//            _trashCanIV.animationDuration = 1;
//            [_trashCanIV startAnimating];
//        }else{
//            //停止
//            if (_trashCanIV.isAnimating)
//                [_trashCanIV stopAnimating];
//            _trashCanIV.animationImages = nil;
//            _trashCanIV.image = kTrashImage1;
//        }
//    }
//}

#pragma mark - 更新音频峰值
- (void)updateMetersByAvgPower:(float)lowPassResults {
    if (0 < lowPassResults <= 0.06) {
        [self.leftImageView setImage:[self GetimageNamed:@"leftNow1.png"]];
        [self.rightImageView setImage:[self GetimageNamed:@"rightNow1.png"]];
    } else if (0.06 < lowPassResults <= 0.13) {
        [self.leftImageView setImage:[self GetimageNamed:@"leftNow1.png"]];
        [self.rightImageView setImage:[self GetimageNamed:@"rightNow1.png"]];
    } else if (0.13 < lowPassResults <= 0.20) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:1]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:1]];
    } else if (0.20 < lowPassResults <= 0.27) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:1]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:1]];
    } else if (0.27 < lowPassResults <= 0.34) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:2]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:2]];
    } else if (0.34 < lowPassResults <= 0.41) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:2]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:2]];
    } else if (0.41 < lowPassResults <= 0.48) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:2]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:2]];
    } else if (0.48 < lowPassResults <= 0.55) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:3]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:3]];
    } else if (0.55 < lowPassResults <= 0.62) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:3]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:3]];
    } else if (0.62 < lowPassResults <= 0.69) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:3]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:3]];
    } else if (0.69 < lowPassResults <= 0.76) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:4]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:4]];
    } else if (0.76 < lowPassResults <= 0.83) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:4]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:4]];
    } else if (0.83 < lowPassResults <= 0.9) {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:4]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:4]];
    } else {
        [self.leftImageView setImage:[peakImageLeftAry objectAtIndex:4]];
        [self.rightImageView setImage:[peakImageRightAry objectAtIndex:4]];
    }
}

@end