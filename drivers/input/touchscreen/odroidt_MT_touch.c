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

//[*]--------------------------------------------------------------------------------------------------[*]
#ifdef CONFIG_HAS_EARLYSUSPEND
	#include <linux/wakelock.h>
	#include <linux/earlysuspend.h>
	#include <linux/suspend.h>
#endif

//[*]--------------------------------------------------------------------------------------------------[*]
#include "hkc1xx_touch.h"
#include "hkc1xx_touch_gpio_i2c.h"
#include "hkc1xx_touch_sysfs.h"

//[*]--------------------------------------------------------------------------------------------------[*]
//#define	DEBUG_HKC1XX_TOUCH_MSG
#define	DEBUG_HKC1XX_TOUCH_PM_MSG

//[*]--------------------------------------------------------------------------------------------------[*]
hkc1xx_touch_t	hkc1xx_touch;

//[*]--------------------------------------------------------------------------------------------------[*]
static void 			hkc1xx_touch_process_data	(touch_process_data_t *ts_data);
static void 			hkc1xx_touch_get_data		(void);

irqreturn_t				hkc1xx_touch_irq			(int irq, void *dev_id);

static int              hkc1xx_touch_open			(struct input_dev *dev);
static void             hkc1xx_touch_close			(struct input_dev *dev);

static void             hkc1xx_touch_release_device	(struct device *dev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	static void				hkc1xx_touch_early_suspend	(struct early_suspend *h);
	static void				hkc1xx_touch_late_resume	(struct early_suspend *h);
#else
	static int              hkc1xx_touch_resume			(struct platform_device *dev);
	static int              hkc1xx_touch_suspend		(struct platform_device *dev, pm_message_t state);
#endif

static void				hkc1xx_touch_reset			(void);
static void 			hkc1xx_touch_config			(unsigned char state);

static int __devinit    hkc1xx_touch_probe			(struct platform_device *pdev);
static int __devexit    hkc1xx_touch_remove			(struct platform_device *pdev);

static int __init       hkc1xx_touch_init			(void);
static void __exit      hkc1xx_touch_exit			(void);

//[*]--------------------------------------------------------------------------------------------------[*]
static struct platform_driver hkc1xx_touch_platform_device_driver = {
		.probe          = hkc1xx_touch_probe,
		.remove         = hkc1xx_touch_remove,

#ifndef CONFIG_HAS_EARLYSUSPEND
		.suspend        = hkc1xx_touch_suspend,
		.resume         = hkc1xx_touch_resume,
#endif
		.driver		= {
			.owner	= THIS_MODULE,
			.name	= HKC1XX_TOUCH_DEVICE_NAME,
		},
};

//[*]--------------------------------------------------------------------------------------------------[*]
static struct platform_device hkc1xx_touch_platform_device = {
        .name           = HKC1XX_TOUCH_DEVICE_NAME,
        .id             = -1,
        .num_resources  = 0,
        .dev    = {
                .release	= hkc1xx_touch_release_device,
        },
};

//[*]--------------------------------------------------------------------------------------------------[*]
module_init(hkc1xx_touch_init);
module_exit(hkc1xx_touch_exit);

//[*]--------------------------------------------------------------------------------------------------[*]
MODULE_AUTHOR("HardKernel");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("10.1\" WXGA Touch panel for HKC1xx-Dev Board");

//[*]--------------------------------------------------------------------------------------------------[*]
static void hkc1xx_touch_process_data(touch_process_data_t *ts_data)
{
	// read address setup
	hkc1xx_touch_write(0x00, NULL, 0x00);

	// Acc data read
	write_seqlock(&hkc1xx_touch.lock);
	hkc1xx_touch_read(&hkc1xx_touch.rd[0], 10);
	write_sequnlock(&hkc1xx_touch.lock);

	ts_data->finger_cnt = hkc1xx_touch.rd[0] & 0x03;

	if((ts_data->x1 = ((hkc1xx_touch.rd[3] << 8) | hkc1xx_touch.rd[2])))	{
		ts_data->x1 = (ts_data->x1 * 133) / 100;
		ts_data->x1 = TS_ABS_MAX_X - ts_data->x1;	// flip X & resize
	}
		
	if((ts_data->y1 = ((hkc1xx_touch.rd[5] << 8) | hkc1xx_touch.rd[4])))	{
		ts_data->y1 = (ts_data->y1 * 128 ) / 100;	// resize
	}
	
	if(ts_data->finger_cnt > 1)	{
		if((ts_data->x2 = ((hkc1xx_touch.rd[7] << 8) | hkc1xx_touch.rd[6])))	{
			ts_data->x2 = (ts_data->x2 * 133) / 100;
			ts_data->x2 = TS_ABS_MAX_X - ts_data->x2;	// flip X & resize
		}
		
		if((ts_data->y2 = ((hkc1xx_touch.rd[9] << 8) | hkc1xx_touch.rd[8])))	{
			ts_data->y2 = (ts_data->y2 * 128 ) / 100;	// resize
		}
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void hkc1xx_touch_get_data(void)
{
//	unsigned int	temp_x, temp_y, cnt;
	touch_process_data_t	ts_data;
	
	hkc1xx_touch_process_data(&ts_data);
		
	if(ts_data.finger_cnt > 0 && ts_data.finger_cnt < 3)	{
		hkc1xx_touch.x 	= ts_data.x1;	hkc1xx_touch.y	= ts_data.y1;
		input_report_abs(hkc1xx_touch.driver, ABS_MT_TOUCH_MAJOR, 200);   // press               
		input_report_abs(hkc1xx_touch.driver, ABS_MT_WIDTH_MAJOR, 10);
		input_report_abs(hkc1xx_touch.driver, ABS_MT_POSITION_X, hkc1xx_touch.x);
		input_report_abs(hkc1xx_touch.driver, ABS_MT_POSITION_Y, hkc1xx_touch.y);
		input_mt_sync(hkc1xx_touch.driver);
		
		if(ts_data.finger_cnt > 1)	{
			hkc1xx_touch.x 	= ts_data.x2;	hkc1xx_touch.y	= ts_data.y2;
			input_report_abs(hkc1xx_touch.driver, ABS_MT_TOUCH_MAJOR, 200);   // press               
			input_report_abs(hkc1xx_touch.driver, ABS_MT_WIDTH_MAJOR, 10);
			input_report_abs(hkc1xx_touch.driver, ABS_MT_POSITION_X, hkc1xx_touch.x);
			input_report_abs(hkc1xx_touch.driver, ABS_MT_POSITION_Y, hkc1xx_touch.y);
			input_mt_sync(hkc1xx_touch.driver);
		}
		
		input_sync(hkc1xx_touch.driver);
	}
	else	{
		input_report_abs(hkc1xx_touch.driver, ABS_MT_TOUCH_MAJOR, 0);   // press               
		input_report_abs(hkc1xx_touch.driver, ABS_MT_WIDTH_MAJOR, 10);
		input_report_abs(hkc1xx_touch.driver, ABS_MT_POSITION_X, hkc1xx_touch.x);
		input_report_abs(hkc1xx_touch.driver, ABS_MT_POSITION_Y, hkc1xx_touch.y);
		input_mt_sync(hkc1xx_touch.driver);
		input_sync(hkc1xx_touch.driver);
	}
}

//------------------------------------------------------------------------------------------------------------------------
irqreturn_t	hkc1xx_touch_irq(int irq, void *dev_id)
{
	unsigned long	flags;
	
	local_irq_save(flags); local_irq_disable();
	hkc1xx_touch_get_data();

	local_irq_restore(flags);
	return	IRQ_HANDLED;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int	hkc1xx_touch_open	(struct input_dev *dev)
{
	#if	defined(DEBUG_HKC1XX_TOUCH_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
	
	return	0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void	hkc1xx_touch_close	(struct input_dev *dev)
{
	#if	defined(DEBUG_HKC1XX_TOUCH_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void	hkc1xx_touch_release_device	(struct device *dev)
{
	#if	defined(DEBUG_HKC1XX_TOUCH_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
}

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
#ifdef	CONFIG_HAS_EARLYSUSPEND
static void		hkc1xx_touch_late_resume	(struct early_suspend *h)
#else
static 	int		hkc1xx_touch_resume			(struct platform_device *dev)
#endif
{
	#if	defined(DEBUG_HKC1XX_TOUCH_PM_MSG)
		printk("%s\n", __FUNCTION__);
	#endif

	hkc1xx_touch_config(TOUCH_STATE_RESUME);

	// interrupt enable
	enable_irq(HKC1XX_TOUCH_IRQ);

#ifndef	CONFIG_HAS_EARLYSUSPEND
	return	0;
#endif	
}
//[*]--------------------------------------------------------------------------------------------------[*]
#ifdef	CONFIG_HAS_EARLYSUSPEND
static void		hkc1xx_touch_early_suspend	(struct early_suspend *h)
#else
static 	int		hkc1xx_touch_suspend		(struct platform_device *dev, pm_message_t state)
#endif
{
	unsigned char	wdata;
	#if	defined(DEBUG_HKC1XX_TOUCH_PM_MSG)
		printk("%s\n", __FUNCTION__);
	#endif

	wdata = 0x00;
	hkc1xx_touch_write(MODULE_POWERMODE, &wdata, 1);	
	mdelay(10);

	// INT_mode : disable interrupt
	wdata = 0x00;
	hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
	mdelay(10);

	// Touchscreen enter freeze mode : 
	wdata = 0x03;
	hkc1xx_touch_write(MODULE_POWERMODE, &wdata, 1);
	mdelay(10);

	// interrupt disable
	disable_irq(HKC1XX_TOUCH_IRQ);

#ifndef	CONFIG_HAS_EARLYSUSPEND
	return	0;
#endif	
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void		hkc1xx_touch_reset(void)
{
	if(gpio_request(TS_RESET_OUT,"TS_RESET_OUT"))	{
		printk("%s : request port error!\n", __FUNCTION__);
	}

    gpio_direction_output(TS_RESET_OUT, 1);	s3c_gpio_setpull(TS_RESET_OUT, S3C_GPIO_PULL_DOWN);

	gpio_set_value(TS_RESET_OUT, 0);	mdelay(10);
	gpio_set_value(TS_RESET_OUT, 1);	mdelay(10);
	gpio_set_value(TS_RESET_OUT, 0);	mdelay(10);

    gpio_free(TS_RESET_OUT);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void 	hkc1xx_touch_config(unsigned char state)
{
	unsigned char	wdata;

	hkc1xx_touch_reset();	hkc1xx_touch_port_init();
	mdelay(500);

	// Touchscreen Active mode
	wdata = 0x00;
	hkc1xx_touch_write(MODULE_POWERMODE, &wdata, 1);
	mdelay(10);
	
	if(state == TOUCH_STATE_BOOT)	{
		// INT_mode : disable interrupt
//		wdata = 0x00;
//		hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
		wdata = 0x01;
		hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
		// INT_mode : enable interrupt, low-active, finger moving
		wdata = 0x09;
		hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);

		if(!request_irq(HKC1XX_TOUCH_IRQ, hkc1xx_touch_irq, IRQF_DISABLED, "HKC1XX-Touch IRQ", (void *)&hkc1xx_touch))
			printk("HKC1XX TOUCH request_irq = %d\r\n" , HKC1XX_TOUCH_IRQ);
		else
			printk("HKC1XX TOUCH request_irq = %d error!! \r\n", HKC1XX_TOUCH_IRQ);

		set_irq_type(HKC1XX_TOUCH_IRQ, IRQ_TYPE_EDGE_RISING);
		// seqlock init
		seqlock_init(&hkc1xx_touch.lock);		hkc1xx_touch.seq = 0;
	}
	else {
		// INT_mode : disable interrupt, low-active, finger moving
		wdata = 0x01;
		hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
		// INT_mode : enable interrupt, low-active, finger moving
		wdata = 0x09;
		hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int __devinit    hkc1xx_touch_probe	(struct platform_device *pdev)
{
	int		rc;

	// struct init
	memset(&hkc1xx_touch, 0x00, sizeof(hkc1xx_touch_t));
	
	// create sys_fs
	if((rc = hkc1xx_touch_sysfs_create(pdev)))	{
		printk("%s : sysfs_create_group fail!!\n", __FUNCTION__);
		return	rc;
	}
	
	hkc1xx_touch.driver = input_allocate_device();

    if(!(hkc1xx_touch.driver))	{
		printk("ERROR! : %s cdev_alloc() error!!! no memory!!\n", __FUNCTION__);
		hkc1xx_touch_sysfs_remove(pdev);
		return -ENOMEM;
    }

	hkc1xx_touch.driver->name 	= HKC1XX_TOUCH_DEVICE_NAME;
	hkc1xx_touch.driver->phys 	= "hkc1xx_touch/input1";
    hkc1xx_touch.driver->open 	= hkc1xx_touch_open;
    hkc1xx_touch.driver->close	= hkc1xx_touch_close;
	
	hkc1xx_touch.driver->id.bustype	= BUS_HOST;
	hkc1xx_touch.driver->id.vendor 	= 0x16B4;
	hkc1xx_touch.driver->id.product	= 0x0702;
	hkc1xx_touch.driver->id.version	= 0x0001;

	set_bit(EV_ABS, hkc1xx_touch.driver->evbit);

	/* multi touch */
	input_set_abs_params(hkc1xx_touch.driver, ABS_MT_POSITION_X, TS_ABS_MIN_X, TS_ABS_MAX_X,	0, 0);
	input_set_abs_params(hkc1xx_touch.driver, ABS_MT_POSITION_Y, TS_ABS_MIN_Y, TS_ABS_MAX_Y,	0, 0);
	input_set_abs_params(hkc1xx_touch.driver, ABS_MT_TOUCH_MAJOR, 0, 255, 2, 0);
	input_set_abs_params(hkc1xx_touch.driver, ABS_MT_WIDTH_MAJOR, 0,  15, 2, 0);
	
	if(input_register_device(hkc1xx_touch.driver))	{
		printk("HKC1XX TOUCH input register device fail!!\n");

		hkc1xx_touch_sysfs_remove(pdev);
		input_free_device(hkc1xx_touch.driver);		return	-ENODEV;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	hkc1xx_touch.power.suspend 	= hkc1xx_touch_early_suspend;
	hkc1xx_touch.power.resume 	= hkc1xx_touch_late_resume;
	hkc1xx_touch.power.level 	= EARLY_SUSPEND_LEVEL_DISABLE_FB-1;
	//if, is in USER_SLEEP status and no active auto expiring wake lock
	//if (has_wake_lock(WAKE_LOCK_SUSPEND) == 0 && get_suspend_state() == PM_SUSPEND_ON)
	register_early_suspend(&hkc1xx_touch.power);
#endif

	if (gpio_is_valid(TS_ATTB))
	{
		if(gpio_request( TS_ATTB, "TS_ATTB")) {
			printk(KERN_ERR "failed to request GPH1 for TS_ATTB..\n");
			return -1;
		}
		gpio_direction_input(TS_ATTB);
		s3c_gpio_setpull(TS_ATTB, S3C_GPIO_PULL_NONE);
		
		gpio_free(TS_ATTB);
	}

	hkc1xx_touch_config(TOUCH_STATE_BOOT);

	printk("--------------------------------------------------------\n");
	printk("HardKernel : ODROID-T(MT) Multi-Touch driver initialized!! Ver 1.0\n");
	printk("--------------------------------------------------------\n");

	return	0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int __devexit    hkc1xx_touch_remove	(struct platform_device *pdev)
{
	#if	defined(DEBUG_HKC1XX_TOUCH_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
	
	free_irq(HKC1XX_TOUCH_IRQ, (void *)&hkc1xx_touch); 

	hkc1xx_touch_sysfs_remove(pdev);

	input_unregister_device(hkc1xx_touch.driver);

	return  0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int __init	hkc1xx_touch_init	(void)
{
	int ret = platform_driver_register(&hkc1xx_touch_platform_device_driver);
	
	#if	defined(DEBUG_HKC1XX_TOUCH_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
	
	if(!ret)        {
		ret = platform_device_register(&hkc1xx_touch_platform_device);
		
		#if	defined(DEBUG_HKC1XX_TOUCH_MSG)
			printk("platform_driver_register %d \n", ret);
		#endif
		
		if(ret)	platform_driver_unregister(&hkc1xx_touch_platform_device_driver);
	}
	return ret;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void __exit	hkc1xx_touch_exit	(void)
{
	#if	defined(DEBUG_HKC1XX_TOUCH_MSG)
		printk("%s\n",__FUNCTION__);
	#endif
	
	platform_device_unregister(&hkc1xx_touch_platform_device);
	platform_driver_unregister(&hkc1xx_touch_platform_device_driver);
}

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
