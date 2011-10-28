//[*]--------------------------------------------------------------------------------------------------[*]
//
//
// 
//  HKC1XX Board : HKC1XX Keypad Interface driver (charles.park)
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
#include <linux/input.h>

#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>

#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-keypad.h>

//[*]--------------------------------------------------------------------------------------------------[*]
#include "hkc1xx_keypad.h"

//[*]--------------------------------------------------------------------------------------------------[*]
//   function prototype define
//[*]--------------------------------------------------------------------------------------------------[*]
int		hkc1xx_keypad_sysfs_create		(struct platform_device *pdev);
void	hkc1xx_keypad_sysfs_remove		(struct platform_device *pdev);	
		
//[*]--------------------------------------------------------------------------------------------------[*]
//
//   sysfs function prototype define
//
//[*]--------------------------------------------------------------------------------------------------[*]
// 	force system power off sysfs
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_system_off			(struct device *dev, struct device_attribute *attr, char *buf);
static	ssize_t set_system_off			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(system_off, S_IRWXUGO, show_system_off, set_system_off);

//[*]--------------------------------------------------------------------------------------------------[*]
//   keypad hold control (on -> hold, off -> normal mode)
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
//   long power off control (set to 1 than touch controller reset)
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_poweroff_control	(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_poweroff_control	(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(poweroff_control, S_IRWXUGO, show_poweroff_control, set_poweroff_control);

//[*]--------------------------------------------------------------------------------------------------[*]
//   slide Switch status
//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_slide_sw_state		(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_slide_sw_state		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static 	DEVICE_ATTR(slide_sw_state, S_IRWXUGO, show_slide_sw_state, set_slide_sw_state);

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static struct attribute *hkc1xx_keypad_sysfs_entries[] = {
	&dev_attr_system_off.attr,
	&dev_attr_hold_state.attr,
	&dev_attr_sampling_rate.attr,
	&dev_attr_poweroff_control.attr,
	&dev_attr_slide_sw_state.attr,

	NULL
};

static struct attribute_group hkc1xx_keypad_attr_group = {
	.name   = NULL,
	.attrs  = hkc1xx_keypad_sysfs_entries,
};

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_system_off			(struct device *dev, struct device_attribute *attr, char *buf)
{
	return	sprintf(buf, "%s : unsupport this function!\n", __FUNCTION__);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_system_off			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long flags;

    local_irq_save(flags);

	if(!strcmp(buf, "on\n"))	{
		POWER_OFF_ENABLE();
		gpio_set_value(POWER_LED_PORT, 0);	    gpio_set_value(POWER_CONTROL_PORT, 0);		
	}

    local_irq_restore(flags);

    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_hold_state			(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(hkc1xx_keypad.hold_state)	return	sprintf(buf, "on\n");
	else							return	sprintf(buf, "off\n");
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_hold_state			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long flags;

    local_irq_save(flags);

	if(!strcmp(buf, "on\n"))	hkc1xx_keypad.hold_state = 1;
	else						hkc1xx_keypad.hold_state = 0;

    local_irq_restore(flags);

    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_sampling_rate		(struct device *dev, struct device_attribute *attr, char *buf)
{
	switch(hkc1xx_keypad.sampling_rate)	{
		default	:
			hkc1xx_keypad.sampling_rate = 0;
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

    if(!(sscanf(buf, "%u\n", &val))) 	return	-EINVAL;
    
    local_irq_save(flags);
    
    if		(val > 20)		hkc1xx_keypad.sampling_rate = 2;
    else if	(val > 10)		hkc1xx_keypad.sampling_rate = 1;
    else					hkc1xx_keypad.sampling_rate = 0;

    local_irq_restore(flags);

    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_poweroff_control	(struct device *dev, struct device_attribute *attr, char *buf)
{
	switch(hkc1xx_keypad.poweroff_time)	{
		default	:
			hkc1xx_keypad.poweroff_time = 0;
		case	0:
			return	sprintf(buf, "1 sec\n");
		case	1:
			return	sprintf(buf, "3 sec\n");
		case	2:
			return	sprintf(buf, "5 sec\n");
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_poweroff_control	(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long 	flags;
    unsigned int	val;

    if(!(sscanf(buf, "%u\n", &val)))	return	-EINVAL;

    local_irq_save(flags);
    
    if		(val > 3)		hkc1xx_keypad.sampling_rate = 2;
    else if	(val > 1)		hkc1xx_keypad.sampling_rate = 1;
    else					hkc1xx_keypad.sampling_rate = 0;

    local_irq_restore(flags);

    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_slide_sw_state		(struct device *dev, struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_KEYPAD_ODROID_S)
	if(hkc1xx_keypad.slide_sw_state)	return	sprintf(buf, "on\n");
	else								return	sprintf(buf, "off\n");
#else
	return	sprintf(buf, "off\n");
#endif
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_slide_sw_state		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	printk("%s : unsupport this function!\n", __FUNCTION__);
    return count;
}


//[*]--------------------------------------------------------------------------------------------------[*]
int		hkc1xx_keypad_sysfs_create		(struct platform_device *pdev)	
{
	// variable init
	hkc1xx_keypad.hold_state		= 0;	// key hold off
	hkc1xx_keypad.sampling_rate		= 0;	// 10 msec sampling
	hkc1xx_keypad.poweroff_time		= 1;	// 3 sec
#if defined(CONFIG_KEYPAD_ODROID_S)
	hkc1xx_keypad.slide_sw_state	= 0;
#endif

	return	sysfs_create_group(&pdev->dev.kobj, &hkc1xx_keypad_attr_group);
}

//[*]--------------------------------------------------------------------------------------------------[*]
void	hkc1xx_keypad_sysfs_remove		(struct platform_device *pdev)	
{
    sysfs_remove_group(&pdev->dev.kobj, &hkc1xx_keypad_attr_group);
}
//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]

