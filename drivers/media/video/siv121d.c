
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

#define SENSOR_DG(format, ...) dprintk(1, format, ## __VA_ARGS__)
/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_SIV121D
#define SENSOR_V4L2_IDENT V4L2_IDENT_SIV121D
#define SENSOR_ID 0xDE
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
    {0x00, 0x01},
    {0x03, 0x08},
   
    {0x00, 0x01},
    {0x03, 0x08},
   
    {0x00, 0x00},
    {0x03, 0x04},
    {0x10, 0x86},
    {0x11, 0x74},	
    
    {0x00, 0x01},
   
    {0x04, 0x00}, 
    {0x06, 0x04}, 
     
    {0x10, 0x11}, 
    {0x11, 0x25}, 
    {0x12, 0x21},
     
    {0x17, 0x94}, //ABS 1.74V
    {0x18, 0x00},
      
    {0x20, 0x00},
    {0x21, 0x05},
    {0x22, 0x01},
    {0x23, 0x69},
     
    {0x40, 0x0F}, 
    {0x41, 0x90},
    {0x42, 0xd2},
    {0x43, 0x00},
    
    // AE
    {0x00, 0x02},
    {0x10, 0x84}, 
    {0x11, 0x10},                    
    {0x12, 0x70},//0x70,//78,//,??-----------
    {0x14, 0x60},//0x70,//78,//-----------
    {0x34, 0x96},//96},              
    {0x40, 0x36},//48,//dark ???-----------
    {0x44, 0x08},
	  
    
    // AWB
    {0x00, 0x03},
    {0x10, 0xd3},
    {0x11, 0xc0},
    {0x13, 0x80},//7e}, //Cr target
    {0x14, 0x80}, //Cb target
    {0x15, 0xe0}, // R gain Top
    {0x16, 0x88}, // R gain bottom 
    {0x17, 0xe0}, // B gain Top
    {0x18, 0x80}, // B gain bottom 0x80
    {0x19, 0x90}, // Cr top value 0x90
    {0x1a, 0x64}, // Cr bottom value 0x70
    {0x1b, 0x94}, // Cb top value 0x90
    {0x1c, 0x6c}, // Cb bottom value 0x70
    {0x1d, 0x94}, // 0xa0
    {0x1e, 0x6c}, // 0x60
    {0x20, 0xe8}, // AWB luminous top value
    {0x21, 0x30}, // AWB luminous bottom value 0x20
    {0x22, 0xb8},
    {0x23, 0x10},
    {0x25, 0x08},
    {0x26, 0x0f},
    {0x27, 0x60}, // BRTSRT 
    {0x28, 0x70}, // BRTEND
    {0x29, 0xb7}, // BRTRGNBOT 
    {0x2a, 0xa3}, // BRTBGNTOP
    								
    {0x40, 0x01},
    {0x41, 0x03},
    {0x42, 0x08},
    {0x43, 0x10},
    {0x44, 0x13},
    {0x45, 0x6a},
    {0x46, 0xca},
    
    
    {0x63, 0x9c}, // R D30 to D20
    {0x64, 0xd4}, // B D30 to D20
    {0x65, 0xa0}, // R D20 to D30
    {0x66, 0xd0}, // B D20 to D30
    
    // IDP
    {0x00, 0x04},
    {0x10, 0x7f},
    {0x11, 0x19},//19},   //VSYNC active at high
    {0x12, 0xfd},
    {0x13, 0xfe},
    {0x14, 0x01},
    
    // DPCBNR
    {0x18, 0xbb},	// DPCNRCTRL
    {0x19, 0x00},	// DPCTHV
    {0x1A, 0x00},	// DPCTHVSLP
    {0x1B, 0x08},	// DPCTHVDIFSRT
   
    
    {0x1E, 0x04},	// BNRTHV  0c
    {0x1F, 0x06},	// BNRTHVSLPN 10
    {0x20, 0x20},	// BNRTHVSLPD
    {0x21, 0x00},	// BNRNEICNT
    {0x22, 0x08},	// STRTNOR
    {0x23, 0x38},	// STRTDRK
    {0x24, 0x00},
    
    // Gamma
    {0x31, 0x03}, //0x08
    {0x32, 0x0b}, //0x10
    {0x33, 0x1e}, //0x1B
    {0x34, 0x46}, //0x37
    {0x35, 0x62}, //0x4D
    {0x36, 0x78}, //0x60
    {0x37, 0x8b}, //0x72
    {0x38, 0x9b}, //0x82
    {0x39, 0xa8}, //0x91
    {0x3a, 0xb6}, //0xA0
    {0x3b, 0xcc}, //0xBA
    {0x3c, 0xdf}, //0xD3
    {0x3d, 0xf0}, //0xEA
    
    // Shading Register Setting 				 
    {0x40, 0x06},						   
    {0x41, 0x44},						   
    {0x42, 0x44},						   
    {0x43, 0x20},						   
    {0x44, 0x22}, // left R gain[7:4], right R gain[3:0] 						
    {0x45, 0x22}, // top R gain[7:4], bottom R gain[3:0] 						 
    {0x46, 0x00}, // left G gain[7:4], right G gain[3:0] 	  
    {0x47, 0x11}, // top G gain[7:4], bottom G gain[3:0] 	  
    {0x48, 0x00}, // left B gain[7:4], right B gain[3:0] 	  
    {0x49, 0x00}, // top B gain[7:4], bottom B gain[3:0] 	 
    {0x4a, 0x04}, // X-axis center high[3:2], Y-axis center high[1:0]	 
    {0x4b, 0x48}, // X-axis center low[7:0]						
    {0x4c, 0xe8}, // Y-axis center low[7:0]					   
    {0x4d, 0x84}, // Shading Center Gain
    {0x4e, 0x00}, // Shading R Offset   
    {0x4f, 0x00}, // Shading Gr Offset  
    {0x50, 0x00}, // Shading B Offset  
    
    // Interpolation
   
    {0x61,  0xc4},	// INTCTRL outdoor
    
    // Color matrix (D65) - Daylight
    {0x71, 0x3b},	 
    {0x72, 0xC7},	 
    {0x73, 0xFe},	 
    {0x74, 0x10},	 
    {0x75, 0x28},	 
    {0x76, 0x08},	 
    {0x77, 0xec},	
    {0x78, 0xcd},	 
    {0x79, 0x47},	 
    
    // Color matrix (D20) - A
    {0x83, 0x38}, //0x3c 	
    {0x84, 0xd1}, //0xc6 	
    {0x85, 0xf7}, //0xff   
    {0x86, 0x12}, //0x12    
    {0x87, 0x25}, //0x24 	
    {0x88, 0x09}, //0x0a 	
    {0x89, 0xed}, //0xed   
    {0x8a, 0xbb}, //0xc2 	
    {0x8b, 0x58}, //0x51
    
    {0x8c, 0x10}, //CMA select	  
    
    //G Edge
    {0x90, 0x35},//18}, //Upper gain
    {0x91, 0x48},//28}, //down gain
    {0x92, 0x33},//55}, //[7:4] upper coring [3:0] down coring
    {0x9a, 0x40},
    {0x9b, 0x40},
    {0x9c, 0x38}, //edge suppress start
    {0x9d, 0x30}, //edge suppress slope
    
    {0x9f, 0x26},
    {0xa0, 0x11},
    
    {0xa8, 0x11},
    {0xa9, 0x11},
    {0xaa, 0x11},
    
    {0xb9, 0x30}, // nightmode 38 at gain 0x48 5fps
    {0xba, 0x44}, // nightmode 80 at gain 0x48 5fps
    
    
    
    {0xbf, 0x20},
    
    {0xe5, 0x15}, //MEMSPDA
    {0xe6, 0x02}, //MEMSPDB
    {0xe7, 0x04}, //MEMSPDC


    {0x15, 0x00},
    
    //Sensor On  
    {0x00, 0x01},
    {0x03, 0x01}, // SNR Enable

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



static struct rk_sensor_reg sensor_WhiteB_Auto[]=
{
    {0x00, 0x03}, 
    {0x60, 0xa0}, 
    {0x61, 0xa0}, 
    {0x10, 0xd3}, 
    	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[]=
{
    {0x00,  0x03}, 
    {0x10, 0x00},  // disable AWB
    {0x60, 0xb4}, 
    {0x61, 0x74}, 
    	SensorEnd

};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[]=
{
    {0x00,  0x03}, 
    {0x10, 0x00},   // disable AWB
    {0x60, 0xd8}, 
    {0x61, 0x90},
   	SensorEnd
    
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[]=
{
    {0x00,  0x03}, 
    {0x10, 0x00},  // disable AWB
    {0x60, 0x80},
    {0x61, 0xe0},
    	SensorEnd
};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[]=
{
    {0x00,  0x03}, 
    {0x10, 0x00},  // disable AWB
    {0x60, 0xb8},
    {0x61, 0xcc},
	SensorEnd
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};

static	struct rk_sensor_reg sensor_Brightness0[]=
{
    // Brightness -2
    {0x00,  0x04}, 
    {0xab,  0xb0}, 
    	SensorEnd  
};

static	struct rk_sensor_reg sensor_Brightness1[]=
{
    // Brightness -1
    {0x00,  0x04}, 
    {0xab,  0xa0}, 
   	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness2[]=
{
    //  Brightness 0
    {0x00,  0x04}, 
    {0xab,  0x82},
 	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness3[]=
{
    // Brightness +1
    {0x00,  0x04}, 
    {0xab,  0x00}, 
   	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness4[]=
{
    //  Brightness +2
    {0x00,  0x04}, 
    {0xab,  0x10}, 
   	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness5[]=
{
    //  Brightness +3
    {0x00,  0x04}, 
    {0xab,  0x20}, 
    	SensorEnd 
};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
	sensor_Brightness4, sensor_Brightness5,NULL,
};

static	struct rk_sensor_reg sensor_Effect_Normal[] =
{
    {0x00,0x04}, 
  //  {0x90,0x14},
  //  {0x91,0x18},
    {0xb6,0x00},
   	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_WandB[] =
{
    //mono 
    {0x00,0x04}, 
    {0x90,0x14},
    {0x91,0x18},
    {0xb6,0x40},
    	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Sepia[] =
{
    {0x00,0x04}, 
    {0x90, 0x14},
    {0x91, 0x18},
    {0xB6, 0x80},
    {0xB7, 0x58},
    {0xB8, 0x98},
  	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Negative[] =
{
    //Negative
    {0x00,0x04}, 
    {0x90, 0x18},
    {0x91, 0x18},
    {0xB6, 0x20},
   	SensorEnd
};
static	struct rk_sensor_reg sensor_Effect_Bluish[] =
{
    {0x00,0x04}, 
    {0x90, 0x14},
    {0x91, 0x18},
    {0xB6, 0x80},
    {0xB7, 0xb8},
    {0xB8, 0x50},
  	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Green[] =
{
    //  Greenish
    {0x00,0x04}, 
    {0x90, 0x14},
    {0x91, 0x18},
    {0xB6, 0x80},
    {0xB7, 0x68},
    {0xB8, 0x68},
	SensorEnd 
};
static struct rk_sensor_reg *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_WandB, sensor_Effect_Negative,sensor_Effect_Sepia,
	sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};

static	struct rk_sensor_reg sensor_Exposure0[]=
{
    {0x00,0x02},
    {0x12,0x44},
    {0x14,0x44},
 	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure1[]=
{
    {0x00,0x02},
    {0x12,0x54},
    {0x14,0x54},
    	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure2[]=
{
    {0x00,0x02},
    {0x12,0x54},
    {0x14,0x54},
    	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure3[]=
{
    {0x00,0x02},
    {0x12,0x60},
    {0x14,0x60},
   	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure4[]=
{
    {0x00,0x02},
    {0x12,0x84},
    {0x14,0x84},
  	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure5[]=
{
    {0x00,0x02},
    {0x12,0x94},
    {0x14,0x94},
    	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure6[]=
{
    {0x00,0x02},
    {0x12,0xa4},
    {0x14,0xa4},
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
    //  Brightness +3
    {0x00,  0x04}, 
    {0xa8,  0xb0}, 
   	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast1[]=
{
    //  Brightness +3
    {0x00,  0x04}, 
    {0xa8,  0xa0}, 
  	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast2[]=
{
    //  Brightness +3
    {0x00,  0x04}, 
    {0xa8,  0x90}, 
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast3[]=
{
    //  Brightness +3
    {0x00,  0x04}, 
    {0xa8,  0x00}, 
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast4[]=
{
    //  Brightness +3
    {0x00,  0x04}, 
    {0xa8,  0x10}, 
 	SensorEnd
};


static	struct rk_sensor_reg sensor_Contrast5[]=
{
    //  Brightness +3
    {0x00,  0x04}, 
    {0xa8,  0x20}, 
    	SensorEnd 
};

static	struct rk_sensor_reg sensor_Contrast6[]=
{
    //  Brightness +3
    {0x00,  0x04}, 
    {0xa8,  0x30}, 
   	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
    sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};
static	struct rk_sensor_reg sensor_SceneAuto[] =
{
    {0x00, 0x02},
	{0x40, 0x48},	
	    {0x00, 0x04},
	{0xab, 0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_SceneNight[] =
{
    {0x00, 0x02},
	{0x40, 0x60},	
	    {0x00, 0x04},
	{0xab, 0x30},
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
	sensor_write(client, 0x00, 0x01);
	sensor_write(client, 0x03, 0x08);
    msleep(200);
	sensor_write(client, 0x00, 0x01);
	sensor_write(client, 0x03, 0x08);
    msleep(200);
	ret = sensor_write(client, 0x00, 0x00);  //before read id should write 0xfc
	msleep(20);
	ret = sensor_read(client, 0x01, &pid);
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
		sensor_write(client, 0x00, 0x01);
		err = sensor_read(client, 0x04, &val);
		if (err == 0) {
			if((val & 0x1) == 0)
				err = sensor_write(client, 0x04, (val |0x00));
			else 
				err = sensor_write(client, 0x04, (val & 0x01));
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
		sensor_write(client, 0x00, 0x01);
		err = sensor_read(client, 0x04, &val);
		if (err == 0) {
			if((val & 0x2) == 0)
				err = sensor_write(client, 0x04, (val |0x00));
			else 
				err = sensor_write(client, 0x04, (val & 0x02));
		}
	} else {
		//do nothing
	}
	return err;    
}

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




