#ifndef	_GETAUDIOBITMAP_H
#define	_GETAUDIOBITMAP_H
#include "inc.h"
#include "fft.h"

//#define USE_MIDI_TO_FREQ_MAP

#define	EnergyThresholdSpectral	200
#define	EnergyZeroBound	300

typedef struct 
{
	int center;
	int low;
	int high;
}SMidiFilter;

#ifdef _EXTERN_CALL_AUDIOBITMAP
#ifdef DLLEXPORT
#undef DLLEXPORT
#endif
#endif

#ifdef DLLEXPORT
class DLLEXPORT CAudioBitmap: CFft {
#else
class CAudioBitmap: CFft {
#endif
public:
	CAudioBitmap(int bitMapType, float lowfreq, float highfreq, int frameLen, int frameShiftLen, 
			int sampleRate, int blockLen, float threshold, bool m_bitOneFrameIndex, int frameOfindexUint);
	~CAudioBitmap();
private:
	INT16	m_bitMapType;
	INT16	m_byteSizePerFrame;
	INT16	m_bitSizePerUnit;//we use N bits as feature store unit, when dimension is >N, then multiple unit is used. 
				 //eg. m_bitSizePerUnit=16, then 16 bit element in one int16, if feature >16, multiple units are used
	bool	m_isSuccess;
	float	m_codingThreshold;
	double  m_thresholdArray[1997];
	INT16	*m_sample;
	float	*m_realFft;
	float	*m_spectrum; 	//store FFT spectrum
	// vector<SMidiFilter>	m_noteFreqList; //store corresponding note for each frequency // by jiangguohua
	// vector<float>	m_midiFreq; // by jiangguohua
	INT16	*m_freqNote; 	//store corresponding note for each frequency
	float	*m_chroma;
	UINT16	*m_bitmap; 	//store extracted bit stream for audio 
	UINT32 	m_frameNum;
	UINT32 	m_frameLen;
	UINT32 	m_frameShiftLen;
	UINT32 	m_chromaDim;
	UINT32 	m_sampleRate;
	float 	m_frameSecond;
	float 	m_frameShiftSecond;
	UINT32 	m_blockLen;
	UINT32 	m_sampleNum;
	UINT32	m_totalAudioNum;
	UINT32	m_totalFrameNum;
	UINT32 	m_sPos, m_ePos;
	UINT32	m_detectFrameNum;
	UINT32 	m_queryFrameNum; //frame number of query with shift
	// hash_map<int,vector<int> >	m_bitOneFrame; //store frames & bit-one position, key=frame // by jiangguohua
	bool 	m_bitOneFrameIndex; //if true, store one bit-one for each frame in m_bitOneFrame
	float 	m_lowFreq;
	float 	m_highFreq;
	INT16	m_midiNoteLow;
	INT16	m_midiNoteHigh;
	UINT32	m_frameOfindexUint;
	
	//online variables
	int PackID;//音乐包编号，从1开始，最后一个包的编号是-n
	unsigned int nFrameS;//从当前音乐包提取的bimap的起始帧位置，从0开始
	int nFrameE;//从当前音乐包提取的bimap的末尾帧位置
//	int nRecSample;//当前已获取到的总样本数量
	unsigned int nRecFrameS;//从当前音乐包提取的fft特征的起始帧位置，从0开始
	int nRecFrameE;//从当前音乐包提取的fft特征的末尾帧位置
	float *m_totalu_forward;//保存当前时刻，前向统计的特征均值
	float *m_totalsigma_forward;//保存当前时刻，前向统计的特征方差
	
private:
	int		Freq2Midinote(float freq);
	float Midinote2Freq(int midinote);
	void 	FreqMidinoteMap(void);
	void 	MidinoteFreqMap(void);
	bool 	ReadWaveAudio(char* wavefile);
	bool 	ReadPcmAudio(char* wavefile);
	bool 	ReadFftAudio(char* wavefile);
	int		GetWAVHeaderInfo(FILE *f);
	// void 	SaveBitmapHeader(ofstream&); // by jiangguohua
	bool 	SaveBitmap(char *outfile);
	void	ComputeFft(short *data, int datalen);
	void 	ComputeBitmap() ;
	void	ComputeBitmapFft();
	void	ComputeBitmapMidi();
	void 	CheckSilence();
	void 	ResetVar();
	void 	ComputeBitmapShift(int shift);
public:
	bool 	IsSuccess();
	//for batch extraction of a list of audio files in wavefile
	//bool 	ExtractBitmap(char* wavefileList, char* wavpath, char* filetype, char* savepath); 
	//bool 	ExtractBitmap(char* wavefile, char*audiotype);	//extract single file
	bool 	ExtractBitmap_SingleFile(char *wavfile, char *wavpath, char *audiotype, char *savepath);
	UINT16* GetBitmap(int &frameNum); 			//get bitmap just extracted
	// void	GetBitmap(hash_map<int, vector<int> > &frameBitOne, int &frameNum); // by jiangguohua
	UINT16* GetBitmapShift(int &frameNum, int shift); 	//get bitmap just extracted
	// hash_map<int,vector<int> >*  GetBitmapCompress(int &frameNum); // by jiangguohua
	bool	IsBitOneCompress() {return m_bitOneFrameIndex;}

	//online functions
	int	ExtractBitmap_Online(int PID, int ibandwidth);
	int	ComputeBitmapFft_Online(int ibandwidth);
	void	ComputeFft_Online(short *data, int datalen);
	int	GetPackBitmapPos(int &frameFirst, int &frameLast, unsigned long threadID);
	int	SetPack(int PID);
	int	GetPackID();
	short*	GetDataPtr();
	void	SetDataNum(UINT32 nSampleNum);
//	int GetReceivedSampleNum();

};

#endif

