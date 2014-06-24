/*
 * tchip_devices.c
 *
 *  Created on: 2012-2-17
 *      Author: zhansb
 */

struct tchip_device {
	char		name[20];
	unsigned short	active;
};

#define TCSI_GET_GROUP_INDEX(pos, group) ((pos) - TCSI_##group##_PRESTART - 1)

#define TCSI_GET_CODEC_INDEX(pos) (TCSI_GET_GROUP_INDEX(pos, CODEC))
#define TCSI_GET_WIFI_INDEX(pos) (TCSI_GET_GROUP_INDEX(pos, WIFI))
#define TCSI_GET_HDMI_INDEX(pos) (TCSI_GET_GROUP_INDEX(pos, HDMI))
#define TCSI_GET_MODEM_INDEX(pos) (TCSI_GET_GROUP_INDEX(pos, MODEM))

#define GET_CUR_DEVICE(list)	get_cur_device(list, (sizeof(list)/sizeof(list[0])))
#define GET_DEVICE_LIST(Info, list)	get_device_list(Info, list, (sizeof(list)/sizeof(list[0])))

/*
 * 	board support list
 */
static const struct tchip_device tchip_boards[] =
{
#if defined(CONFIG_TCHIP_MACH_DEFAULT)
	{.name = "TCHIP",.active = 1},
#elif defined(CONFIG_TCHIP_MACH_TR726)
        {   .name = "TR726",.active = 1},
#elif defined(CONFIG_TCHIP_MACH_TR726C)
        {   .name = "TR726C",.active = 1},
#elif defined(CONFIG_TCHIP_MACH_TR976Q)
        {   .name = "TR976Q",.active = 1},
#endif
};

/*
 * 	touch support list
 */
static const struct tchip_device tchip_touchs[] =
{
#ifdef	CONFIG_GOODIX_CAPACITIVE_SIGNAL_IC
	{.name = "GT801",.active = 1},
#elif defined(CONFIG_TOUCHSCREEN_CT36X)
	{.name = "CT365",.active = 1},
#elif defined(CONFIG_GSL1680) && defined(CONFIG_TOUCHSCREEN_GT811)
	{.name = "TP1",.active = 1}, //GT811&GSL1680
#elif defined(CONFIG_TOUCHSCREEN_GT811)
	{.name = "GT811",.active = 1},
#elif defined(CONFIG_TOUCHSCREEN_GSLX680_RK3028)
	{.name = "GSL1680",.active = 1},
#elif defined(CONFIG_TOUCHSCREEN_ICN83XX)
	{.name = "ICN83XX",.active = 1},
#elif defined(CONFIG_TOUCHSCREEN_ICN850X)
	{.name = "ICN850X",.active = 1},
#else
	{.name = "NoTouch",.active = 1},
#endif

};

/*
 * 	tp screen  support list
 */
static const struct tchip_device tchip_tps[] =
{
#ifdef CONFIG_TCHIP_TP_FEICU
	{.name = "FC",.active = 1},
#else 
	{.name = "Unknow",.active = 1},
#endif
};



/*
 * 	wifi support list
 */
static const struct tchip_device tchip_wifis[] =
{
#ifdef CONFIG_MT5931_MT6622
     { .name = "CDTK25931", .active = 1,},
#endif
#ifdef	CONFIG_AR6003
	{.name = "AR6302", .active = 1,},
#endif
#ifdef	CONFIG_BCM4329
	{.name = "B23",	.active = 1,},
#endif
#ifdef	CONFIG_MV8686
	{.name = "MV8686", .active = 1,	},
#endif
#ifdef	CONFIG_RTL8192CU
	{.name = "RTL8188", .active = 1,},
#endif
#ifdef	CONFIG_RK903
	{.name = "RK903", .active = 1,},
#endif
#ifdef	CONFIG_RT5370V2_STA
	{.name = "RT5370",	.active = 1,},
#endif
#ifdef	CONFIG_RT5370
	{.name = "RT5370V2", .active = 1,},
#endif
#ifdef	CONFIG_MT7601
	{.name = "MT7601", .active = 1,},
#endif
#ifdef	CONFIG_AIDC
	{.name = "8188ETV_MT7601", .active = 1,},
#endif
#ifdef CONFIG_NMC1XXX_WIFI_MODULE
    #ifdef CONFIG_NMC1XXX_SPI_722_VERSION
    {.name = "NMCv722",        .active = 1,},
    #else
    {.name = "NMCv94x",    .active = 1,},
    #endif
#endif
};

/* ###########   bt list 
*/

static const struct tchip_device tchip_bt[] =
{
#ifdef CONFIG_MT5931_MT6622
     { .name = "MT6622", .active = 1,},
#else
     { .name = "NoBt", .active =1, },
#endif
};


/*      camera support list
*/
static const struct tchip_device tchip_cameras[] =
{
#ifdef CONFIG_TCHIP_CAM_ALL
     { .name = "CAM+all", .active = 1,},
#else

#ifdef CONFIG_TCHIP_CAM_B_BF3920
     { .name = "BF3920", .active = 1,},
#endif
#ifdef CONFIG_TCHIP_CAM_B_BF3703
     { .name = "BF3703", .active = 1,},
#endif
#ifdef CONFIG_TCHIP_CAM_F_GC0308
     { .name = "GC0308", .active = 1,},
#endif
#ifdef CONFIG_TCHIP_CAM_F_GC0328
     { .name = "GC0328", .active = 1,},
#endif

#endif // end of cam+all
	
};
	
static const struct tchip_device tchip_screen[] =
{
#ifdef CONFIG_LCD_RK2926_V86
     { .name = "TN", .active = 1,},
#else
     { .name = "NoLCD", .active = 1,},
#endif
};

static const struct tchip_device tchip_gsensor[] =
{
#ifdef CONFIG_GS_STK831X
     { .name = "STK8312", .active = 1,},
#else
     { .name = "NoGs", .active = 1,},
#endif
};

static const struct tchip_device tchip_misc[] =
{

};

static const struct tchip_device tchip_customer[] =
{
#if defined (CONFIG_TCHIP_TR726C_CUSTOMER_PUBLIC)
     { .name = "Public", .active = 1,},
#elif defined (CONFIG_TCHIP_TR726C_CUSTOMER_JINGHUA)
     { .name = "jh", .active = 1,},
#elif defined (CONFIG_TCHIP_TR726C_CUSTOMER_CUBE)
     { .name = "cube", .active = 1,},
#elif defined (CONFIG_TCHIP_TR726C_CUSTOMER_GBXY)
     { .name = "gbxy", .active = 1,},
#elif defined (CONFIG_TCHIP_TR726C_CUSTOMER_AIPU)
     { .name = "aipu", .active = 1,},
#elif defined (CONFIG_TCHIP_TR726C_CUSTOMER_HUIKE)
     { .name = "Huike", .active = 1,},
#elif defined (CONFIG_TCHIP_TR726C_CUSTOMER_XFH)
     { .name = "XFH", .active = 1,},
#endif
};






		
/*
 * 	codec support list
 */
static const struct tchip_device tchip_codecs[] =
{
#ifdef CONFIG_SND_RK29_SOC_ES8323
    [TCSI_GET_CODEC_INDEX(TCSI_CODEC_ES8323)] =
    {
        .name = "ES8323",
#ifdef  CONFIG_TCHIP_MACH_SND_RK29_SOC_ES8323
        .active = 1,
#endif
    },
#endif
#ifdef	CONFIG_SND_RK29_SOC_RK610
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_RK610)] =
	{
		.name = "RK610CODEC",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_RK610
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_SND_RK29_SOC_WM8988
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_WM8988)] =
	{
		.name = "WM8988",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_WM8988
		.active = 1,
#endif
	},
#endif

#ifdef	CONFIG_SND_RK29_SOC_WM8900
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_WM8900)] =
	{
		.name = "WM8900",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_WM8900
		.active = 1,
#endif
	},
#endif

#ifdef	CONFIG_SND_RK29_SOC_RT5621
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_RT5621)] =
	{
		.name = "RT5621",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_RT5621
		.active = 1,
#endif
	},
#endif

#ifdef	CONFIG_SND_RK29_SOC_WM8994
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_WM8994)] =
	{
		.name = "WM8994",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_WM8994
		.active = 1,
#endif
	},
#endif

#ifdef	CONFIG_SND_RK29_SOC_RT5631
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_RT5631)] =
	{
		.name = "RT5631",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_RT5631
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_SND_RK29_SOC_ES8323
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_ES8323)] =
	{
		.name = "ES8323",
		.active = 1,
	},
#endif
};

/*
 * 	encrypt support list
 */
static const struct tchip_device tchip_encrypts[] =
{
#ifdef	CONFIG_AT18_DEVICE
	{.name = "AT18",.active = 1},
#elif defined(CONFIG_AT28_DEVICE)
	{.name = "AT28",.active = 1},
#elif defined(CONFIG_AT38_DEVICE)
	{.name = "AT38",.active = 1},
#endif
};
/*
 * 	modem support list
 */
static const struct tchip_device tchip_modems[] =
{
#ifdef	CONFIG_MODEM_ROCKCHIP_DEMO
	[TCSI_GET_MODEM_INDEX(TCSI_MODEM_OTHERS)] =
	{
		.name = "MODEM",
#ifdef	CONFIG_TCHIP_MACH_MODEM_OTHERS
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_TDM330
	[TCSI_GET_MODEM_INDEX(TCSI_MODEM_TDM330)] =
	{
		.name = "TDM330",
#ifdef	CONFIG_TCHIP_MACH_MODEM_TDM330
		.active = 1,
#endif
	},
#endif
};
/*
 * 	hdmi support list
 */
static const struct tchip_device tchip_hdmis[] =
{
#if	defined(CONFIG_HDMI_RK610)
	[TCSI_GET_HDMI_INDEX(TCSI_HDMI_RK610)] =
	{
		.name = "RK610HDMI",
#if defined(CONFIG_TCHIP_MACH_HDMI_RK610)
		.active = 1,
#endif
	},
#endif
#if	defined(CONFIG_ANX7150) || defined(CONFIG_ANX7150_NEW)
	[TCSI_GET_HDMI_INDEX(TCSI_HDMI_ANX7150)] =
	{
		.name = "ANX7150",
#ifdef	CONFIG_TCHIP_MACH_ANX7150
		.active = 1,
#endif
	},
#endif
#if	defined(CONFIG_CAT6611) || defined(CONFIG_CAT6611_NEW)
	[TCSI_GET_HDMI_INDEX(TCSI_HDMI_CAT6611)] =
	{
		.name = "CAT6611",
#if defined(CONFIG_TCHIP_MACH_CAT6611)
		.active = 1,
#endif
	},
#endif
#if	defined(CONFIG_HDMI_RK30)
	[TCSI_GET_HDMI_INDEX(TCSI_HDMI_RK30)] =
	{
		.name = "RK30HDMI",
#if defined(CONFIG_TCHIP_MACH_HDMI_RK30)
		.active = 1,
#endif
	},
#endif
};



struct tchip_device *get_cur_device(struct tchip_device *devices, int size)
{
	int i;

	for(i = 0; i < size; devices++,i++)
	{
		if(devices->active)
		{
			return devices;
		}
	}

	return 0;
}

static char * strupper(char * dst, char *src)
{
	char * start = dst;

	while(*src!='\0')
		*dst++ = toupper(*src++);
	*dst = '\0';

	return start;
}

static void add2versionex(char *version, struct tchip_device *dev, char *prefix)
{
	char str[20];

	strcpy(str, prefix);

	strupper(&str[strlen(prefix)], dev->name);
	strcat(version, str);
}

static void add2version(char *version, struct tchip_device *dev)
{
	add2versionex(version,dev,"_");
}

struct tchip_device *set_all_active_device_version(struct tchip_device *devices, int size, char *version)
{
	int i;

	for(i = 0; i < size; devices++,i++)
	{
		if(devices->active)
		{
			add2version(version, devices);
		}
	}
}
struct tchip_device *set_all_active_device_version_ex(struct tchip_device *devices, int size, char *version,char* prefix)
{
	int i;

	for(i = 0; i < size; devices++,i++)
	{
		if(devices->active)
		{
			if ( i > 0 )
				add2versionex(version, devices,prefix);
			else
				add2version(version, devices);
		}
	}
}
#if 0
void cur_sensor_init(void)
{
	int i;

	cur_sensor[SENSOR_BACK] = cur_sensor[SENSOR_FRONT] = &no_sensor;

	for(i = 0; i < (TCSI_GET_SENSOR_INDEX(TCSI_CAMERA_END)); i++)
	{
		if(sensors[i].active)
		{
			if(TCSI_SENSOR_POS_BACK == sensors[i].pos)
				cur_sensor[SENSOR_BACK] = &sensors[i];
			else if(TCSI_SENSOR_POS_FRONT == sensors[i].pos)
				cur_sensor[SENSOR_FRONT] = &sensors[i];
		}
	}
}
#endif
int get_device_list(char * Info, struct tchip_device *devices, int size)
{
	int i;

	for (i = 0; i < size; devices++, i++)
	{
		if (devices->active)
			Info[i] = 1;

		strncpy(&Info[10 + i * 9], devices->name, 8);

		//printf("-->%9s:%d\n", &Info[10 + i * 9], Info[i]);
	}

	return i;
}
