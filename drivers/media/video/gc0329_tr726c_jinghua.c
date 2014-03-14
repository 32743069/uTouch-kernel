
#include "generic_sensor.h"

/*
*      Driver Version Note
*v0.0.1: this driver is compatible with generic_sensor
*v0.0.3:
*        add sensor_focus_af_const_pause_usr_cb;
*/
static int version = KERNEL_VERSION(0,0,3);
module_param(version, int, S_IRUGO);


static int debug;
module_param(debug, int, S_IRUGO|S_IWUSR);

#define dprintk(level, fmt, arg...) do {			\
	if (debug >= level) 					\
	printk(KERN_WARNING fmt , ## arg); } while (0)

/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_GC0329
#define SENSOR_V4L2_IDENT V4L2_IDENT_GC0329
#define SENSOR_ID 0xc0
#define GC0329_12M_MCLK

#define SENSOR_BUS_PARAM					 (SOCAM_MASTER |\
											 SOCAM_PCLK_SAMPLE_RISING|SOCAM_HSYNC_ACTIVE_HIGH| SOCAM_VSYNC_ACTIVE_HIGH|\
											 SOCAM_DATA_ACTIVE_HIGH | SOCAM_DATAWIDTH_8  |SOCAM_MCLK_24MHZ)
#define SENSOR_PREVIEW_W					 640
#define SENSOR_PREVIEW_H					 480
#define SENSOR_PREVIEW_FPS					 15000	   // 15fps 
#define SENSOR_FULLRES_L_FPS				 7500	   // 7.5fps
#define SENSOR_FULLRES_H_FPS				 7500	   // 7.5fps
#define SENSOR_720P_FPS 					 0
#define SENSOR_1080P_FPS					 0

#define SENSOR_REGISTER_LEN 				 1		   // sensor register address bytes
#define SENSOR_VALUE_LEN					 1		   // sensor register value bytes
static unsigned int SensorConfiguration = (CFG_WhiteBalance|CFG_Effect|CFG_Scene);	
static unsigned int SensorChipID[] = {SENSOR_ID};
/* Sensor Driver Configuration End */


#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a,b) CONS4(SensorReg,SENSOR_REGISTER_LEN,Val,SENSOR_VALUE_LEN)(a,b)
#define sensor_write(client,reg,v) CONS4(sensor_write_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_read(client,reg,v) CONS4(sensor_read_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_write_array generic_sensor_write_array

struct sensor_parameter
{
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;
	unsigned short int preview_line_width;
	unsigned short int preview_gain;

	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};

struct specific_sensor{
	struct generic_sensor common_sensor;
	//define user data below
	struct sensor_parameter parameter;

};

/*
*  The follow setting need been filled.
*  
*  Must Filled:
*  sensor_init_data :				Sensor initial setting;
*  sensor_fullres_lowfps_data : 	Sensor full resolution setting with best auality, recommand for video;
*  sensor_preview_data :			Sensor preview resolution setting, recommand it is vga or svga;
*  sensor_softreset_data :			Sensor software reset register;
*  sensor_check_id_data :			Sensir chip id register;
*
*  Optional filled:
*  sensor_fullres_highfps_data: 	Sensor full resolution setting with high framerate, recommand for video;
*  sensor_720p: 					Sensor 720p setting, it is for video;
*  sensor_1080p:					Sensor 1080p setting, it is for video;
*
*  :::::WARNING:::::
*  The SensorEnd which is the setting end flag must be filled int the last of each setting;
*/

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] ={
	// TODO: add initial code here
	  {0xfe, 0x80},
	{0xfe, 0x80},
	{0xfc, 0x16},
	{0xfc, 0x16},
	{0xfc, 0x16},
	{0xfc, 0x16},
	{0xfe, 0x00},
	{0xf0, 0x07},
	{0xf1, 0x01},
	
#ifdef GC0329_12M_MCLK
	{0xfa,0x11},	   
#else
	{0xfa,0x00},	   
#endif
	{0x73, 0x90},
	{0x74, 0x80},
	{0x75, 0x80},
	{0x76, 0x94},

	{0x42, 0x00},
	{0x77, 0x57},
	{0x78, 0x4d},
	{0x79, 0x45},
	//{0x42, 0xfc},

#ifdef GC0329_12M_MCLK
	{0x05, 0x01},
	{0x06, 0x32}, 
	{0x07, 0x00},
	{0x08, 0x70},
	
	{0xfe, 0x01},
	{0x29, 0x00},   //anti-flicker step [11:8]
	{0x2a, 0x3c},   //anti-flicker step [7:0]
	
	{0x2b, 0x02},   //exp level 0  14.28fps
	{0x2c, 0x58}, 
	{0x2d, 0x02},   //exp level 1  12.50fps
	{0x2e, 0xd0}, 
	{0x2f, 0x03},   //exp level 2  10.00fps
	{0x30, 0x84}, 
	{0x31, 0x04},   //exp level 3  7.14fps
	{0x32, 0x38}, 
	{0x33, 0x20}, 
	{0xfe, 0x00},
#else
	{0x05, 0x02},
	{0x06, 0x2c}, 
	{0x07, 0x00},
	{0x08, 0xb8},
	
	{0xfe, 0x01},
	{0x29, 0x00},   //anti-flicker step [11:8]
	{0x2a, 0x60},   //anti-flicker step [7:0]
	
	{0x2b, 0x02},   //exp level 0  14.28fps
	{0x2c, 0xa0}, 
	{0x2d, 0x03},   //exp level 1  12.50fps
	{0x2e, 0x00}, 
	{0x2f, 0x03},   //exp level 2  10.00fps
	{0x30, 0xc0}, 
	{0x31, 0x05},   //exp level 3  7.14fps
	{0x32, 0x40}, 
	{0x33, 0x20}, 
	{0xfe, 0x00},
#endif
	////////////////////analog////////////////////
	{0x0a, 0x02},
	{0x0c, 0x02},
	{0x17, 0x14},
	{0x19, 0x05},
	{0x1b, 0x24},
	{0x1c, 0x04},
	{0x1e, 0x08},
	{0x1f, 0xc8},  //  c0 20120720	 
	{0x20, 0x00},
	{0x21, 0x48},
	{0x22, 0xba}, 
	{0x23, 0x22},
	{0x24, 0x16},

	////////////////////blk////////////////////
	{0x26, 0xf7}, 
	{0x28, 0x7f},// 20130819 lanking 
	{0x29, 0x80}, 
	{0x32, 0x04},
	{0x33, 0x20},
	{0x34, 0x20},
	{0x35, 0x20},
	{0x36, 0x20},

	////////////////////ISP BLOCK ENABL////////////////////
	{0x40, 0xff},
	{0x41, 0x44},
	{0x42, 0x7e},
	{0x44, 0xa2},  /// yuv order
	{0x46, 0x03},  /// vsync
	{0x4b, 0xca},
	{0x4d, 0x01},
	{0x4f, 0x01},
	{0x70, 0x48},

	//{0xb0, 0x00},
	//{0xbc, 0x00},
	//{0xbd, 0x00},
	//{0xbe, 0x00},
	////////////////////DNDD////////////////////
	{0x80, 0xe7}, 
	{0x82, 0x7f},//55 cyrille 
	{0x83, 0x03}, 
	{0x87, 0x4a},

	////////////////////INTPEE////////////////////
	{0x95, 0x00},//45 cyrille

	////////////////////ASDE////////////////////
	//{0xfe, 0x01},
	//{0x18, 0x22},
	//{0xfe, 0x00},
	//{0x9c, 0x0a},
	//{0xa0, 0xaf},
	//{0xa2, 0xff},
	//{0xa4, 0x50},
	//{0xa5, 0x21},
	//{0xa7, 0x35},

	////////////////////RGB gamma////////////////////
	//RGB gamma m4'
	{0xBF, 0x0E},
	{0xc0, 0x1C},
	{0xc1, 0x34},
	{0xc2, 0x48},
	{0xc3, 0x5A},
	{0xc4, 0x6B},
	{0xc5, 0x7B},
	{0xc6, 0x95},
	{0xc7, 0xAB},
	{0xc8, 0xBF},
	{0xc9, 0xCE},
	{0xcA, 0xD9},
	{0xcB, 0xE4},
	{0xcC, 0xEC},
	{0xcD, 0xF7},
	{0xcE, 0xFD},
	{0xcF, 0xFF},



#if 0
	case GC0329_RGB_Gamma_m1:		//smallest gamma curve
	{0xfe, 0x00},
	{0xbf, 0x06},
	{0xc0, 0x12},
	{0xc1, 0x22},
	{0xc2, 0x35},
	{0xc3, 0x4b},
	{0xc4, 0x5f},
	{0xc5, 0x72},
	{0xc6, 0x8d},
	{0xc7, 0xa4},
	{0xc8, 0xb8},
	{0xc9, 0xc8},
	{0xca, 0xd4},
	{0xcb, 0xde},
	{0xcc, 0xe6},
	{0xcd, 0xf1},
	{0xce, 0xf8},
	{0xcf, 0xfd},

	case GC0329_RGB_Gamma_m2:
	{0xBF, 0x08},
	{0xc0, 0x0F},
	{0xc1, 0x21},
	{0xc2, 0x32},
	{0xc3, 0x43},
	{0xc4, 0x50},
	{0xc5, 0x5E},
	{0xc6, 0x78},
	{0xc7, 0x90},
	{0xc8, 0xA6},
	{0xc9, 0xB9},
	{0xcA, 0xC9},
	{0xcB, 0xD6},
	{0xcC, 0xE0},
	{0xcD, 0xEE},
	{0xcE, 0xF8},
	{0xcF, 0xFF},

	case GC0329_RGB_Gamma_m3:			
	{0xBF, 0x0B},
	{0xc0, 0x16},
	{0xc1, 0x29},
	{0xc2, 0x3C},
	{0xc3, 0x4F},
	{0xc4, 0x5F},
	{0xc5, 0x6F},
	{0xc6, 0x8A},
	{0xc7, 0x9F},
	{0xc8, 0xB4},
	{0xc9, 0xC6},
	{0xcA, 0xD3},
	{0xcB, 0xDD},
	{0xcC, 0xE5},
	{0xcD, 0xF1},
	{0xcE, 0xFA},
	{0xcF, 0xFF},

	case GC0329_RGB_Gamma_m4:
	{0xBF, 0x0E},
	{0xc0, 0x1C},
	{0xc1, 0x34},
	{0xc2, 0x48},
	{0xc3, 0x5A},
	{0xc4, 0x6B},
	{0xc5, 0x7B},
	{0xc6, 0x95},
	{0xc7, 0xAB},
	{0xc8, 0xBF},
	{0xc9, 0xCE},
	{0xcA, 0xD9},
	{0xcB, 0xE4},
	{0xcC, 0xEC},
	{0xcD, 0xF7},
	{0xcE, 0xFD},
	{0xcF, 0xFF},

	case GC0329_RGB_Gamma_m5:
	{0xBF, 0x10},
	{0xc0, 0x20},
	{0xc1, 0x38},
	{0xc2, 0x4E},
	{0xc3, 0x63},
	{0xc4, 0x76},
	{0xc5, 0x87},
	{0xc6, 0xA2},
	{0xc7, 0xB8},
	{0xc8, 0xCA},
	{0xc9, 0xD8},
	{0xcA, 0xE3},
	{0xcB, 0xEB},
	{0xcC, 0xF0},
	{0xcD, 0xF8},
	{0xcE, 0xFD},
	{0xcF, 0xFF},

	case GC0329_RGB_Gamma_m6: // largest gamma curve						
	{0xBF, 0x14},
	{0xc0, 0x28},
	{0xc1, 0x44},
	{0xc2, 0x5D},
	{0xc3, 0x72},
	{0xc4, 0x86},
	{0xc5, 0x95},
	{0xc6, 0xB1},
	{0xc7, 0xC6},
	{0xc8, 0xD5},
	{0xc9, 0xE1},
	{0xcA, 0xEA},
	{0xcB, 0xF1},
	{0xcC, 0xF5},
	{0xcD, 0xFB},
	{0xcE, 0xFE},
	{0xcF, 0xFF},

	case GC0329_RGB_Gamma_night:	//Gamma for night mode
	{0xBF, 0x0B},
	{0xc0, 0x16},
	{0xc1, 0x29},
	{0xc2, 0x3C},
	{0xc3, 0x4F},
	{0xc4, 0x5F},
	{0xc5, 0x6F},
	{0xc6, 0x8A},
	{0xc7, 0x9F},
	{0xc8, 0xB4},
	{0xc9, 0xC6},
	{0xcA, 0xD3},
	{0xcB, 0xDD},
	{0xcC, 0xE5},
	{0xcD, 0xF1},
	{0xcE, 0xFA},
	{0xcF, 0xFF},

	//GC0329_RGB_Gamma_m1  default
	{0xfe, 0x00},
	{0xbf, 0x06},
	{0xc0, 0x12},
	{0xc1, 0x22},
	{0xc2, 0x35},
	{0xc3, 0x4b},
	{0xc4, 0x5f},
	{0xc5, 0x72},
	{0xc6, 0x8d},
	{0xc7, 0xa4},
	{0xc8, 0xb8},
	{0xc9, 0xc8},
	{0xca, 0xd4},
	{0xcb, 0xde},
	{0xcc, 0xe6},
	{0xcd, 0xf1},
	{0xce, 0xf8},
	{0xcf, 0xfd},
#endif

	//////////////////CC///////////////////
	{0xfe, 0x00},

	{0xb3, 0x40},
	{0xb4, 0xf4},//fd cyrille
	{0xb5, 0x02},
	{0xb6, 0xfa},
	{0xb7, 0x48},
	{0xb8, 0xf0},

	// crop 						   
	{0x50, 0x01},

	////////////////////YCP////////////////////
	{0xfe, 0x00},
	{0xd1, 0x24},
	{0xd2, 0x24},
	{0xd3, 0x32},
	{0xd5, 0x05},	   /// luma_offset
	{0xdd, 0x56},//54 cyrille

	////////////////////AEC////////////////////
	{0xfe, 0x01},
	{0x10, 0x40},
	{0x11, 0x21},
	{0x12, 0x07},
	{0x13, 0x60},//50 cyrille
	{0x17, 0x88},
	{0x21, 0xa0},//80 cyrille
	{0x22, 0x50},
	{0x3c, 0x95},
	{0x3d, 0x50},
	{0x3e, 0x48}, 

	////////////////////AWB////////////////////
	{0xfe, 0x01},
	{0x06, 0x16},
	{0x07, 0x06},
	{0x08, 0x98},
	{0x09, 0xee},
	{0x50, 0xfc},
	{0x51, 0x20},
	{0x52, 0x0b},
	{0x53, 0x20},
	{0x54, 0x40},
	{0x55, 0x10},
	{0x56, 0x20},
	//{0x57, 0x40},
	{0x58, 0xa0},
	{0x59, 0x28},
	{0x5a, 0x02},
	{0x5b, 0x63},
	{0x5c, 0x34},
	{0x5d, 0x73},
	{0x5e, 0x11},
	{0x5f, 0x40},
	{0x60, 0x40},
	{0x61, 0xc8},
	{0x62, 0xa0},
	{0x63, 0x40},
	{0x64, 0x50},
	{0x65, 0x98},
	{0x66, 0xfa},
	{0x67, 0x70},
	{0x68, 0x58},
	{0x69, 0x85},
	{0x6a, 0x40},
	{0x6b, 0x39},
	{0x6c, 0x18},
	{0x6d, 0x28},
	{0x6e, 0x41},
	{0x70, 0x02},
	{0x71, 0x00},
	{0x72, 0x10},
	{0x73, 0x40},

	//{0x74, 0x32},
	//{0x75, 0x40},
	//{0x76, 0x30},
	//{0x77, 0x48},
	//{0x7a, 0x50},
	//{0x7b, 0x20}, 

	{0x80, 0x60},
	{0x81, 0x50},
	{0x82, 0x42},
	{0x83, 0x40},
	{0x84, 0x40},
	{0x85, 0x40},

	{0x74, 0x40},
	{0x75, 0x58},
	{0x76, 0x24},
	{0x77, 0x40},
	{0x78, 0x20},
	{0x79, 0x60},
	{0x7a, 0x58},
	{0x7b, 0x20},
	{0x7c, 0x30},
	{0x7d, 0x35},
	{0x7e, 0x10},
	{0x7f, 0x08},

	////////////////////ABS////////////////////
	{0x9c, 0x02}, 
	{0x9d, 0x20},
	//{0x9f, 0x40},	

	////////////////////CC-AWB////////////////////
	{0xd0, 0x00},
	{0xd2, 0x2c},
	{0xd3, 0x80}, 

	////////////////////LSC///////////////////
	{0xfe, 0x01},
	{0xa0, 0x00},
	{0xa1, 0x3c},
	{0xa2, 0x50},
	{0xa3, 0x00},
	{0xa8, 0x0f},
	{0xa9, 0x08},
	{0xaa, 0x00},
	{0xab, 0x04},
	{0xac, 0x00},
	{0xad, 0x07},
	{0xae, 0x0e},
	{0xaf, 0x00},
	{0xb0, 0x00},
	{0xb1, 0x09},
	{0xb2, 0x00},
	{0xb3, 0x00},
	{0xb4, 0x31},
	{0xb5, 0x19},
	{0xb6, 0x24},
	{0xba, 0x3a},
	{0xbb, 0x24},
	{0xbc, 0x2a},
	{0xc0, 0x17},
	{0xc1, 0x13},
	{0xc2, 0x17},
	{0xc6, 0x21},
	{0xc7, 0x1c},
	{0xc8, 0x1c},
	{0xb7, 0x00},
	{0xb8, 0x00},
	{0xb9, 0x00},
	{0xbd, 0x00},
	{0xbe, 0x00},
	{0xbf, 0x00},
	{0xc3, 0x00},
	{0xc4, 0x00},
	{0xc5, 0x00},
	{0xc9, 0x00},
	{0xca, 0x00},
	{0xcb, 0x00},
	{0xa4, 0x00},
	{0xa5, 0x00},
	{0xa6, 0x00},
	{0xa7, 0x00},

	////////////////////asde ///////////////////
	//{0xa0, 0xaf},
	//{0xa2, 0xff},

	////////////20120427/////////////               
	{0xfe,0x01}, 		
	{0x18,0x22}, 		
	{0x21,0xa0}, 		
	{0x06,0x12},		
	{0x08,0x9c},		
	{0x51,0x28},		
	{0x52,0x10},		
	{0x53,0x20},		
	{0x54,0x40},		
	{0x55,0x16},		
	{0x56,0x30},		
	{0x58,0x60},		
	{0x59,0x08},		
	{0x5c,0x35},		
	{0x5d,0x72},		
	{0x67,0x80},		
	{0x68,0x60},		
	{0x69,0x90},		
	{0x6c,0x30},		
	{0x6d,0x60},		
	{0x70,0x10},	        
							
	{0xfe,0x00},		
	{0x9c,0x0a},		
	{0xa0,0xaf},		
	{0xa2,0xff},		
	{0xa4,0x60},		
	{0xa5,0x31},		
	{0xa7,0x35},		
	{0x42,0xfe},		
//	{0xd1,0x28},		
//	{0xd2,0x28},		
	{0xfe,0x00},	
	SensorEnd
};
/* Senor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] ={
	SensorEnd

};
/* Senor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] ={
	SensorEnd
};
/* Preview resolution setting*/
static struct rk_sensor_reg sensor_preview_data[] =
{
	SensorEnd
};
/* 1280x720 */
static struct rk_sensor_reg sensor_720p[]={
	SensorEnd
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[]={
	SensorEnd
};


static struct rk_sensor_reg sensor_softreset_data[]={
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[]={
	SensorEnd
};
/*
*  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
*/
static struct rk_sensor_reg sensor_WhiteB_Auto[]=
{
	{0xfe, 0x00},
	{0x77, 0x57},
	{0x78, 0x4d},
	{0x79, 0x45},
	{0x42, 0xfe},
	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[]=
{
	{0xfe, 0x00},
	{0x42, 0xfc},
	{0x77, 0x8c}, //WB_manual_gain 
	{0x78, 0x50},
	{0x79, 0x40},
	SensorEnd
};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[]=
{
	//Sunny
	{0xfe, 0x00},
			{0x42, 0xfc},
			{0x77, 0x74}, 
			{0x78, 0x52},
			{0x79, 0x40},	
	SensorEnd
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[]=
{
	//Office
	{0xfe, 0x00},
			{0x42, 0xfc},
			{0x77, 0x40},
			{0x78, 0x42},
			{0x79, 0x50},
	SensorEnd

};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[]=
{
	//Home
	{0xfe, 0x00},
	{0x42, 0xfc},
	{0x77, 0x40},
	{0x78, 0x54},
	{0x79, 0x70},
	SensorEnd
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};

static	struct rk_sensor_reg sensor_Brightness0[]=
{
	// Brightness -2
	
	{0xfe, 0x01}, 
	{0x13, 0x40},
	{0xfe, 0x00}, 
	{0xd5, 0xe0},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness1[]=
{
	// Brightness -1
	
	{0xfe, 0x01}, 
	{0x13, 0x48},
	{0xfe, 0x00}, 
	{0xd5, 0xf0},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness2[]=
{
	//	Brightness 0
	
	
	{0xfe, 0x01}, 
	{0x13, 0x50},
	{0xfe, 0x00}, 
	{0xd5, 0x00},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness3[]=
{
	// Brightness +1
	
	{0xfe, 0x01}, 
	{0x13, 0x58},
	{0xfe, 0x00}, 
	{0xd5, 0x10},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness4[]=
{
	//	Brightness +2
	
	{0xfe, 0x01}, 
	{0x13, 0x60},
	{0xfe, 0x00}, 
	{0xd5, 0x20},

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness5[]=
{
	//	Brightness +3
	
	{0xfe, 0x01}, 
	{0x13, 0x68},
	{0xfe, 0x00}, 
	{0xd5, 0x30},

	SensorEnd
};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
	sensor_Brightness4, sensor_Brightness5,NULL,
};

static	struct rk_sensor_reg sensor_Effect_Normal[] =
{
	{0xfe, 0x00},
	{0x43,0x00},  
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_WandB[] =
{
	{0xfe, 0x00},
	{0x43,0x02},
	{0xda,0x00},
	{0xdb,0x00}, 
	
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Sepia[] =
{
	{0xfe,0x00},
	{0x43,0x02},
	{0xda,0xd0},
	{0xdb,0x28},	 
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Negative[] =
{
	//Negative
	{0xfe,0x00},
	{0x43,0x01},
	SensorEnd
};
static	struct rk_sensor_reg sensor_Effect_Bluish[] =
{
	// Bluish
	{0xfe,0x00},
	{0x43,0x02},
	{0xda,0x50},
	{0xdb,0xe0}, 
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Green[] =
{
	//	Greenish
	{0xfe,0x00},
	{0x43,0x02},
	{0xda,0xc0},
	{0xdb,0xc0}, 
	SensorEnd
};
#if 0
static struct rk_sensor_reg sensor_Effect_Grayscale[]=
{
	{0x23,0x02},	
	{0x2d,0x0a},
	{0x20,0xff},
	{0xd2,0x90},
	{0x73,0x00},

	{0xb3,0x40},
	{0xb4,0x80},
	{0xba,0x00},
	{0xbb,0x00},
	{0x00,0x00}
};
#endif
static struct rk_sensor_reg *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_WandB, sensor_Effect_Negative,sensor_Effect_Sepia,
	sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};

static	struct rk_sensor_reg sensor_Exposure0[]=
{
	//-3

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure1[]=
{
	//-2
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure2[]=
{
	//-1
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure3[]=
{
	//default
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure4[]=
{
	// 1
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure5[]=
{
	// 2

	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure6[]=
{
	// 3
	SensorEnd
};

static struct rk_sensor_reg *sensor_ExposureSeqe[] = {sensor_Exposure0, sensor_Exposure1, sensor_Exposure2, sensor_Exposure3,
	sensor_Exposure4, sensor_Exposure5,sensor_Exposure6,NULL,
};

static	struct rk_sensor_reg sensor_Saturation0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation2[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_SaturationSeqe[] = {sensor_Saturation0, sensor_Saturation1, sensor_Saturation2, NULL,};

static	struct rk_sensor_reg sensor_Contrast0[]=
{

	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast1[]=
{

	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast2[]=
{

	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast3[]=
{

	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast4[]=
{

	SensorEnd
};


static	struct rk_sensor_reg sensor_Contrast5[]=
{

	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast6[]=
{

	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
	sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};
static	struct rk_sensor_reg sensor_SceneAuto[] =
{
	{0xfe, 0x01},
	{0x33, 0x20},
	{0xfe, 0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_SceneNight[] =
{
	{0xfe, 0x01},
	{0x33, 0x30},
	{0xfe, 0x00},
	SensorEnd
};
static struct rk_sensor_reg *sensor_SceneSeqe[] = {sensor_SceneAuto, sensor_SceneNight,NULL,};

static struct rk_sensor_reg sensor_Zoom0[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom1[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom2[] =
{
	SensorEnd
};


static struct rk_sensor_reg sensor_Zoom3[] =
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ZoomSeqe[] = {sensor_Zoom0, sensor_Zoom1, sensor_Zoom2, sensor_Zoom3, NULL,};

/*
* User could be add v4l2_querymenu in sensor_controls by new_usr_v4l2menu
*/
static struct v4l2_querymenu sensor_menus[] =
{
};
/*
* User could be add v4l2_queryctrl in sensor_controls by new_user_v4l2ctrl
*/
static struct sensor_v4l2ctrl_usr_s sensor_controls[] =
{
};

//MUST define the current used format as the first item   
static struct rk_sensor_datafmt sensor_colour_fmts[] = {
	{V4L2_MBUS_FMT_YUYV8_2X8, V4L2_COLORSPACE_JPEG} 
};
static struct soc_camera_ops sensor_ops;


/*
**********************************************************
* Following is local code:
* 
* Please codeing your program here 
**********************************************************
*/
/*
**********************************************************
* Following is callback
* If necessary, you could coding these callback
**********************************************************
*/
/*
* the function is called in open sensor  
*/
static int sensor_activate_cb(struct i2c_client *client)
{
	
	return 0;
}
/*
* the function is called in close sensor
*/
static int sensor_deactivate_cb(struct i2c_client *client)
{
	
	return 0;
}
/*
* the function is called before sensor register setting in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{

	return 0;
}
/*
* the function is called after sensor register setting finished in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_bh (struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	return 0;
}
static int sensor_try_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_softrest_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	
	return 0;
}
static int sensor_check_id_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	char pid = 0;
	int ret = 0;
	struct generic_sensor *sensor = to_generic_sensor(client);
	
	/* check if it is an sensor sensor */
	ret = sensor_write(client, 0xfc, 0x16);  //before read id should write 0xfc
	msleep(20);
	ret = sensor_read(client, 0x00, &pid);
	if (ret != 0) {
		SENSOR_TR("%s read chip id high byte failed\n",SENSOR_NAME_STRING());
		ret = -ENODEV;
	}
	SENSOR_DG("\n %s  pid = 0x%x\n", SENSOR_NAME_STRING(), pid);
	if (pid == SENSOR_ID) {
		sensor->model = SENSOR_V4L2_IDENT;
	} else {
		SENSOR_TR("error: %s mismatched   pid = 0x%x\n", SENSOR_NAME_STRING(), pid);
		ret = -ENODEV;
	}
	return pid;
}
static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	//struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
		
	if (pm_msg.event == PM_EVENT_SUSPEND) {
		SENSOR_DG("Suspend");
		
	} else {
		SENSOR_TR("pm_msg.event(0x%x) != PM_EVENT_SUSPEND\n",pm_msg.event);
		return -EINVAL;
	}
	return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{

	SENSOR_DG("Resume");

	return 0;

}
static int sensor_mirror_cb (struct i2c_client *client, int mirror)
{
	char val;
	int err = 0;
	
	SENSOR_DG("mirror: %d",mirror);
	if (mirror) {
			sensor_write(client, 0xfe, 0);
			err = sensor_read(client, 0x17, &val);
			if (err == 0) {
				if((val & 0x1) == 0)
					err = sensor_write(client, 0x17, (val |0x1));
				else 
					err = sensor_write(client, 0x17, (val & 0xfe));
			}
		} else {
			//do nothing
		}

	return err;    
}
/*
* the function is v4l2 control V4L2_CID_HFLIP callback	
*/
static int sensor_v4l2ctrl_mirror_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
													 struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_mirror_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_mirror failed, value:0x%x",ext_ctrl->value);
	
	SENSOR_DG("sensor_mirror success, value:0x%x",ext_ctrl->value);
	return 0;
}

static int sensor_flip_cb(struct i2c_client *client, int flip)
{
	char val;
	int err = 0;	

	SENSOR_DG("flip: %d",flip);
	if (flip) {
		sensor_write(client, 0xfe, 0);
		err = sensor_read(client, 0x17, &val);
		if (err == 0) {
			if((val & 0x2) == 0)
				err = sensor_write(client, 0x17, (val |0x2));
			else 
				err = sensor_write(client, 0x17, (val & 0xfc));
		}
	} else {
		//do nothing
	}

	return err;    
}
/*
* the function is v4l2 control V4L2_CID_VFLIP callback	
*/
static int sensor_v4l2ctrl_flip_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
													 struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_flip_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_flip failed, value:0x%x",ext_ctrl->value);
	
	SENSOR_DG("sensor_flip success, value:0x%x",ext_ctrl->value);
	return 0;
}
/*
* the functions are focus callbacks
*/
static int sensor_focus_init_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_single_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_near_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_far_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_specialpos_usr_cb(struct i2c_client *client,int pos){
	return 0;
}

static int sensor_focus_af_const_usr_cb(struct i2c_client *client){
	return 0;
}
static int sensor_focus_af_const_pause_usr_cb(struct i2c_client *client)
{
    return 0;
}
static int sensor_focus_af_close_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_zoneupdate_usr_cb(struct i2c_client *client, int *zone_tm_pos)
{
	return 0;
}

/*
face defect call back
*/
static int	sensor_face_detect_usr_cb(struct i2c_client *client,int on){
	return 0;
}

/*
*	The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
* initialization in the function. 
*/
static void sensor_init_parameters_user(struct specific_sensor* spsensor,struct soc_camera_device *icd)
{
	return;
}

/*
* :::::WARNING:::::
* It is not allowed to modify the following code
*/

sensor_init_parameters_default_code();

sensor_v4l2_struct_initialization();

sensor_probe_default_code();

sensor_remove_default_code();

sensor_driver_default_module_code();

