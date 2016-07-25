#ifndef _INC_H
#define	_INC_H

#define _USE_MATH_DEFINES //use constant defined in math.h
#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<string.h>
#include	<float.h>
//#include	<conio.h>
#include	<assert.h>
// #include	<iostream> // by jiangguohua
// #include	<fstream> // by jiangguohua
#include	<string.h>
// #include	<vector> // by jiangguohua
// #include	<list> // by jiangguohua
// #include	<bitset> // by jiangguohua
// #include 	<algorithm> // by jiangguohua
// #include 	<ext/hash_map> // by jiangguohua
#include 	<time.h>
// #include 	<ext/hash_set> // by jiangguohua


#define		UINT8	unsigned char
#define		UINT16	unsigned short
#define		UINT32	unsigned int
#define		UINT64	unsigned long
#define		INT8	char
#define		INT16	short
#define		INT32	int
#define		INT64	long

#define	DEFAULT_CHROMA_DIMENSION 12
#define	MAX_SPEECH_FRAME_LEN	312	//20s
//#define LOW_BAND_FREQ	100//0//50
//#define HIGH_BAND_FREQ 8000//high frequence seg not useful
#define	MAX_FILE_NAME_LEN 512
#define LOW_MIDI_NOTE	1
#define HIGH_MIDI_NOTE	119


//#define DLLEXPORT __declspec(dllexport)
// using namespace std; // by jiangguohua
// using namespace __gnu_cxx; // by jiangguohua

typedef enum
{
	getChromaBit, 
	getFftBit, 
	getMidiBit
}bitMapTypeConst;

#endif

