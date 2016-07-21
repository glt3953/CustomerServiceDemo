//
//  CreatePCMheaderToWav.h
//  DIDIVRClientSample
//
//  Created by Joseph on 16/3/26.
//  Copyright © 2016年 DIDI. All rights reserved.
//

#ifndef CreatePCMheaderToWav_h
#define CreatePCMheaderToWav_h

#include <stdio.h>

void *createWaveHeader(int fileLength, short channel, int sampleRate, short bitPerSample);

#endif /* CreatePCMheaderToWav_h */
