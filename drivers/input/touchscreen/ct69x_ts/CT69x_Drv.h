/**************************************************************************
*  CT69x_Drv.h
* 
*  CT69x Driver code version 1.0
* 
*  Create Date : 2012/06/25
* 
*  Modify Date : 
*
*  Create by   : wuhaijun
* 
**************************************************************************/

#ifndef CT69X_DRV_H

#define CT69X_DRV_H

#define Release_Ver		302


#define	MAX_POINT					5

#define NUM_TX						26		// TX number of TOUCH IC
#define NUM_RX						14	  	// RX number of TOUCH IC

#define DEBUG_BASE		0x01
#define DEBUG_DIFF		0x02
#define DEBUG_PROXIMITY	0x04
#define DEBUG_PEAK		0x08
#define DEBUG_ESD		0x10
#define DEBUG_ID		0x20
#define DEBUG_POINT		0x40
#define DEBUG_FILTER	0x80

#define ABS(X)                  ((X > 0) ? (X) : -(X))


typedef enum{
	RawDataMode = 0,
	DeltaMode,
	MonitorMode,
	DeltaRawMode
}enumWorkMode;

typedef enum{
	BASE_INITIAL,
	BASE_FAST_TRACE,
	BASE_STABLE,
	TEMP_DRIFT
} CompensateMode;

typedef struct {
	unsigned short  Base[NUM_TX][NUM_RX];	
	unsigned short	ChipBase[NUM_TX][NUM_RX];
	signed char   Flag[NUM_TX][NUM_RX];	
	signed char		BaseCnt[NUM_TX][NUM_RX];
	unsigned char   CompensateFlag;    			
	unsigned char   TraceTempIncCnt;   			
	unsigned char   TraceTempDecCnt;   			
	unsigned char   CompensateStateFrameCnt;		
	short 	        LastMaxDiff;              		
	CompensateMode  CompensateState;  
	unsigned int	InitialFrameCnt;
	unsigned char	PosBigAreaTouchFlag;	
	unsigned char	NegBigAreaTouchFlag;
	unsigned char	BigAreaFirstFlag;				
	unsigned char	BigAreaChangeFlag;	
	unsigned short	BigTouchFrame;				
	unsigned short	FrameCnt;
	unsigned short	LongStableCnt;
	unsigned char   PosPeakCnt;							
	unsigned char   NegPeakCnt;	
	unsigned char   NegPeakInvalid;	
	unsigned char 	PeakCheckFrameCnt;
	unsigned char	BaseFrozen;
	unsigned char   PosPeakCompensateCnt[MAX_POINT];
	unsigned char   NegPeakCompensateCnt[MAX_POINT];
	unsigned char	BaseFreshFlag;
	unsigned char	BaseFreshCnt;
	unsigned char	Proximity_Diff;
	unsigned char	Proximity_Touch;
	unsigned char	Proximity;
}STRUCTBASE;

typedef struct {
	unsigned char   Peak[MAX_POINT][2];		
	unsigned char 	LastPeak[MAX_POINT][2];	
	unsigned char   NegPeak[MAX_POINT][2];	
	unsigned char   CurrentPointNum;						
	unsigned char   CurrentNegPointNum;					
	unsigned char   LastPointNum;
}STRUCTPEAK;

typedef	struct {
	unsigned short	X,Y;						// X,Y coordinate
	unsigned char	PointID;			// Assigned point ID
	unsigned char	Event;				// Event of current point
}STRUCTPOINT;

typedef	struct {
	STRUCTPOINT 	PointList[16];
	short	PointDeltaX[16];
	short	PointDeltaY[16];
}STRUCTPOINTBUFF;

typedef	struct {
	STRUCTPOINT 	PointInfo[MAX_POINT];
	STRUCTPOINT		RptPoint[MAX_POINT];
	unsigned char 	PointNum;
	unsigned char 	LastPointNum;
	unsigned char 	NegPointNum;
	unsigned char	FilterPointCnt;
	unsigned char 	FirstLiftUpFlag;
	unsigned char 	TouchStatus;
	unsigned char	PointHoldCnt[MAX_POINT];
	unsigned char	PointPressCnt[MAX_POINT];

}STRUCTFRAME;

typedef struct {
	unsigned char fileflag[14];
	unsigned char TXOFFSET[(NUM_TX+1)/2];
	unsigned char RXOFFSET[(NUM_RX+1)/2];
	unsigned char TXCAC[NUM_TX];
	unsigned char RXCAC[NUM_RX];
	unsigned char TXGAIN[NUM_TX];
	unsigned char TXCC[(NUM_TX+1)/2];
	unsigned char RXCC[(NUM_RX+1)/2];
	short SOFTOFFSET[NUM_TX][NUM_RX];
}STRUCTCALI;

typedef struct{
	unsigned char BaseRegulate;
	unsigned char BaseChecked;
	unsigned char FirstPosPeakFlag;
	unsigned char FirstNegPeakFlag;
	unsigned char PosAndNegPeakFlag;
	unsigned char DeltaDiffCnt;
	unsigned char BaseBadCnt;
	unsigned short BaseGoodCnt;
	unsigned short NoTouchCnt;
	unsigned short FrameCnt;
}STURCTOFFSETCHECK;


#define NOISE_LISTENING 0
#define NOISE_SCAN	1
#define NOISE_FREQ_JUMP	2
#define NOISE_SEEK_FAIL	3

#define NOISE_FRM_NORMAL	0
#define NOISE_FRM_PRE_MEASURE	1
#define NOISE_FRM_MEASURE	2

typedef struct {
	unsigned char AllFrmCnt;	// Frame counter to generate noise meaure frame indicator
	unsigned char NoiseFrmCnt;	// Frame counter for noise level checking
	unsigned char IdleFrmCnt;	// No touch frame counter
	unsigned char State;		// Noise checking state: LISTENING, SCAN, JUMP
	unsigned char FrmState;		// Frame type indicator: PRE_MEAUSRE, MEAUSRE, NORMAL
	short	NoiseNormal;		// Noise in working freq
	short	NoiseScan;		// Noise in scan freq
	short	Better_NoiseScan;   //pfx:smaller Noise in Scan freq
	unsigned char Better_ScanFreqID;	//pfx:the Scan Freq for the smaller Noise
	unsigned char ScanFreqID;	// Scan freq ID
	unsigned char WorkFreqID;	// Current freq ID
	short	NoiseTh1;		// Diff threshold for noise too high judgement
	char	JumpTh;			// frame number threshold for freq jumping
	char	FailedFreqList [32];	// Searched freq indicator for freq scanning
}STRUCTNOISE;


void CT69x_TP_Init(void);
void CT69x_TP_Reinit(void);
void CT69x_Sleep(void);
char CT69x_TouchProcess(void);
void CT69x_ChargeMode(char mode);
void CT69x_BatteryMode(char mode);
unsigned char CT69x_GetPointNum(void);
unsigned char CT69x_GetPeakNum(void);
char CT69x_GetPoint(int *x,int *y, int *id, int *event,char Index);
void CT69x_GetBase(unsigned short *data, char x,char y);
void CT69x_GetDiff(short *data, char x,char y);
char CT69x_GetPeak(unsigned char *x,unsigned char *y,unsigned char Index);
char CT69x_GetNegPeak(unsigned char *x,unsigned char *y,unsigned char Index);
char CT69x_GetCalcPoint(unsigned short *x,unsigned short *y,unsigned char Index);
char CT69x_CLB(void);
void CT69x_CLB_GetCfg(void);
void CT69x_CLB_WriteCfg(void);
void TP_Force_Calibration(void);
void FreqScan(unsigned char BaseFreq);
char CT69x_TP_Test(void);

char  CheckBaseRegularity(void);

#endif
