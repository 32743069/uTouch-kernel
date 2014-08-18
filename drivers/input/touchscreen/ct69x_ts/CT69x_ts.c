/**************************************************************************
*  CT69x_ts.c
* 
*  CT69x rockchip sample code version 1.0
* 
*  Create Date : 2014/05/01
* 
*  Modify Date : 
*
*  Create by   : Robert
* 
**************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
    #include <linux/pm.h>
    #include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/io.h>
#include <mach/gpio.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <linux/irq.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <linux/proc_fs.h>

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>

#include <asm/gpio.h>

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <asm/uaccess.h>
#include <linux/input/mt.h>
#include <mach/iomux.h>
#include <asm/io.h>

#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <linux/input/mt.h>
#include <mach/iomux.h>
#include <mach/board.h>


//*************************TouchScreen Work Part*****************************

//define resolution of the touchscreen
#define TOUCH_MAX_HEIGHT 	2048//1024			
#define TOUCH_MAX_WIDTH		2048//600

#define INTMODE

#define CHARGE_DETECT
//#define	BATLOW_DETECT
//#define TOUCH_KEY_SUPPORT


#define AUTO_RUDUCEFRAME
#define I2C_SPEED 400000
#define CT69x_I2C_NAME "ct69x_ts"

static struct i2c_client * this_client=NULL;


#include "CT69x_Drv.h"
#include "CT69x_Reg.h"
#include "CT69x_userpara.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
static void CT69x_ts_early_suspend(struct early_suspend *h);
static void CT69x_ts_late_resume(struct early_suspend *h);
#endif
int CT_I2C_WriteByte(u8 addr, u8 para);
unsigned char CT_I2C_ReadByte(u8 addr);


extern char CT69x_CLB(void);
extern void CT69x_CLB_GetCfg(void);
extern STRUCTCALI       CT_Cali;
extern CT69x_UCF   CTTPCfg;
extern STRUCTBASE		CT_Base;
extern short	Diff[NUM_TX][NUM_RX];
extern short	adbDiff[NUM_TX][NUM_RX];
extern short	CTDeltaData[32];
extern unsigned char TestErrorCode;

char	CT_CALI_FILENAME[50] = {0,};
char	CT_UCF_FILENAME[50] = {0,};
char	CT_DIFF_FILENAME[50] = {0,};

//static unsigned int release = 0;
static unsigned char suspend_flag=0; //0: sleep out; 1: sleep in
static short tp_idlecnt = 0;
static char tp_SlowMode = 1;
static char CTTP_cali_st = 0;
static char CTTP_test_st = 0;

struct ts_event {
	int	x[5];
	int	y[5];
	int	pressure;
	int  touch_ID[5];
	int touch_point;
	int pre_point;
};


struct CT69x_ts_data {
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct work_struct 	Charge_Detectwork;
	struct work_struct 	tp_cali_work;
	struct workqueue_struct *ts_workqueue;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
	//struct hrtimer touch_timer;
	//struct hrtimer charge_detect;	
	struct timer_list touch_timer;
	struct timer_list charge_detect;
	int irq;
    char phys[32];
    char bad_data;
    int use_irq;

	int		reset_gpio;
};

#ifdef TOUCH_KEY_SUPPORT

#define TPD_KEY_COUNT 4

static unsigned short tpd_keys_local[TPD_KEY_COUNT] = {  KEY_MENU,  KEY_HOME,  KEY_BACK,  KEY_SEARCH   };

static unsigned short tpd_keys_dim_local[TPD_KEY_COUNT][4] = {{30,60,485,500}, {80,120,485,500}, {150,190,485,500}, {200,240,485,500}};

static unsigned short key_state = 0;
static unsigned short key_code = 0;
#endif


#ifdef INTMODE
static void ctp_enable_irq(void)
{
	struct CT69x_ts_data *CT69x_ts = i2c_get_clientdata(this_client);
	//printk(" bobo CT69x ctp_enable_irq! suspend_flag = %d\n", suspend_flag);	
	enable_irq(CT69x_ts->irq);
	
	 CT_I2C_ReadByte(SA_ISR);//clear intflag
	return;
}

/**
 * ctp_disable_irq - 
 *
 */
static void ctp_disable_irq(void)
{
	struct CT69x_ts_data *CT69x_ts = i2c_get_clientdata(this_client);
	//printk(" ~~~bobo CT69x ctp_disable_irq!  suspend_flag = %d\n", suspend_flag);	
	disable_irq_nosync(CT69x_ts->irq);
	return;
}
#endif

int CT_nvram_read(char *filename, char *buf, ssize_t len, int offset)
{	
    struct file *fd;
    //ssize_t ret;
    int retLen = -1;
    
    mm_segment_t old_fs = get_fs();
    set_fs(KERNEL_DS);
    
    fd = filp_open(filename, O_RDONLY, 0);
    
    if(IS_ERR(fd)) {
        printk("[CT69x][nvram_read] : failed to open!!\n");
        return -1;
    }
    do{
        if ((fd->f_op == NULL) || (fd->f_op->read == NULL))
    		{
            printk("[CT69x][nvram_read] : file can not be read!!\n");
            break;
    		} 
    		
        if (fd->f_pos != offset) {
            if (fd->f_op->llseek) {
        		    if(fd->f_op->llseek(fd, offset, 0) != offset) {
						printk("[CT69x][nvram_read] : failed to seek!!\n");
					    break;
        		    }
        	  } else {
        		    fd->f_pos = offset;
        	  }
        }    		
        
    		retLen = fd->f_op->read(fd,
    									  buf,
    									  len,
    									  &fd->f_pos);			
    		
    }while(false);
    
    filp_close(fd, NULL);
    
    set_fs(old_fs);
    
    return retLen;
}

int CT_nvram_write(char *filename, char *buf, ssize_t len, int offset)
{	
    struct file *fd;
    //ssize_t ret;
    int retLen = -1;
        
    mm_segment_t old_fs = get_fs();
    set_fs(KERNEL_DS);
    
    fd = filp_open(filename, O_WRONLY|O_CREAT, 0666);
    
    if(IS_ERR(fd)) {
        printk("[CT69x][nvram_write] : failed to open!!\n");
        return -1;
    }
    do{
        if ((fd->f_op == NULL) || (fd->f_op->write == NULL))
    		{
            printk("[CT69x][nvram_write] : file can not be write!!\n");
            break;
    		} /* End of if */
    		
        if (fd->f_pos != offset) {
            if (fd->f_op->llseek) {
        	    if(fd->f_op->llseek(fd, offset, 0) != offset) {
				    printk("[CT69x][nvram_write] : failed to seek!!\n");
                    break;
                }
            } else {
                fd->f_pos = offset;
            }
        }       		
        
        retLen = fd->f_op->write(fd,
                                 buf,
                                 len,
                                 &fd->f_pos);			
    		
    }while(false);
    
    filp_close(fd, NULL);
    
    set_fs(old_fs);
    
    return retLen;
}

int CT_I2C_WriteByte(u8 addr, u8 para)
{
	int ret;
	u8 buf[3];

	struct i2c_msg msg[] = {
		{
			.scl_rate = I2C_SPEED,
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= buf,
		},
	};
	
	buf[0] = addr;
	buf[1] = para;

	ret = i2c_transfer(this_client->adapter, msg, 1);

	return ret;
}

unsigned char CT_I2C_ReadByte(u8 addr)
{
	int ret;
	u8 buf[2] = {0};

	struct i2c_msg msgs[] = {
		{
			.scl_rate = I2C_SPEED,
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= buf,
		},
		{
			.scl_rate = I2C_SPEED,
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= buf,
		},
	};

	buf[0] = addr;
	ret = i2c_transfer(this_client->adapter, msgs, 2);

	return buf[0];
  
}

unsigned char CT_I2C_ReadXByteBlock( unsigned char *buf, unsigned char addr, unsigned short len)
{
	int ret,i;
	u8 rdbuf[512] = {0};
	struct i2c_msg msgs[] = {
		{
			.scl_rate = I2C_SPEED,
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rdbuf,
		},
		{
			.scl_rate = I2C_SPEED,
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= rdbuf,
		},
	};
	rdbuf[0] = addr;

	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	for(i = 0; i < len; i++)
	{
		buf[i] = rdbuf[i];
	}

    return ret;
}

unsigned char CT_I2C_ReadXByte(unsigned char *buf, unsigned char addr, unsigned short len)
{
	int ret;
	short cnt = 0;

	if((addr == SA_RAWDATA) && (len > 248))
	{
		do{
			CT_I2C_WriteByte(SA_ADDRH,(cnt&0xFF00)>>8);
			CT_I2C_WriteByte(SA_ADDRL,cnt&0x00FF);
			if((len - cnt) > 248)
			{
				ret = CT_I2C_ReadXByteBlock(&buf[cnt],SA_RAWDATA,248);
				cnt+=248;
			}
			else
			{
				ret = CT_I2C_ReadXByteBlock(&buf[cnt],SA_RAWDATA,len - cnt);
				cnt = len;
			}
		}while(cnt < len);	
	}
	else
	{
		ret = CT_I2C_ReadXByteBlock(&buf[0],addr,len);
	}
    return ret;
}

unsigned char CT_I2C_WriteXByteBlock( unsigned char *buf, unsigned char addr, unsigned short len)
{
	int ret,i;
	u8 wdbuf[512] = {0};
	struct i2c_msg msgs[] = {
		{
			.scl_rate = I2C_SPEED,
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= len+1,
			.buf	= wdbuf,
		}
	};
	wdbuf[0] = addr;	
	for(i = 0; i < len; i++)
	{
		wdbuf[i+1] = buf[i];
	}



	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msgs, 1);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	

    return ret;
}

unsigned char CT_I2C_WriteXByte( unsigned char *buf, unsigned char addr, unsigned short len)
{
	int ret;
	short cnt = 0;

	if((addr == SA_RAWDATA) && (len > 248))
	{
		do{
			CT_I2C_WriteByte(SA_ADDRH,(cnt&0xFF00)>>8);
			CT_I2C_WriteByte(SA_ADDRL,cnt&0x00FF);
			if((len - cnt) > 248)
			{
				ret = CT_I2C_WriteXByteBlock(&buf[cnt],SA_RAWDATA,248);
				cnt+=248;
			}
			else
			{
				ret = CT_I2C_WriteXByteBlock(&buf[cnt],SA_RAWDATA,len - cnt);
				cnt = len;
			}
		}while(cnt < len);
	}
	else
	{
		ret = CT_I2C_WriteXByteBlock(&buf[0],addr,len);
	}
    return ret;	
}

void CT_Sleep(unsigned int msec)
{
	msleep(msec);
}

static ssize_t CT69x_get_Cali(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_set_Cali(struct device* cd,struct device_attribute *attr, const char *buf, size_t count);
static ssize_t CT69x_get_reg(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_write_reg(struct device* cd,struct device_attribute *attr, const char *buf, size_t count);
static ssize_t CT69x_get_Base(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_get_Diff(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_get_adbBase(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_get_adbDiff(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_get_FreqScan(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_Set_FreqScan(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t CT69x_GetUcf(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_GetCaliSt(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_GetTestSt(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_GetTest(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t CT69x_SetTest(struct device* cd,struct device_attribute *attr, const char *buf, size_t count);


static DEVICE_ATTR(cali,  S_IRUGO | S_IWUGO, CT69x_get_Cali,    CT69x_set_Cali);
static DEVICE_ATTR(readreg,  S_IRUGO | S_IWUGO, CT69x_get_reg,    CT69x_write_reg);
static DEVICE_ATTR(base,  S_IRUGO | S_IWUSR, CT69x_get_Base,    NULL);
static DEVICE_ATTR(diff, S_IRUGO | S_IWUSR, CT69x_get_Diff,    NULL);
static DEVICE_ATTR(adbbase,  S_IRUGO | S_IWUSR, CT69x_get_adbBase,    NULL);
static DEVICE_ATTR(adbdiff, S_IRUGO | S_IWUSR, CT69x_get_adbDiff,    NULL);
static DEVICE_ATTR(freqscan, S_IRUGO | S_IWUGO, CT69x_get_FreqScan,    CT69x_Set_FreqScan);
static DEVICE_ATTR(getucf, S_IRUGO | S_IWUSR, CT69x_GetUcf,    NULL);
static DEVICE_ATTR(cali_st, 	S_IRUGO | S_IWUGO, CT69x_GetCaliSt,    NULL);
static DEVICE_ATTR(test_st, 	S_IWUGO | S_IRUGO, CT69x_GetTestSt, NULL);
static DEVICE_ATTR(test, 	S_IWUGO | S_IRUGO, CT69x_GetTest,	CT69x_SetTest);


static ssize_t CT69x_get_Cali(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len,"VTL RELEASE CODE VER = %d\n", Release_Ver);
	
	len += snprintf(buf+len, PAGE_SIZE-len,"*****CT69x Calibrate data*****\n");
	len += snprintf(buf+len, PAGE_SIZE-len,"TXOFFSET:");
	
	for(i=0;i<11;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", CT_Cali.TXOFFSET[i]);
	}
	
	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "RXOFFSET:");

	for(i=0;i<6;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", CT_Cali.RXOFFSET[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "TXCAC:");

	for(i=0;i<21;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", CT_Cali.TXCAC[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "RXCAC:");

	for(i=0;i<12;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", CT_Cali.RXCAC[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "TXGAIN:");

	for(i=0;i<21;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", CT_Cali.TXGAIN[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "TXCC:");

	for(i=0;i<13;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", CT_Cali.TXCC[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "RXCC:");

	for(i=0;i<7;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", CT_Cali.RXCC[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");

	for(i=0;i<CTTPCfg.TX_LOCAL;i++)
	{
		for(j=0;j<CTTPCfg.RX_LOCAL;j++)
		{
			len += snprintf(buf+len, PAGE_SIZE-len, "%4d ", CT_Cali.SOFTOFFSET[i][j]);
		}
		len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	}
	return len;
	
}

static ssize_t CT69x_set_Cali(struct device* cd,struct device_attribute *attr, const char *buf, size_t count)
{
//#ifndef INTMODE
	struct CT69x_ts_data *CT69x_ts = i2c_get_clientdata(this_client);
//#endif	
	unsigned long on_off = simple_strtoul(buf, NULL, 10);

	CTTP_cali_st = 0;

	if(on_off == 1)
	{
		queue_work(CT69x_ts->ts_workqueue, &CT69x_ts->tp_cali_work);
	}
	
	CTTP_cali_st = 1;

	return count;
}


static ssize_t CT69x_get_adbBase(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len, "base: \n");
	for(i=0;i< CTTPCfg.TX_LOCAL;i++)
	{
		for(j=0;j<CTTPCfg.RX_LOCAL;j++)
		{
			len += snprintf(buf+len, PAGE_SIZE-len, "%4d, ",CT_Base.Base[i][j]+CT_Cali.SOFTOFFSET[i][j]);
		}
		len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	}
	
	return len;
}

static ssize_t CT69x_get_Base(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	*(buf+len) = CTTPCfg.TX_LOCAL;
	len++;
	*(buf+len) = CTTPCfg.RX_LOCAL;
	len++;
	
	for(i=0;i< CTTPCfg.TX_LOCAL;i++)
	{
		for(j=0;j<CTTPCfg.RX_LOCAL;j++)
		{
			*(buf+len) = (char)(((CT_Base.Base[i][j]+CT_Cali.SOFTOFFSET[i][j]) & 0xFF00)>>8);
			len++;
			*(buf+len) = (char)((CT_Base.Base[i][j]+CT_Cali.SOFTOFFSET[i][j]) & 0x00FF);
			len++;
		}
	}
	return len;

}

static ssize_t CT69x_get_adbDiff(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len, "Diff: \n");
	for(i=0;i< CTTPCfg.TX_LOCAL;i++)
	{
		for(j=0;j<CTTPCfg.RX_LOCAL;j++)
		{
			len += snprintf(buf+len, PAGE_SIZE-len, "%4d, ",adbDiff[i][j]);
		}
		len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	}
	
	return len;
}

static ssize_t CT69x_get_Diff(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	*(buf+len) = CTTPCfg.TX_LOCAL;
	len++;
	*(buf+len) = CTTPCfg.RX_LOCAL;
	len++;
	
	for(i=0;i< CTTPCfg.TX_LOCAL;i++)
	{
		for(j=0;j<CTTPCfg.RX_LOCAL;j++)
		{
			*(buf+len) = (char)((adbDiff[i][j] & 0xFF00)>>8);
			len++;
			*(buf+len) = (char)(adbDiff[i][j] & 0x00FF);
			len++;
		}
	}
	return len;
}

static ssize_t CT69x_get_FreqScan(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i;
	ssize_t len = 0;

	for(i=0;i< 32;i++)
	{
		//*(buf+len) = (char)((CTDeltaData[i] & 0xFF00)>>8);
		//len++;
		//*(buf+len) = (char)(CTDeltaData[i] & 0x00FF);
		//len++;
		len += snprintf(buf+len, PAGE_SIZE-len, "%4d, ",CTDeltaData[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	return len;
}

static ssize_t CT69x_Set_FreqScan(struct device* cd, struct device_attribute *attr,
		       const char* buf, size_t len)
{
#ifndef INTMODE
	struct CT69x_ts_data *data = i2c_get_clientdata(this_client);
#endif
	unsigned long Basefreq = simple_strtoul(buf, NULL, 10);

	if(Basefreq < 16)
	{
	#ifdef INTMODE
		ctp_disable_irq();
		CT69x_Sleep();
		suspend_flag = 1;
		CT_Sleep(50);

		FreqScan(Basefreq);

		CT69x_TP_Reinit();
		ctp_enable_irq();
		suspend_flag = 0;
	#else
		suspend_flag = 1;
		CT_Sleep(200);

		FreqScan(Basefreq);

		CT69x_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;

		data->touch_timer.expires = jiffies + HZ/CTTPCfg.FAST_FRAME;
		add_timer(&data->touch_timer);
	#endif
	}
	return len;
}

static ssize_t CT69x_get_reg(struct device* cd,struct device_attribute *attr, char* buf)
{
#ifndef INTMODE
	struct CT69x_ts_data *data = i2c_get_clientdata(this_client);
#endif
	u8 reg_val[128];
	ssize_t len = 0;
	u8 i;

	if(suspend_flag != 1)
	{
	#ifdef INTMODE
		ctp_disable_irq();
		CT69x_Sleep();
		suspend_flag = 1;
		CT_Sleep(50);

		CT_I2C_ReadXByte(reg_val,0,127);
		
		CT69x_TP_Reinit();
		ctp_enable_irq();
		suspend_flag = 0;
	#else
		suspend_flag = 1;
		
		CT_Sleep(50);
				
		CT_I2C_ReadXByte(reg_val,0,127);

		CT69x_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;

		data->touch_timer.expires = jiffies + HZ/CTTPCfg.FAST_FRAME;
		add_timer(&data->touch_timer);
	#endif
	}
	else
	{
		CT_I2C_ReadXByte(reg_val,0,127);
	}
	
	for(i=0;i<0x7F;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg%02X = 0x%02X, ", i,reg_val[i]);
	}

	return len;

}

static ssize_t CT69x_write_reg(struct device* cd,struct device_attribute *attr, const char *buf, size_t count)
{
#ifndef INTMODE
	struct CT69x_ts_data *data = i2c_get_clientdata(this_client);
#endif
	int databuf[2];
	
	if(2 == sscanf(buf, "%d %d", &databuf[0], &databuf[1]))
	{ 
		if(suspend_flag != 1)
		{
		#ifdef INTMODE
			ctp_disable_irq();
			CT69x_Sleep();
			suspend_flag = 1;
			CT_Sleep(50);

			CT_I2C_WriteByte((u8)databuf[0],(u8)databuf[1]);
			
			CT69x_TP_Reinit();
			ctp_enable_irq();
			suspend_flag = 0;
		#else
			suspend_flag = 1;
			CT_Sleep(50);
			
			CT_I2C_WriteByte((u8)databuf[0],(u8)databuf[1]);

			CT69x_TP_Reinit();
			tp_idlecnt = 0;
			tp_SlowMode = 0;
			suspend_flag = 0;
			data->touch_timer.expires = jiffies + HZ/CTTPCfg.FAST_FRAME;
			add_timer(&data->touch_timer);
		#endif
		}
		else
		{
			CT_I2C_WriteByte((u8)databuf[0],(u8)databuf[1]);
		}
	}
	else
	{
		printk("invalid content: '%s', length = %d\n", buf, count);
	}
	return count; 
}

static ssize_t CT69x_GetUcf(struct device* cd,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;
	
	len += snprintf(buf+len, PAGE_SIZE-len,"*****CT69x UCF DATA*****\n");
	
	len += snprintf(buf+len, PAGE_SIZE-len,"PAGE1:\n");
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,\n",CTTPCfg.CHIPVER);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",CTTPCfg.TX_LOCAL,CTTPCfg.RX_LOCAL);
	len += snprintf(buf+len, PAGE_SIZE-len,"(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d}\n",
		CTTPCfg.TX_ORDER[0],CTTPCfg.TX_ORDER[1],CTTPCfg.TX_ORDER[2],CTTPCfg.TX_ORDER[3],CTTPCfg.TX_ORDER[4],
		CTTPCfg.TX_ORDER[5],CTTPCfg.TX_ORDER[6],CTTPCfg.TX_ORDER[7],CTTPCfg.TX_ORDER[8],CTTPCfg.TX_ORDER[9],
		CTTPCfg.TX_ORDER[10],CTTPCfg.TX_ORDER[11],CTTPCfg.TX_ORDER[12],CTTPCfg.TX_ORDER[13],CTTPCfg.TX_ORDER[14],
		CTTPCfg.TX_ORDER[15],CTTPCfg.TX_ORDER[16],CTTPCfg.TX_ORDER[17],CTTPCfg.TX_ORDER[19],CTTPCfg.TX_ORDER[19],
		CTTPCfg.TX_ORDER[20],CTTPCfg.TX_ORDER[21],CTTPCfg.TX_ORDER[22],CTTPCfg.TX_ORDER[23],CTTPCfg.TX_ORDER[24],
		CTTPCfg.TX_ORDER[25]);
	len += snprintf(buf+len, PAGE_SIZE-len,"{%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d},\n",
					CTTPCfg.RX_ORDER[0],CTTPCfg.RX_ORDER[1],CTTPCfg.RX_ORDER[2],CTTPCfg.RX_ORDER[3],
					CTTPCfg.RX_ORDER[4],CTTPCfg.RX_ORDER[5],CTTPCfg.RX_ORDER[6],CTTPCfg.RX_ORDER[7],
					CTTPCfg.RX_ORDER[8],CTTPCfg.RX_ORDER[9],CTTPCfg.RX_ORDER[10],CTTPCfg.RX_ORDER[11],
					CTTPCfg.RX_ORDER[12],CTTPCfg.RX_ORDER[13]);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",CTTPCfg.RX_START,CTTPCfg.HAVE_KEY_LINE);
	len += snprintf(buf+len, PAGE_SIZE-len,"{%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d},\n",
		CTTPCfg.KeyLineValid[0],CTTPCfg.KeyLineValid[1],CTTPCfg.KeyLineValid[2],CTTPCfg.KeyLineValid[3],
		CTTPCfg.KeyLineValid[4],CTTPCfg.KeyLineValid[5],CTTPCfg.KeyLineValid[6],CTTPCfg.KeyLineValid[7],
		CTTPCfg.KeyLineValid[8],CTTPCfg.KeyLineValid[9],CTTPCfg.KeyLineValid[10],CTTPCfg.KeyLineValid[11]);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",CTTPCfg.MAPPING_MAX_X,CTTPCfg.MAPPING_MAX_Y);
	
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d,\n",CTTPCfg.GainClbDeltaMin,CTTPCfg.GainClbDeltaMax,
		CTTPCfg.RawDataDeviation,CTTPCfg.GainClbDeltaMin);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d,\n",CTTPCfg.KeyLineDeltaMin,CTTPCfg.KeyLineDeltaMax,
		CTTPCfg.CacMultiCoef,CTTPCfg.GainTestDeltaMax);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d,\n",CTTPCfg.OffsetClbExpectedMin,CTTPCfg.OffsetClbExpectedMax,
		CTTPCfg.GAIN_CLB_SEPERATE,CTTPCfg.KeyLineTestDeltaMin);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d,\n",CTTPCfg.RawDataCheckMin,CTTPCfg.RawDataCheckMax,
		CTTPCfg.FIRST_CALI,CTTPCfg.KeyLineTestDeltaMax);

	len += snprintf(buf+len, PAGE_SIZE-len,"\nPAGE2:\n");
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,0x%x,0x%x,\n",CTTPCfg.MULTI_SCANFREQ,CTTPCfg.BASE_FREQ,
		CTTPCfg.FREQ_OFFSET,CTTPCfg.WAIT_TIME);
	len += snprintf(buf+len, PAGE_SIZE-len,"0x%x,0x%x,%d,%d\n",CTTPCfg.CHAMP_CFG,CTTPCfg.POSLEVEL_TH,
		CTTPCfg.RAWDATA_DUMP_SWITCH,CTTPCfg.ESD_PROTECT);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d,\n",CTTPCfg.PEAK_TH,CTTPCfg.GROUP_TH,CTTPCfg.BIGAREA_TH,CTTPCfg.BIGAREA_CNT);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d,\n",CTTPCfg.BIGAREA_FRESHCNT,CTTPCfg.PEAK_ROW_COMPENSATE,
		CTTPCfg.PEAK_COL_COMPENSATE,CTTPCfg.PEAK_COMPENSATE_COEF);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d,\n",CTTPCfg.POINT_RELEASEHOLD,CTTPCfg.MARGIN_RELEASEHOLD,
		CTTPCfg.POINT_PRESSHOLD,CTTPCfg.KEY_PRESSHOLD);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,\n",CTTPCfg.STABLE_DELTA_X,CTTPCfg.STABLE_DELTA_Y,CTTPCfg.FIRST_DELTA);
	len += snprintf(buf+len, PAGE_SIZE-len,"CHARGE:\n");
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d\n",CTTPCfg.CHARGE_PEAK_TH,CTTPCfg.CHARGE_GROUP_TH,CTTPCfg.CHARGE_BIGAREA_TH);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d\n",CTTPCfg.CHARGE_PEAK_ROW_COMPENSATE,CTTPCfg.CHARGE_PEAK_COL_COMPENSATE,
		CTTPCfg.CHARGE_PEAK_COMPENSATE_COEF);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d\n",CTTPCfg.CHARGE_POINT_RELEASEHOLD,CTTPCfg.CHARGE_MARGIN_RELEASEHOLD,
		CTTPCfg.CHARGE_POINT_PRESSHOLD);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d\n",CTTPCfg.CHARGE_KEY_PRESSHOLD,CTTPCfg.CHARGE_STABLE_DELTA_X,
		CTTPCfg.CHARGE_STABLE_DELTA_Y);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d\n",CTTPCfg.CHARGE_FIRST_DELTA,CTTPCfg.CHARGE_SECOND_HOLD,CTTPCfg.CHARGE_SECOND_DELTA);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d\n",CTTPCfg.FREQ_JUMP,CTTPCfg.ID_LOOKUP);

	len += snprintf(buf+len, PAGE_SIZE-len,"\nPAGE3:\n");
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,\n",CTTPCfg.CACULATE_COEF);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d,%d,\n",CTTPCfg.MARGIN_COMPENSATE,CTTPCfg.MARGIN_COMP_DATA_UP,
		CTTPCfg.MARGIN_COMP_DATA_DOWN,CTTPCfg.MARGIN_COMP_DATA_LEFT,CTTPCfg.MARGIN_COMP_DATA_RIGHT);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,\n",CTTPCfg.FLYING_TH,CTTPCfg.MOVING_TH,CTTPCfg.MOVING_ACCELER);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",CTTPCfg.LCD_NOISE_PROCESS,CTTPCfg.LCD_NOISETH);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",CTTPCfg.FALSE_PEAK_PROCESS,CTTPCfg.FALSE_PEAK_TH);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,\n",CTTPCfg.DEBUG_LEVEL,CTTPCfg.FAST_FRAME,CTTPCfg.SLOW_FRAME);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,\n",CTTPCfg.MARGIN_PREFILTER,CTTPCfg.BIGAREA_HOLDPOINT);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d\n",CTTPCfg.BASE_MODE,CTTPCfg.WATER_REMOVE,CTTPCfg.INT_MODE);
	len += snprintf(buf+len, PAGE_SIZE-len,"PROXIMITY:\n");
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,\n",CTTPCfg.PROXIMITY);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d\n",CTTPCfg.PROXIMITY_LINE,CTTPCfg.PROXIMITY_TH_HIGH,CTTPCfg.PROXIMITY_TH_LOW);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d\n",CTTPCfg.PROXIMITY_TIME,CTTPCfg.PROXIMITY_CNT_HIGH,CTTPCfg.PROXIMITY_CNT_LOW);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d,%d\n",CTTPCfg.PROXIMITY_TOUCH_TH_HIGH,CTTPCfg.PROXIMITY_TOUCH_TH_LOW,
				CTTPCfg.PROXIMITY_PEAK_CNT_HIGH,CTTPCfg.PROXIMITY_PEAK_CNT_LOW);
	len += snprintf(buf+len, PAGE_SIZE-len,"%d,%d,%d\n",CTTPCfg.PROXIMITY_LAST_TIME,CTTPCfg.PROXIMITY_TOUCH_TIME,CTTPCfg.PROXIMITY_SATUATION);

	return len;

}

static ssize_t CT69x_GetCaliSt(struct device* cd,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len,  "%d",CTTP_cali_st);
	return len;

}

static ssize_t CT69x_GetTestSt(struct device* cd,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;

	if(TestErrorCode == 0)
	{
		len += snprintf(buf+len,PAGE_SIZE-len,"test OK");
	}
	else
	{
		len += snprintf(buf+len,PAGE_SIZE-len,"test FAIL%d",TestErrorCode);
	}

	return len;

}

static ssize_t CT69x_GetTest(struct device* cd,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;
	
	len += snprintf(buf+len, PAGE_SIZE-len,  "%d",CTTP_test_st);
	return len;
	
}


static ssize_t CT69x_SetTest(struct device* cd,struct device_attribute *attr, const char *buf, size_t count)
{
#ifndef INTMODE
	struct CT69x_ts_data *data = i2c_get_clientdata(this_client);
#endif
	unsigned long on_off = simple_strtoul(buf, NULL, 10);
	CTTP_test_st = 0;	// set "0" to test_st indicates the test is running

	if(on_off == 1)
	{	
	#ifdef INTMODE
		ctp_disable_irq();
		CT69x_Sleep();
		suspend_flag = 1;
		CT_Sleep(50);
		CT69x_TP_Test();

		CT69x_TP_Reinit();
		ctp_enable_irq(); 
		suspend_flag = 0;
	#else
		suspend_flag = 1;
		CT_Sleep(50);
		CT69x_TP_Test();

		CT69x_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;

		data->touch_timer.expires = jiffies + HZ/CTTPCfg.FAST_FRAME;
		add_timer(&data->touch_timer);
	#endif
	}

	CTTP_test_st = 1;	// set "1" to test_st indicates the test is finished
	
	return count;
}


static int CT69x_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);

	//TS_DBG("%s", __func__);
	
	err = device_create_file(dev, &dev_attr_cali);
	err = device_create_file(dev, &dev_attr_readreg);
	err = device_create_file(dev, &dev_attr_base);
	err = device_create_file(dev, &dev_attr_diff);
	err = device_create_file(dev, &dev_attr_adbbase);
	err = device_create_file(dev, &dev_attr_adbdiff);
	err = device_create_file(dev, &dev_attr_freqscan);
	err = device_create_file(dev, &dev_attr_getucf);
	err = device_create_file(dev, &dev_attr_cali_st);
	err = device_create_file(dev, &dev_attr_test_st);
	err = device_create_file(dev, &dev_attr_test);
	
	printk("%s ,err=%d \n", __func__,err);
	return err;
}


/*
static void CT69x_ts_release(void)
{
	short tmp;
	struct CT69x_ts_data *data = i2c_get_clientdata(this_client);

	release = 0x00;

#ifdef TOUCH_KEY_SUPPORT	
	if(key_state)
	{
		input_report_key(data->input_dev, key_code, 0);
		key_state = 0;
	}
	else
#endif
	{
		for(tmp=0; tmp< 5; tmp++)
		{
			input_mt_slot(data->input_dev, tmp);//نîëم╙╣م╣╨ل╤è???		//input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
		}
	}
	
	input_sync(data->input_dev);
	return;

}
*/

#ifdef TOUCH_KEY_SUPPORT

void CT69x_report_key(unsigned short x, unsigned short y)
{
	struct CT69x_ts_data *data = i2c_get_clientdata(this_client);

	u16 i = 0;

	for(i = 0; i < TPD_KEY_COUNT; i++) 
	{
		if((tpd_keys_dim_local[i][0] < x) && (x < tpd_keys_dim_local[i][1])
			&&(tpd_keys_dim_local[i][2] < y) && (y < tpd_keys_dim_local[i][3]))
		{
			key_code = tpd_keys_local[i];	
			input_report_key(data->input_dev, key_code, 1);
			input_sync(data->input_dev); 		
			key_state = 1;
			break;
		}
	}

}

#endif

static void CT69x_report_multitouch(void)
{
	unsigned char i;
	struct CT69x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;     
 
	int sync;
	unsigned int press;
	static unsigned int release = 0;

    
#ifdef TOUCH_KEY_SUPPORT
	if(event->x[0] >= TOUCH_MAX_HEIGHT || event->y[0] >= TOUCH_MAX_WIDTH)
	{
		CT69x_report_key(event->x[0],event->y[0]);
		return;
	}
	else
#endif
	{		
		sync = 0;  press = 0;
		for(i=0;i<event->touch_point;i++)
		{
			//printk("touch_ID=%d   ==x = %d,y = %d ====\n",event->touch_ID[i],event->x[i],event->y[i]);
			
			input_mt_slot(data->input_dev, event->touch_ID[i]);
			input_report_abs(data->input_dev,  ABS_MT_TRACKING_ID, event->touch_ID[i]);

			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[i]);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[i]);			
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);

			press |= 0x01 << event->touch_ID[i];
			sync = 1;
		}

		release &= (release ^ press);//release point flag

		for ( i = 0; i < 5; i++ ) //up
		{
			if ( release & (0x01<<i) ) 
			{
				//printk("release ====touch_ID=%d   ==x = %d,y = %d ====\n",i,event->x[i],event->y[i]);

				input_mt_slot(data->input_dev, i);
				input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, -1);
				sync = 1;
			}
		}

		release = press;
		if(sync)
		{
			input_sync(data->input_dev);
		}
	}
	

}





static int CT69x_read_data(void)
{
	struct CT69x_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	 int Pevent;
   	 int i = 0;

	if(CT69x_TouchProcess())
	{
		event->touch_point = CT69x_GetPointNum();
	
		for(i=0;i<event->touch_point;i++)
		{
			CT69x_GetPoint(&event->x[i],&event->y[i],&event->touch_ID[i],&Pevent,i);
			swap(event->x[i], event->y[i]);
			if(CTTPCfg.DEBUG_LEVEL > 0)
			{
				printk("point%d = %d,%d,%d \n",i,event->x[i],event->y[i],event->touch_ID[i] );
			}
		}

		CT69x_report_multitouch();
#if	1	    	
		if (event->touch_point == 0)
		{		
			if(tp_idlecnt <= CTTPCfg.FAST_FRAME*5)
			{
				tp_idlecnt++;
			}
			if(tp_idlecnt > CTTPCfg.FAST_FRAME*5)
			{
				tp_SlowMode = 1;
			}
			
			return 1; 
		}
		else
		{
			tp_SlowMode = 0;
			tp_idlecnt = 0;
			event->pressure = 20;
			return 0;
		}
#endif

	}
	else
	{
		return 1;
	}
}


#ifdef CHARGE_DETECT
#define CT_CHARGE_FILENAME	"sys/class/power_supply/battery/status"
#define CT_CHARGE_FILENAME1	"sys/class/power_supply/battery/voltage_now"

static void CT69x_charge_detect(struct work_struct *work)
{
	char batstatus[20],batvoltage[20];
	
	memset(batstatus,0,20);
	CT_nvram_read(CT_CHARGE_FILENAME,batstatus,20,0);
	//bobo add 2014-4-29 	
	memset(batvoltage,0,20);
	CT_nvram_read(CT_CHARGE_FILENAME1,batvoltage,20,0);	
	//
	if(CTTPCfg.DEBUG_LEVEL > 0)
	{
		printk("CT69x get batstatus %s,batvoltage %s  \n", batstatus,batvoltage);
	}
	if(memcmp(batstatus,"Charging",8) == 0 || memcmp(batstatus,"Full",4) == 0)	
	{
		CT69x_BatteryMode(0);
		CT69x_ChargeMode(1);
		
	}
#ifdef	BATLOW_DETECT
	else if(memcmp(batvoltage,"370",3) < 0)	//3.70V ！！
	{	
		CT69x_ChargeMode(0);
		CT69x_BatteryMode(1);		
	}
#endif
	else
	{
		CT69x_BatteryMode(0);	
		CT69x_ChargeMode(0);				
	}	
}

static void CT69x_charge_polling(unsigned long unuse)
{
	 struct CT69x_ts_data *CT69x_ts = i2c_get_clientdata(this_client);

	if(suspend_flag != 1)		//if suspend del timer
	{
		queue_work(CT69x_ts->ts_workqueue, &CT69x_ts->Charge_Detectwork);

		CT69x_ts->charge_detect.expires = jiffies + HZ*3;
		add_timer(&CT69x_ts->charge_detect);
	}
}
#endif

static void CT69x_tp_cali(struct work_struct *work)
{
#ifndef INTMODE
	 struct CT69x_ts_data *data = i2c_get_clientdata(this_client);
#endif
	#ifdef INTMODE
		ctp_disable_irq();
		CT69x_Sleep();
		suspend_flag = 1;
		CT_Sleep(50);

		TP_Force_Calibration();

		CT69x_TP_Reinit();
		ctp_enable_irq();
		suspend_flag = 0;
		
	#else
		suspend_flag = 1;
		CT_Sleep(50);
		
		TP_Force_Calibration();
		
		CT69x_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;

		data->touch_timer.expires = jiffies + HZ/CTTPCfg.FAST_FRAME;
		add_timer(&data->touch_timer);
	#endif
}

static void CT69x_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;
	
	ret = CT69x_read_data();

#ifdef INTMODE
	if(suspend_flag != 1)
	{
		ctp_enable_irq();
	}
#endif
	if(suspend_flag == 1)
	{
		CT69x_Sleep(); 
	}
}


#ifdef INTMODE
static irqreturn_t CT69x_ts_interrupt(int irq, void *dev_id)
{
	struct CT69x_ts_data *CT69x_ts = dev_id;
		
	ctp_disable_irq();		
	if (!work_pending(&CT69x_ts->pen_event_work)) 
	{
		queue_work(CT69x_ts->ts_workqueue, &CT69x_ts->pen_event_work);
	}

	return IRQ_HANDLED;
}
#endif


static void CT69x_tpd_polling(unsigned long unuse)
 {
	struct CT69x_ts_data *CT69x_ts = i2c_get_clientdata(this_client);
	
	if (!work_pending(&CT69x_ts->pen_event_work)) 
	{
		queue_work(CT69x_ts->ts_workqueue, &CT69x_ts->pen_event_work);
	}

	//printk("~~~bobo          polling~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");

#ifdef INTMODE 
//	ctp_enable_irq();
#else
	if(suspend_flag != 1)
	{
#ifdef AUTO_RUDUCEFRAME
		if(tp_SlowMode)
		{  	
			CT69x_ts->touch_timer.expires = jiffies + HZ/CTTPCfg.SLOW_FRAME;
		}
		else
		{
			CT69x_ts->touch_timer.expires = jiffies + HZ/CTTPCfg.FAST_FRAME;
		}
#else
		CT69x_ts->touch_timer.expires = jiffies + HZ/CTTPCfg.FAST_FRAME;

#endif
		add_timer(&CT69x_ts->touch_timer);
	}
	
#endif
 }


//????
static int CT69x_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
struct CT69x_ts_data *CT69x_ts = i2c_get_clientdata(this_client);
char i;

#ifdef INTMODE
	if(suspend_flag != 1)
	{
		//suspend_flag = 1;
		msleep(50);
		//ctp_disable_irq();
		CT69x_Sleep();
	}
#else
	if(suspend_flag != 1)
	{
		printk("CT69x SLEEP!!!");
		//suspend_flag = 1;
		msleep(50);
        del_timer(&CT69x_ts->touch_timer);
	}  
#endif
#ifdef CHARGE_DETECT
	del_timer(&CT69x_ts->charge_detect);
#endif


	flush_work(&CT69x_ts->pen_event_work);
	suspend_flag = 1;
	#ifdef INTMODE
		ctp_disable_irq();

		for(i=0;i<5;i++)
		{
			input_mt_slot(CT69x_ts->input_dev,i);
			input_report_abs(CT69x_ts->input_dev, ABS_MT_TRACKING_ID, -1);
			//input_mt_report_slot_state(CT69x_ts->input_dev, MT_TOOL_FINGER, false);
		}
		input_sync(CT69x_ts->input_dev);
	#endif
	
	return 0;
}

extern void CT69x_User_Cfg1(void);
static int CT69x_ts_resume(struct i2c_client *client)
{
	struct CT69x_ts_data *CT69x_ts = i2c_get_clientdata(this_client);
#ifdef INTMODE
	if(suspend_flag != 0)
	{
		CT69x_User_Cfg1();
		CT69x_TP_Reinit();
		suspend_flag = 0;

		ctp_enable_irq();
//		CT69x_ts->touch_timer.expires = jiffies + HZ/CTTPCfg.FAST_FRAME;
//		add_timer(&CT69x_ts->touch_timer);
#ifdef CHARGE_DETECT
		CT69x_ts->charge_detect.expires = jiffies + HZ*3;
		add_timer(&CT69x_ts->charge_detect);
#endif

	}
#else
	if(suspend_flag != 0)
	{
		CT69x_User_Cfg1();
		CT69x_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;
		printk("CT69x WAKE UP!!!");

		CT69x_ts->touch_timer.expires = jiffies + HZ/CTTPCfg.FAST_FRAME;
		add_timer(&CT69x_ts->touch_timer);
#ifdef CHARGE_DETECT
		CT69x_ts->charge_detect.expires = jiffies + HZ*3;
		add_timer(&CT69x_ts->charge_detect);
#endif

	}
#endif
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void CT69x_ts_early_suspend(struct early_suspend *h)
{
	struct CT69x_ts_data *CT69x_ts;
	CT69x_ts = container_of(h, struct CT69x_ts_data, early_suspend);
	CT69x_ts_suspend(NULL, PMSG_SUSPEND);
}

static void CT69x_ts_late_resume(struct early_suspend *h)
{
	struct CT69x_ts_data *CT69x_ts;
	CT69x_ts = container_of(h, struct CT69x_ts_data, early_suspend);
	CT69x_ts_resume(NULL);
}
#endif
 

/*******************************************************	
Function:
	Touch-screen detection function
	Called when the registration drive (required for a corresponding client);
	For IO, interrupts and other resources to apply; equipment registration; touch screen initialization, etc.
Parameters:
	client: the device structure to be driven
	id: device ID
return:
	Results of the implementation code, 0 for normal execution
********************************************************/
static int CT69x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	unsigned char reg_value; 
	struct CT69x_ts_data *CT69x_ts;
	struct ts_hw_data *pdata = client->dev.platform_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{
		dev_err(&client->dev, "Must have I2C_FUNC_I2C.\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	
	CT69x_ts = kzalloc(sizeof(*CT69x_ts), GFP_KERNEL);
	if (CT69x_ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	this_client = client;
	if(	pdata->init_platform_hw )
	{
		ret=pdata->init_platform_hw();
		if(ret!=0)
		{
			dev_err(&client->dev, "Failed to request  gpio \n");
			goto err_hw_init_failed;
		}
	}
	printk("client addr1 = %x", client->addr);
	client->addr =0x38;
	this_client->addr = client->addr;

	reg_value = CT_I2C_ReadByte(0x01);
	printk("[gll]reg_value = %x", reg_value);
	if(reg_value != 0xB8)
	//if(reg_value != 0xA8)	// 5208
	{
		client->addr = 0x39;
		dev_err(&client->dev, "CT69x_ts_probe: CHIP ID NOT CORRECT\n");
		goto err_i2c_failed;
	}
	
	i2c_set_clientdata(client, CT69x_ts);
	INIT_WORK(&CT69x_ts->pen_event_work, CT69x_ts_pen_irq_work);
	INIT_WORK(&CT69x_ts->Charge_Detectwork, CT69x_charge_detect);
	INIT_WORK(&CT69x_ts->tp_cali_work, CT69x_tp_cali);
	CT69x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!CT69x_ts->ts_workqueue) {
		ret = -ESRCH;
		goto exit_create_singlethread;
	}

	CT69x_ts->input_dev = input_allocate_device();
	if (CT69x_ts->input_dev == NULL) 
	{
		ret = -ENOMEM;
		dev_dbg(&client->dev,"CT69x_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}

	__set_bit(INPUT_PROP_DIRECT, CT69x_ts->input_dev->propbit);
	__set_bit(EV_ABS, CT69x_ts->input_dev->evbit);
	__set_bit(EV_KEY, CT69x_ts->input_dev->evbit);
	__set_bit(EV_REP, CT69x_ts->input_dev->evbit);
	
	input_mt_init_slots(CT69x_ts->input_dev, 5);
	input_set_abs_params(CT69x_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(CT69x_ts->input_dev, ABS_MT_POSITION_X, 0, TOUCH_MAX_HEIGHT, 0, 0);
	input_set_abs_params(CT69x_ts->input_dev, ABS_MT_POSITION_Y, 0, TOUCH_MAX_WIDTH, 0, 0);	

	#ifdef TOUCH_KEY_SUPPORT
	CT69x_ts->input_dev->evbit[0] = BIT_MASK(EV_KEY);
	for (i = 0; i < TPD_KEY_COUNT; i++)
		set_bit(tpd_keys_local[i], CT69x_ts->input_dev->keybit);
	#endif
	
	sprintf(CT69x_ts->phys, "input/ts");
	CT69x_ts->input_dev->name = CT69x_I2C_NAME;
	CT69x_ts->input_dev->phys = CT69x_ts->phys;
	CT69x_ts->input_dev->id.bustype = BUS_I2C;
	CT69x_ts->input_dev->dev.parent = &client->dev;

	ret = input_register_device(CT69x_ts->input_dev);
	if (ret) {
		dev_err(&client->dev,"Probe: Unable to register %s input device\n", CT69x_ts->input_dev->name);
		goto err_input_register_device_failed;
	}
	CT69x_ts->bad_data = 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
	CT69x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +1;
	CT69x_ts->early_suspend.suspend = CT69x_ts_early_suspend;
	CT69x_ts->early_suspend.resume = CT69x_ts_late_resume;
	register_early_suspend(&CT69x_ts->early_suspend);
#endif
#ifdef INTMODE
	CT69x_ts->use_irq = 1;
#endif
	dev_info(&client->dev,"Start %s in %s mode,Driver Modify Date:2012-01-05\n", 
		CT69x_ts->input_dev->name, CT69x_ts->use_irq ? "interrupt" : "polling");

	CT69x_create_sysfs(client);

	memcpy(CT_CALI_FILENAME,"/data/tpcali",12);
	memcpy(CT_UCF_FILENAME,"/data/AWTPucf",13);
	memcpy(CT_DIFF_FILENAME,"/data/awdiff",12);
  
	CT69x_TP_Init();  

	CT69x_ts->touch_timer.function = CT69x_tpd_polling;
	CT69x_ts->touch_timer.data = 0;
	init_timer(&CT69x_ts->touch_timer);
	CT69x_ts->touch_timer.expires = jiffies + HZ*10;
	add_timer(&CT69x_ts->touch_timer);	
#ifdef CHARGE_DETECT
	CT69x_ts->charge_detect.function = CT69x_charge_polling;
	CT69x_ts->charge_detect.data = 0;
	init_timer(&CT69x_ts->charge_detect);
	CT69x_ts->charge_detect.expires = jiffies + HZ*30;
	add_timer(&CT69x_ts->charge_detect);
#endif

#ifdef INTMODE
	//CT69x_ts->irq = gpio_to_irq(INT_PORT);//client->irq;
	CT69x_ts->irq = gpio_to_irq(pdata->touch_en_gpio);
	ret=  request_irq(CT69x_ts->irq, CT69x_ts_interrupt, /*IRQF_TRIGGER_RISING*/ IRQF_TRIGGER_FALLING, client->name, CT69x_ts);
	if (ret < 0) {
		printk( "CT69x_probe: request irq failed\n");
		goto error_req_irq_fail;
	}
#endif

	printk("==probe over =\n");
	return 0;

error_req_irq_fail:
    free_irq(CT69x_ts->irq, CT69x_ts);	
err_input_register_device_failed:
	input_free_device(CT69x_ts->input_dev);

err_input_dev_alloc_failed:
	i2c_set_clientdata(client, NULL);
exit_create_singlethread:
err_i2c_failed:	
err_hw_init_failed:
	kfree(CT69x_ts);	
err_alloc_data_failed:
err_check_functionality_failed:

	return ret;
}
                                                                                                                       
/*******************************************************	
Function:
	Drive the release of resources
Parameters:
	client: the device structure
return:
	Results of the implementation code, 0 for normal execution
********************************************************/
static int CT69x_ts_remove(struct i2c_client *client)
{
	struct CT69x_ts_data *CT69x_ts = i2c_get_clientdata(client);
	
	pr_info("==CT69x_ts_remove=\n");
#ifdef INTMODE
	free_irq(CT69x_ts->irq, CT69x_ts);
#else
	del_timer(&CT69x_ts->touch_timer);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&CT69x_ts->early_suspend);
#endif
	input_unregister_device(CT69x_ts->input_dev);
	input_free_device(CT69x_ts->input_dev);
	cancel_work_sync(&CT69x_ts->pen_event_work);
	destroy_workqueue(CT69x_ts->ts_workqueue);
	kfree(CT69x_ts);
    
	i2c_set_clientdata(client, NULL);
	//ctp_ops.free_platform_resource();

	return 0;
}

                                                                                                                             
//???????????????ID ??                                                                              
//only one client                                                                                                               
static const struct i2c_device_id CT69x_ts_id[] = {                                                                            
	{ CT69x_I2C_NAME, 0 },                                                                                                       
	{ }                                                                                                                           
};                                                                                                                              
                                                                                                                                
//?????????                                                                                                         
static struct i2c_driver CT69x_ts_driver = {                                                                                   
	.probe		= CT69x_ts_probe,                                                                                                  
	.remove		= CT69x_ts_remove,                                                                                                 
#ifndef CONFIG_HAS_EARLYSUSPEND                                                                                                 
	.suspend	= CT69x_ts_suspend,                                                                                                
	.resume		= CT69x_ts_resume,                                                                                                 
#endif                                                                                                                          
	.id_table	= CT69x_ts_id,                                                                                                     
	.driver = {                                                                                                                   
		.name	= CT69x_I2C_NAME,                                                                                                    
		.owner = THIS_MODULE,                                                                                                       
	},                                                                                                                            
}; 

/*******************************************************	
?????	??????
return???	??????0??????
********************************************************/
static int __devinit CT69x_ts_init(void)
{
	int ret;
	ret=i2c_add_driver(&CT69x_ts_driver);
	printk("[gll] ret = %d\n",ret);
	return ret; 
}

/*******************************************************	
?????	??????
?????	client??????
********************************************************/
static void __exit CT69x_ts_exit(void)
{
	printk(KERN_ALERT "Touchscreen driver of guitar exited.\n");
	i2c_del_driver(&CT69x_ts_driver);
}


late_initcall(CT69x_ts_init); 
module_exit(CT69x_ts_exit);

MODULE_DESCRIPTION("CT69x Touchscreen Driver");
MODULE_LICENSE("GPL");
               

