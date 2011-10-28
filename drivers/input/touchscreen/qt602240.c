//[*]--------------------------------------------------------------------------------------------------[*]
//
//
// 
//  HardKernel(HKC1XX) Touch driver (charles.park)
//  2009.07.22
// 
//
//[*]--------------------------------------------------------------------------------------------------[*]
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/fs.h>

#include <mach/irqs.h>
#include <asm/system.h>

#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-bank.h>

//[*]--------------------------------------------------------------------------------------------------[*]
#include "hkc1xx_touch.h"
#include "hkc1xx_touch_gpio_i2c.h"
#include "hkc1xx_touch_sysfs.h"

#include "qt602240.h"

//[*]--------------------------------------------------------------------------------------------------[*]
qt602240_t				qt602240;

//[*]--------------------------------------------------------------------------------------------------[*]
unsigned char	get_object_size							(unsigned char type);
unsigned int	get_object_address						(unsigned char type, unsigned char instance);
static	void	qt602240_power_config					(void);
static	void	qt602240_acquisition_config				(void);
static	void	qt602240_multi_touchscreen_init			(void);
static	void	qt602240_key_arrary_init				(void);
static	void	qt602240_comc_config					(void);
static	void	qt602240_gpio_pwm_init					(void);
static	void	qt602240_grip_face_suppression_config	(void);
static	void	qt602240_noise_suppression_config		(void);
static	void	qt602240_proximity_config				(void);
static	void	qt602240_one_touch_gesture_config		(void);
static	void	qt602240_self_test_init					(void);
static	void	qt602240_tow_touch_gesture_config		(void);
static	void	qt602240_cte_config						(void);
static	void	qt602240_backup_config					(void);
static	void	qt602240_calibrate_chip					(void);
static	void	qt602240_soft_reset						(void);
		int		qt602240_set_config						(void);
		int		qt602240_init							(void);

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static	int		qt602240_config_write					(unsigned int type, unsigned int instance, unsigned char *wd)
{
	if((get_object_address(type, instance) == 0) || (get_object_size(type) == 0))	{
		printk("%s(%d) Error! address = 0x%04X, size = 0x%04X\n", __FUNCTION__,__LINE__, get_object_address(type, instance), get_object_size(type));	
		return	-1;
	}

    return	hkc1xx_touch_write_mem(get_object_address(type, instance), wd, get_object_size(type));
}
//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_power_config					(void)
{
	power_config_t	power_config;
	
    /* Set Idle Acquisition Interval to 32 ms. */
    power_config.idleacqint = 255;

    /* Set Active Acquisition Interval to 16 ms. */
    power_config.actvacqint = 255;

    /* Set Active to Idle Timeout to 4 s (one unit = 200ms). */
    power_config.actv2idleto = 50;

	if(qt602240_config_write(GEN_POWERCONFIG_T7, 0, (unsigned char *)&power_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_acquisition_config				(void)
{
	acquisition_config_t	acquisition_config;
	
    acquisition_config.chrgtime 	= 8;	    acquisition_config.reserved 	= 0;
    acquisition_config.tchdrift 	= 5;	    acquisition_config.driftst 		= 5;
    acquisition_config.tchautocal 	= 0;	    acquisition_config.sync 		= 0;
    acquisition_config.atchcalst 	= 10;	    acquisition_config.atchcalsthr 	= 15;

	if(qt602240_config_write(GEN_ACQUISITIONCONFIG_T8, 0, (unsigned char *)&acquisition_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_multi_touchscreen_init			(void)
{
	touchscreen_config_t	touchscreen_config;
	
    touchscreen_config.ctrl 		= 0x83; // enable + message-enable
    touchscreen_config.xorigin 		= 0;	    touchscreen_config.yorigin 		= 0;
    touchscreen_config.xsize 		= 18;	    touchscreen_config.ysize 		= 11;
    touchscreen_config.akscfg 		= 0;	    touchscreen_config.blen 		= 1;
    touchscreen_config.tchthr 		= 30;	    touchscreen_config.tchdi 		= 2;
    touchscreen_config.orient 		= 5;	// charles
    
    touchscreen_config.mrgtimeout 	= 0;	    touchscreen_config.movhysti 	= 1;
    touchscreen_config.movhystn 	= 1;	    touchscreen_config.movfilter 	= 0;
    touchscreen_config.numtouch		= TS_MAX_FINGER; 	    
    touchscreen_config.mrghyst 		= 10;
  
    touchscreen_config.mrgthr 		= 10;	    touchscreen_config.amphyst 		= 10;

    touchscreen_config.xrange 		= 0;	    touchscreen_config.yrange 		= 0;
    touchscreen_config.xloclip 		= 0;	    touchscreen_config.xhiclip 		= 0;
    touchscreen_config.yloclip 		= 0;	    touchscreen_config.yhiclip 		= 0;
    
    touchscreen_config.xedgectrl 	= 0;	    touchscreen_config.xedgedist 	= 0;
    touchscreen_config.yedgectrl 	= 0;	    touchscreen_config.yedgedist 	= 0;

	if(qt602240_config_write(TOUCH_MULTITOUCHSCREEN_T9, 0, (unsigned char *)&touchscreen_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_key_arrary_init				(void)
{
	keyarray_config_t	keyarray_config;
	
    keyarray_config.ctrl 	= 3;	    keyarray_config.xorigin = 14;	//keyarray_config.xorigin = 0;
    keyarray_config.yorigin = 11;	    keyarray_config.xsize 	= 4;
    keyarray_config.ysize 	= 1;	    keyarray_config.akscfg 	= 0;
//    keyarray_config.blen 	= 0x40;	    keyarray_config.tchthr 	= 30;
    keyarray_config.blen 	= 0;	    keyarray_config.tchthr 	= 30;
    keyarray_config.tchdi 	= 2;
    
    keyarray_config.reserved[0] = 0;    keyarray_config.reserved[1] = 0;

	if(qt602240_config_write(TOUCH_KEYARRAY_T15, 0, (unsigned char *)&keyarray_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_comc_config					(void)
{
	comc_config_t	comc_config;
	
    comc_config.ctrl 	= 0x01;		comc_config.cmd 	= 0x01;

	if(qt602240_config_write(SPT_COMCONFIG_T18, 0, (unsigned char *)&comc_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_gpio_pwm_init					(void)
{
	gpiopwm_config_t	gpiopwm_config;
	
    gpiopwm_config.ctrl 		= 0;	    gpiopwm_config.reportmask 	= 0;
    gpiopwm_config.dir 			= 0;	    gpiopwm_config.intpullup 	= 0;
    gpiopwm_config.out 			= 0;	    gpiopwm_config.wake 		= 0;
    gpiopwm_config.pwm 			= 0;	    gpiopwm_config.period 		= 0;
    
    gpiopwm_config.duty[0] 		= 0;	    gpiopwm_config.duty[1] 		= 0;
    gpiopwm_config.duty[2] 		= 0;	    gpiopwm_config.duty[3] 		= 0;

	if(qt602240_config_write(SPT_GPIOPWM_T19, 0, (unsigned char *)&gpiopwm_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_grip_face_suppression_config	(void)
{
	gripfacesuppression_config_t	gripfacesuppression_config;
	
    gripfacesuppression_config.ctrl 	= 0;
    gripfacesuppression_config.xlogrip 	= 0;	    gripfacesuppression_config.xhigrip 	= 0;
    gripfacesuppression_config.ylogrip 	= 0;	    gripfacesuppression_config.yhigrip 	= 0;
    gripfacesuppression_config.maxtchs 	= 0;	    gripfacesuppression_config.reserved = 0;
    
    gripfacesuppression_config.szthr1 	= 0;	    gripfacesuppression_config.szthr2 	= 0;
    gripfacesuppression_config.shpthr1 	= 0;    	gripfacesuppression_config.shpthr2 	= 0;
    gripfacesuppression_config.supextto = 0;

	if(qt602240_config_write(PROCI_GRIPFACESUPPRESSION_T20, 0, (unsigned char *)&gripfacesuppression_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_noise_suppression_config		(void)
{
	noise_suppression_config_t	noise_suppression_config;

    noise_suppression_config.ctrl 		= 7;   

    noise_suppression_config.reserved 	= 0;    noise_suppression_config.reserved1 	= 0;

    noise_suppression_config.gcaful = 25;	    noise_suppression_config.gcafll = 25;
    
    noise_suppression_config.actvgcafvalid 	= 16;

	noise_suppression_config.noisethr 		= 30;
	noise_suppression_config.freqhopscale	= 1;
	
#if 0	
    noise_suppression_config.freq[0] = 10;    	noise_suppression_config.freq[1] = 15;
    noise_suppression_config.freq[2] = 20;		noise_suppression_config.freq[3] = 25;
    noise_suppression_config.freq[4] = 30;
#endif    
    noise_suppression_config.freq[0] = 5; 		noise_suppression_config.freq[1] = 10;
    noise_suppression_config.freq[2] = 15;    	noise_suppression_config.freq[3] = 20;
    noise_suppression_config.freq[4] = 25;

    noise_suppression_config.idlegcafvalid 	= 16;

	if(qt602240_config_write(PROCG_NOISESUPPRESSION_T22, 0, (unsigned char *)&noise_suppression_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_proximity_config				(void)
{
	proximity_config_t	proximity_config;
	
    proximity_config.ctrl 		= 0;	    proximity_config.xorigin 	= 0;
    proximity_config.xsize 		= 0;	    proximity_config.ysize 		= 0;
    
    proximity_config.reserved_for_future_aks_usage = 0;
    
    proximity_config.blen 		= 0;	    proximity_config.tchthr 	= 0;
    proximity_config.tchdi 		= 0;	    proximity_config.average 	= 0;
    proximity_config.rate 		= 0;

	if(qt602240_config_write(TOUCH_PROXIMITY_T23, 0, (unsigned char *)&proximity_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_one_touch_gesture_config		(void)
{
	onetouch_gesture_config_t	onetouch_gesture_config;
	
    /* Disable one touch gestures. */
    onetouch_gesture_config.ctrl 		= 0;    onetouch_gesture_config.numgest 	= 0;

    onetouch_gesture_config.gesten 		= 0;    onetouch_gesture_config.pressproc 	= 0;
    onetouch_gesture_config.tapto 		= 0;    onetouch_gesture_config.flickto 	= 0;
    onetouch_gesture_config.dragto 		= 0;    onetouch_gesture_config.spressto 	= 0;
    onetouch_gesture_config.lpressto 	= 0;    onetouch_gesture_config.reppressto 	= 0;
    onetouch_gesture_config.flickthr 	= 0;    onetouch_gesture_config.dragthr 	= 0;
    onetouch_gesture_config.tapthr 		= 0;    onetouch_gesture_config.throwthr 	= 0;

	if(qt602240_config_write(PROCI_ONETOUCHGESTUREPROCESSOR_T24, 0, (unsigned char *)&onetouch_gesture_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_self_test_init					(void)
{
	selftest_config_t	selftest_config;
	
    selftest_config.ctrl = 0;    selftest_config.cmd = 0;

	if(qt602240_config_write(SPT_SELFTEST_T25, 0, (unsigned char *)&selftest_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_tow_touch_gesture_config		(void)
{
	twotouch_gesture_config_t	twotouch_gesture_config;
	
    /* Disable two touch gestures. */
    twotouch_gesture_config.ctrl 		= 0;    twotouch_gesture_config.numgest 	= 0;
    twotouch_gesture_config.reserved2 	= 0;    twotouch_gesture_config.gesten 		= 0;
    twotouch_gesture_config.rotatethr 	= 0;    twotouch_gesture_config.zoomthr 	= 0;

	if(qt602240_config_write(PROCI_TWOTOUCHGESTUREPROCESSOR_T27, 0, (unsigned char *)&twotouch_gesture_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_cte_config						(void)
{
	cte_config_t	cte_config;
	
    /* Set CTE config */
    cte_config.ctrl 	= 0;	 		cte_config.cmd 		= 0;
    cte_config.mode 	= 2;

	cte_config.idlegcafdepth = 4;    	cte_config.actvgcafdepth = 8;

	if(qt602240_config_write(SPT_CTECONFIG_T28, 0, (unsigned char *)&cte_config))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_backup_config					(void)
{
    /* Write 0x55 to BACKUPNV register to initiate the backup. */
    unsigned char	data[2];

    data[0] = 0x55;

 	if(hkc1xx_touch_write_mem(get_object_address(GEN_COMMANDPROCESSOR_T6, 0) + BACKUP_OFFSET, (unsigned char *)&data[0], 1))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
 	}
 	msleep(100);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_calibrate_chip					(void)
{
    unsigned char	data[2];

    data[0] = 0x01;

 	if(hkc1xx_touch_write_mem(get_object_address(GEN_COMMANDPROCESSOR_T6, 0) + CALIBRATE_OFFSET, (unsigned char *)&data[0], 1))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
 	}
 	msleep(100);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void	qt602240_soft_reset						(void)
{
    unsigned char	data[2];

    data[0] = 0xA5;

 	if(hkc1xx_touch_write_mem(get_object_address(GEN_COMMANDPROCESSOR_T6, 0) + RESET_OFFSET, (unsigned char *)&data[0], 1))	{
		printk("%s(%d) write error!!\n", __FUNCTION__,__LINE__);	return;
 	}
 	msleep(50);
}

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
unsigned char	get_object_size(unsigned char type)
{
    unsigned char	table_index = 0;

    while (table_index < qt602240.info.object_num)
    {
        /* Does object type match? */
        if (qt602240.pobject[table_index].type == type)
        {
        	return	(qt602240.pobject[table_index].size + 1);
        }
        table_index++;
    }

    return	0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
unsigned int	get_object_address(unsigned char type, unsigned char instance)
{
    unsigned char	table_index = 0;

    while (table_index < qt602240.info.object_num)
    {
        /* Does object type match? */
        if (qt602240.pobject[table_index].type == type)
        {
            /* Are there enough instances defined in the FW? */
            if (qt602240.pobject[table_index].instances >= instance)
            {
                return	(qt602240.pobject[table_index].start_address + (qt602240.pobject[table_index].size + 1) * instance);
            }
        }
        table_index++;
    }

    return	0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
int		qt602240_set_config(void)
{
	// Touchscreen Config send
	qt602240_power_config					();		qt602240_acquisition_config				();
	qt602240_multi_touchscreen_init			();		qt602240_key_arrary_init				();
	qt602240_comc_config					();		qt602240_gpio_pwm_init					();
	qt602240_grip_face_suppression_config	();		qt602240_noise_suppression_config		();
	qt602240_proximity_config				();		qt602240_one_touch_gesture_config		();
	qt602240_self_test_init					();		qt602240_tow_touch_gesture_config		();
	qt602240_cte_config						();		qt602240_backup_config					();
	qt602240_calibrate_chip					();		qt602240_soft_reset						();

	while((!GET_INT_STATUS()) || (qt602240.pmessage[0] != 0xFF))	{
		if(!hkc1xx_touch_read_mem(get_object_address(GEN_MESSAGEPROCESSOR_T5, 0), &qt602240.pmessage[0], qt602240.max_message_length))	{
			#if	defined(DEBUG_MSG)
				int	i;
				for(i=0; i<qt602240.max_message_length; i++)	printk("0x%02X ", qt602240.pmessage[i]);
			#endif
		}
	}
	
	printk("=========== QT602240 Touchscreen Information ===========\n");
	printk("Family ID     = 0x%02X\n", qt602240.info.family_id);
	printk("Variant ID    = 0x%02X\n", qt602240.info.variant_id);
	printk("Build Num     = 0x%02X\n", qt602240.info.build);
	printk("F/W Version   = 0x%02X\n", qt602240.info.version);
	printk("Matrix X Size = 0x%02X\n", qt602240.info.matrix_xsize);
	printk("Matrix Y Size = 0x%02X\n", qt602240.info.matrix_ysize);
	printk("Object Count  = 0x%02X\n", qt602240.info.object_num);
	printk("Max Report IDs     = 0x%02X\n", qt602240.max_report_id);
	printk("Max Message length = 0x%02X\n", qt602240.max_message_length);
	printk("========================================================\n");
	return	0;
}
//[*]--------------------------------------------------------------------------------------------------[*]
int		qt602240_init(void)
{
	if(hkc1xx_touch_read_mem(0, (unsigned char *)&qt602240.info, sizeof(qt602240_info_t)))	{
    	printk("Read Fail!!(%d)\n" , __LINE__);		return	-1;
	}
	
	if((qt602240.pobject = (qt602240_object_t *)kcalloc(qt602240.info.object_num, sizeof(qt602240_object_t), GFP_KERNEL)) == NULL)	{
		printk("Memory Allocation Fail!!(%d)\n" , __LINE__);	return	-1;
	}

    if(!hkc1xx_touch_read_mem(OBJECT_TABLE_START_ADDRESS, (unsigned char *)&qt602240.pobject[0], sizeof(qt602240_object_t) * qt602240.info.object_num))	{
		int	i;
    	for(i = 0; i < qt602240.info.object_num; i++)	{
    		#if defined(DEBUG_MSG)
				printk("=========== OBJECT Number %d ===========\n", i);
				printk("TYPE          = 0x%02X\n", qt602240.pobject[i].type);
				printk("Start Address = 0x%04X\n", qt602240.pobject[i].start_address);
				printk("Size          = 0x%02X\n", qt602240.pobject[i].size);
				printk("Instances     = 0x%02X\n", qt602240.pobject[i].instances);
				printk("Report IDs    = 0x%02X\n", qt602240.pobject[i].num_report_ids);
			#endif

			qt602240.max_report_id += qt602240.pobject[i].num_report_ids;
	        if (qt602240.pobject[i].type == GEN_MESSAGEPROCESSOR_T5)
	        {
	            qt602240.max_message_length = qt602240.pobject[i].size + 1;
	        }
    	}
    	
    	if((qt602240.pmessage = (unsigned char *)kcalloc(qt602240.max_message_length, sizeof(unsigned char), GFP_KERNEL)) == NULL)	{
    		kfree(qt602240.pobject);
			printk("Memory Allocation Fail!!(%d)\n" , __LINE__);	return	-1;
    	}
    }
    else	{
    	printk("Read Fail!!(%d)\n" , __LINE__);		return	-1;
    }

	if((qt602240.preport_id_map = (qt602240_report_id_map_t *)kcalloc(qt602240.max_report_id+1, sizeof(qt602240_report_id_map_t), GFP_KERNEL)) == NULL)	{
		kfree(qt602240.pobject);		kfree(qt602240.pmessage);
		printk("Memory Allocation Fail!!(%d)\n" , __LINE__);	return	-1;
	}	
	else	{
		unsigned int	current_report_id = 1, i;
	    
	    qt602240.preport_id_map[0].instance = 0;
	    qt602240.preport_id_map[0].type = 0;
	
	    for (i = 0; i < qt602240.info.object_num; i++)
	    {
	        if (qt602240.pobject[i].num_report_ids != 0)
	        {
	            int instance;
	            for (instance = 0; instance <= qt602240.pobject[i].instances; instance++)
	            {
	                int start_report_id = current_report_id;
	                for (; current_report_id < (start_report_id + qt602240.pobject[i].num_report_ids); current_report_id++)
	                {
	                    qt602240.preport_id_map[current_report_id].instance = instance;
	                    qt602240.preport_id_map[current_report_id].type = qt602240.pobject[i].type;
	                    #if defined(DEBUG_MSG)
	                    	printk("report id[%d] map : instance = 0x%04X, type = 0x%02X\n",current_report_id, instance, qt602240.pobject[i].type);
	                    #endif
	                }
	            }
	        }
	    }
	}
	
	return	(qt602240_set_config());
}
//[*]--------------------------------------------------------------------------------------------------[*]
