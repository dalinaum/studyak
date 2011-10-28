//[*]--------------------------------------------------------------------------------------------------[*]
//
//
// 
//  HKC1XX Board : Touch Sensor Interface driver (charles.park)
//  2009.10.09
// 
//
//[*]--------------------------------------------------------------------------------------------------[*]
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>

//[*]--------------------------------------------------------------------------------------------------[*]
#include "hkc1xx_touch_gpio_i2c.h"
#include "hkc1xx_touch.h"

//[*]--------------------------------------------------------------------------------------------------[*]
//
//   function prototype define
//
//[*]--------------------------------------------------------------------------------------------------[*]
int		hkc1xx_touch_sysfs_create		(struct platform_device *pdev);
void	hkc1xx_touch_sysfs_remove		(struct platform_device *pdev);	
		
//[*]--------------------------------------------------------------------------------------------------[*]
//
//   sysfs function prototype define
//
//[*]--------------------------------------------------------------------------------------------------[*]
//   screen hold control (on -> hold, off -> normal mode)
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_hold_state			(struct device *dev, struct device_attribute *attr, char *buf);
static	ssize_t set_hold_state			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(hold_state, S_IRWXUGO, show_hold_state, set_hold_state);

//[*]--------------------------------------------------------------------------------------------------[*]
//   touch sampling rate control (5, 10, 20 : unit msec)
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_sampling_rate		(struct device *dev, struct device_attribute *attr, char *buf);
static	ssize_t set_sampling_rate		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(sampling_rate, S_IRWXUGO, show_sampling_rate, set_sampling_rate);

//[*]--------------------------------------------------------------------------------------------------[*]
// 	touch threshold control (range 0 - 10) : default 3
//[*]--------------------------------------------------------------------------------------------------[*]
#define	THRESHOLD_MAX	10

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_threshold_x		(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_threshold_x			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(threshold_x, S_IRWXUGO, show_threshold_x, set_threshold_x);

static 	ssize_t show_threshold_y		(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_threshold_y			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(threshold_y, S_IRWXUGO, show_threshold_y, set_threshold_y);

//[*]--------------------------------------------------------------------------------------------------[*]
//   touch calibration
//[*]--------------------------------------------------------------------------------------------------[*]
#if defined(CONFIG_TOUCHSCREEN_ODROID_MT_T)
	static	ssize_t set_ts_cal		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
	static	DEVICE_ATTR(ts_cal, S_IWUGO, NULL, set_ts_cal);
#endif

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static struct attribute *hkc1xx_touch_sysfs_entries[] = {
	&dev_attr_hold_state.attr,
	&dev_attr_sampling_rate.attr,
	&dev_attr_threshold_x.attr,
	&dev_attr_threshold_y.attr,

#if defined(CONFIG_TOUCHSCREEN_ODROID_MT_T)
	&dev_attr_ts_cal.attr,
#endif
	NULL
};

static struct attribute_group hkc1xx_touch_attr_group = {
	.name   = NULL,
	.attrs  = hkc1xx_touch_sysfs_entries,
};

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_hold_state			(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(hkc1xx_touch.hold_status)		return	sprintf(buf, "on\n");
	else								return	sprintf(buf, "off\n");
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_hold_state			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long flags;

    local_irq_save(flags);

	if(!strcmp(buf, "on\n")) 
		hkc1xx_touch.hold_status = 1;
	else {
#if defined(CONFIG_TOUCHSCREEN_ODROID_MT_T)
		unsigned char 	wdata;
	
		// Touchscreen Active mode
		wdata = 0x00;
		hkc1xx_touch_write(MODULE_POWERMODE, &wdata, 1);
		mdelay(10);

		// INT_mode : disable interrupt
		wdata = 0x00;
		hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
	
		// touch calibration
		wdata =0x03;
		hkc1xx_touch_write(MODULE_CALIBRATION, &wdata, 1);	// set mode
		
		mdelay(500);

		// INT_mode : disable interrupt, low-active, finger moving
		wdata = 0x01;
		hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);

		// INT_mode : enable interrupt, low-active, finger moving
		wdata = 0x09;
		hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
#endif
		hkc1xx_touch.hold_status = 0;
	}

    local_irq_restore(flags);

    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_sampling_rate		(struct device *dev, struct device_attribute *attr, char *buf)
{
	switch(hkc1xx_touch.sampling_rate)	{
		default	:
			hkc1xx_touch.sampling_rate = 0;
		case	0:
			return	sprintf(buf, "10 msec\n");
		case	1:
			return	sprintf(buf, "20 msec\n");
		case	2:
			return	sprintf(buf, "50 msec\n");
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_sampling_rate		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long 	flags;
    unsigned int	val;

    if(!(sscanf(buf, "%u\n", &val)))	return	-EINVAL;

    local_irq_save(flags);

    if		(val > 20)		hkc1xx_touch.sampling_rate = 2;
    else if	(val > 10)		hkc1xx_touch.sampling_rate = 1;
    else					hkc1xx_touch.sampling_rate = 0;

    local_irq_restore(flags);

    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_threshold_x		(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(hkc1xx_touch.threshold_x > THRESHOLD_MAX)	hkc1xx_touch.threshold_x = THRESHOLD_MAX;
		
	return	sprintf(buf, "%d\n", hkc1xx_touch.threshold_x);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_threshold_x			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long 	flags;
    unsigned int	val;

    if(!(sscanf(buf, "%u\n", &val)))	return	-EINVAL;

    local_irq_save(flags);

	if(val < 0)				val *= (-1);
	if(val > THRESHOLD_MAX)	val = THRESHOLD_MAX;
		
	hkc1xx_touch.threshold_x = val;
    
    local_irq_restore(flags);

    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_threshold_y		(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(hkc1xx_touch.threshold_y > THRESHOLD_MAX)	hkc1xx_touch.threshold_y = THRESHOLD_MAX;
		
	return	sprintf(buf, "%d\n", hkc1xx_touch.threshold_y);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_threshold_y			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long 	flags;
    unsigned int	val;

    if(!(sscanf(buf, "%u\n", &val)))	return	-EINVAL;

    local_irq_save(flags);
    
	if(val < 0)				val *= (-1);
	if(val > THRESHOLD_MAX)	val = THRESHOLD_MAX;
		
	hkc1xx_touch.threshold_y = val;
    
    local_irq_restore(flags);

    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
#if defined(CONFIG_TOUCHSCREEN_ODROID_MT_T)
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t set_ts_cal		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char 	wdata;
    unsigned long 	flags;
    unsigned int	val;

    if(!(sscanf(buf, "%u\n", &val)))	return	-EINVAL;

    local_irq_save(flags);		local_irq_disable();

	#if defined(CONFIG_TOUCHSCREEN_ODROID_MT_7) || defined(CONFIG_TOUCHSCREEN_ODROID_MT_7_P)
		// touch recalibration
		wdata =0x00;
		hkc1xx_touch_write(TS_RECALIBRATION, &wdata, 0);	// set mode
		
		mdelay(500);
	#else
		// INT_mode : disable interrupt
		wdata = 0x00;
		hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
	
		// touch calibration
		wdata =0x03;
		hkc1xx_touch_write(MODULE_CALIBRATION, &wdata, 1);	// set mode
		
		mdelay(500);
	
		// INT_mode : enable interrupt, low-active, periodically
		wdata = 0x09;
		hkc1xx_touch_write(MODULE_INTMODE, &wdata, 1);
	#endif

    local_irq_restore(flags);

    return count;
}

#endif	//#if defined(CONFIG_TOUCHSCREEN_ODROID_MT_T)
//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
int		hkc1xx_touch_sysfs_create		(struct platform_device *pdev)	
{
	// variable init
	hkc1xx_touch.hold_status	= 0;

	hkc1xx_touch.sampling_rate	= 0;	// 10 msec sampling

	hkc1xx_touch.threshold_x	= TS_X_THRESHOLD;	// x data threshold (0~10)
	hkc1xx_touch.threshold_y	= TS_Y_THRESHOLD;	// y data threshold (0~10)

	return	sysfs_create_group(&pdev->dev.kobj, &hkc1xx_touch_attr_group);
}

//[*]--------------------------------------------------------------------------------------------------[*]
void	hkc1xx_touch_sysfs_remove		(struct platform_device *pdev)	
{
    sysfs_remove_group(&pdev->dev.kobj, &hkc1xx_touch_attr_group);
}
//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]

