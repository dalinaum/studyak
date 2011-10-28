//[*]--------------------------------------------------------------------------------------------------[*]
//
//
// 
//  HKC1XX Board : HKC1XX sysfs driver (charles.park)
//  2010.04.20
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
#include <linux/gpio.h>

#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>

//[*]--------------------------------------------------------------------------------------------------[*]
// Sleep disable flage
//[*]--------------------------------------------------------------------------------------------------[*]
#define	SLEEP_DISABLE_FLAG

#if defined(SLEEP_DISABLE_FLAG)
	#ifdef CONFIG_HAS_WAKELOCK
		#include <linux/wakelock.h>
		static struct wake_lock 	sleep_wake_lock;
	#endif
#endif

//[*]--------------------------------------------------------------------------------------------------[*]
// Combo module control port define
//[*]--------------------------------------------------------------------------------------------------[*]
#define	WIFI_RST_PORT	(S5PV210_GPG3(0))
#define	WIFI_RST_STR	"WIFI RESET"

#define	WIFI_PD_PORT	(S5PV210_GPG3(1))
#define	WIFI_PD_STR		"WIFI POWER DOWN"

#define	BT_RST_PORT		(S5PV210_GPG3(2))
#define	BT_RST_STR		"BT RESET"

#define	MODULE_PWR_PORT	(S5PV210_GPG3(3))
#define	MODULE_PWR_STR	"MODULE POWER"

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
#define	DEBUG_PM_MSG

//[*]--------------------------------------------------------------------------------------------------[*]
//   function prototype define
//[*]--------------------------------------------------------------------------------------------------[*]
static	void 	combo_module_suspend	(void);
static 	void 	combo_module_control	(void);
static 	int		hkc1xx_sysfs_resume		(struct platform_device *dev);
static 	int		hkc1xx_sysfs_suspend	(struct platform_device *dev, pm_message_t state);
static	int		hkc1xx_sysfs_probe		(struct platform_device *pdev);
static	int		hkc1xx_sysfs_remove		(struct platform_device *pdev);	

static 	int 	__init hkc1xx_sysfs_init(void);
static 	void 	__exit hkc1xx_sysfs_exit(void);

//[*]--------------------------------------------------------------------------------------------------[*]
static struct platform_driver hkc1xx_sysfs_driver = {
	.driver = {
		.name = "hkc1xx-sysfs",
		.owner = THIS_MODULE,
	},
	.probe 		= hkc1xx_sysfs_probe,
	.remove 	= hkc1xx_sysfs_remove,
	.suspend	= hkc1xx_sysfs_suspend,
	.resume		= hkc1xx_sysfs_resume,
};

//[*]--------------------------------------------------------------------------------------------------[*]
module_init(hkc1xx_sysfs_init);
module_exit(hkc1xx_sysfs_exit);

//[*]--------------------------------------------------------------------------------------------------[*]

MODULE_DESCRIPTION("SYSFS driver for hkc1xx-Dev board");
MODULE_AUTHOR("Hard-Kernel");
MODULE_LICENSE("GPL");

//[*]--------------------------------------------------------------------------------------------------[*]
//
//   sysfs function prototype define
//
//[*]--------------------------------------------------------------------------------------------------[*]
// 	 Wifi & Bluetooth module power control struct (if wifi disable && bt disable than module power down)
//[*]--------------------------------------------------------------------------------------------------[*]
typedef	struct	combo_module__t	{
	unsigned char		status_wifi;
	unsigned char		status_wifi_wakeup;
	unsigned char		status_bt;
	unsigned char		status_bt_wakeup;
}	combo_module_t	;

static	combo_module_t	combo_module;

static	void 	combo_module_init	(void);
static	void 	combo_module_control(void);
//[*]--------------------------------------------------------------------------------------------------[*]
//   WIFI Reset Control (Set 1 to Enable : Reset High)
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_wifi_onoff	(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_wifi_onoff	(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(wifi_onoff, S_IRWXUGO, show_wifi_onoff, set_wifi_onoff);

//[*]--------------------------------------------------------------------------------------------------[*]
//   BT Reset Controll (Set 1 to Enable : Reset High)
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_bt_onoff	(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_bt_onoff	(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(bt_onoff, S_IRWXUGO, show_bt_onoff, set_bt_onoff);

//[*]--------------------------------------------------------------------------------------------------[*]
//   Module wakeup Controll (Set 1 to Wakeup : Reset suspend)
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_wifi_wakeup	(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_wifi_wakeup	(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(wifi_wakeup, S_IRWXUGO, show_wifi_wakeup, set_wifi_wakeup);

//[*]--------------------------------------------------------------------------------------------------[*]
//   BT Module wakeup Controll (Set 1 to Wakeup : Reset suspend)
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_bt_wakeup	(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_bt_wakeup	(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(bt_wakeup, S_IRWXUGO, show_bt_wakeup, set_bt_wakeup);

//[*]--------------------------------------------------------------------------------------------------[*]
//   HDMI Connect status(Set 1 to Wakeup : Reset suspend)
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_hdmi_connect_state		(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_hdmi_connect_state		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(hdmi_connect_state, S_IRWXUGO, show_hdmi_connect_state, set_hdmi_connect_state);

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static struct attribute *hkc1xx_sysfs_entries[] = {
	&dev_attr_wifi_onoff.attr,
	&dev_attr_bt_onoff.attr,
	&dev_attr_wifi_wakeup.attr,
	&dev_attr_bt_wakeup.attr,
	&dev_attr_hdmi_connect_state.attr,
	NULL
};

static struct attribute_group hkc1xx_sysfs_attr_group = {
	.name   = NULL,
	.attrs  = hkc1xx_sysfs_entries,
};

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_bt_wakeup		(struct device *dev, struct device_attribute *attr, char *buf)
{
	return	sprintf(buf, "%d\n", combo_module.status_bt_wakeup);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_bt_wakeup		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int	val;

    if(!(sscanf(buf, "%u\n", &val))) 	return	-EINVAL;

	if(val != 0)	combo_module.status_bt_wakeup = 1;
	else			combo_module.status_bt_wakeup = 0;

    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_wifi_wakeup		(struct device *dev, struct device_attribute *attr, char *buf)
{
	return	sprintf(buf, "%d\n", combo_module.status_wifi_wakeup);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_wifi_wakeup		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int	val;

    if(!(sscanf(buf, "%u\n", &val))) 	return	-EINVAL;

	if(val != 0)	combo_module.status_wifi_wakeup = 1;
	else			combo_module.status_wifi_wakeup = 0;

    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_wifi_onoff		(struct device *dev, struct device_attribute *attr, char *buf)
{
	return	sprintf(buf, "%d\n", combo_module.status_wifi);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_wifi_onoff		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int	val;

    if(!(sscanf(buf, "%u\n", &val))) 	return	-EINVAL;

	if(val != 0)	combo_module.status_wifi = 1;
	else			combo_module.status_wifi = 0;

	combo_module_control();
	
    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_bt_onoff		(struct device *dev, struct device_attribute *attr, char *buf)
{
	return	sprintf(buf, "%d\n", combo_module.status_bt);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_bt_onoff		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int	val;

    if(!(sscanf(buf, "%u\n", &val))) 	return	-EINVAL;
    
	if(val != 0)	combo_module.status_bt = 1;
	else			combo_module.status_bt = 0;

	combo_module_control();
	
    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
// \drivers\media\video\samsung\tv20_ap\s5p_tv_base.c
extern	bool get_hdmi_onoff_status(void);

static	unsigned char	gAndroidSystemReady = 0;

static 	ssize_t show_hdmi_connect_state		(struct device *dev, struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_VIDEO_TV20)
	unsigned char status = get_hdmi_onoff_status();
#else
	unsigned char status = status = 0;
#endif	
	
	gAndroidSystemReady = 1;

	if(status)	return	sprintf(buf, "%s\n", "on");
	else		return	sprintf(buf, "%s\n", "off");
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_hdmi_connect_state		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	printk("%s : read only sysfs\n", __FUNCTION__);
    return count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static	void combo_module_suspend(void)
{
	combo_module.status_wifi		= 0;
	combo_module.status_wifi_wakeup	= 0;
	combo_module.status_bt			= 0;
	combo_module.status_bt_wakeup	= 0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	void combo_module_control(void)
{
	if(combo_module.status_wifi || combo_module.status_bt)	{
		// set power enable(3V3)
		#if !defined(CONFIG_FB_S3C_LP101WH1)
			gpio_set_value(MODULE_PWR_PORT, 1);
		#endif

		if(combo_module.status_wifi)	{
			gpio_set_value(WIFI_RST_PORT, 1);	

			mdelay(1);		gpio_set_value(WIFI_PD_PORT, 1);
		}
		else	{
			gpio_set_value(WIFI_RST_PORT, 0);	
			
			gpio_set_value(WIFI_PD_PORT, 0);
		}
		
		if(combo_module.status_bt)		gpio_set_value(BT_RST_PORT, 1);
		else							gpio_set_value(BT_RST_PORT, 0);
	}
	else	{
		// set power disable
		#if !defined(CONFIG_FB_S3C_LP101WH1)
			gpio_set_value(MODULE_PWR_PORT, 0);		
		#endif
		
		gpio_set_value(WIFI_RST_PORT, 0);		gpio_set_value(BT_RST_PORT, 0);
		
		gpio_set_value(WIFI_PD_PORT, 0);

		combo_module.status_wifi_wakeup = 0;	combo_module.status_bt_wakeup = 0;
	}
}

#ifdef CONFIG_HKDKC110_BOARD
//[*]--------------------------------------------------------------------------------------------------[*]
unsigned char 	get_bt_wifi_status (void)
{
	if(combo_module.status_wifi||combo_module.status_bt)
		return 1;
	else 
		return 0;
}
EXPORT_SYMBOL(get_bt_wifi_status);
#endif

//[*]--------------------------------------------------------------------------------------------------[*]
int	get_hdmi_connect_status(void)	
{
	if(gAndroidSystemReady)	{
		#if defined(CONFIG_VIDEO_TV20)
			if(get_hdmi_onoff_status())	return	1;	// off
			else						return	0;	// on
		#else
			return	1;
		#endif
	}
	return	-1;
}

EXPORT_SYMBOL(get_hdmi_connect_status);
//[*]--------------------------------------------------------------------------------------------------[*]
static int	hkc1xx_sysfs_resume(struct platform_device *dev)
{
	#if	defined(DEBUG_PM_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
	
    combo_module_control();

    return  0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int	hkc1xx_sysfs_suspend(struct platform_device *dev, pm_message_t state)
{
	#if	defined(DEBUG_PM_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
	
	combo_module_suspend();
	combo_module_control();
	
    return  0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	void 	combo_module_init		(void)
{
	if(gpio_request(WIFI_PD_PORT, WIFI_PD_STR))	{
		printk("%s(%s) : request port error!\n", __FUNCTION__, WIFI_PD_STR);
	}
    gpio_direction_output(WIFI_PD_PORT, 1);	s3c_gpio_setpull(WIFI_PD_PORT, S3C_GPIO_PULL_UP);
	gpio_set_value(WIFI_PD_PORT, 0);

	if(gpio_request(WIFI_RST_PORT, WIFI_RST_STR))	{
		printk("%s(%s) : request port error!\n", __FUNCTION__, WIFI_RST_STR);
	}
    gpio_direction_output(WIFI_RST_PORT, 1);	s3c_gpio_setpull(WIFI_RST_PORT, S3C_GPIO_PULL_UP);
	gpio_set_value(WIFI_RST_PORT, 0);

#if !defined(CONFIG_RFKILL)
	if(gpio_request(BT_RST_PORT, BT_RST_STR))	{
		printk("%s(%s) : request port error!\n", __FUNCTION__, BT_RST_STR);
	}
    gpio_direction_output(BT_RST_PORT, 1);	s3c_gpio_setpull(BT_RST_PORT, S3C_GPIO_PULL_UP);
	gpio_set_value(BT_RST_PORT, 0);
#endif

#if !defined(CONFIG_FB_S3C_LP101WH1)
	if(gpio_request(MODULE_PWR_PORT, MODULE_PWR_STR))	{
		printk("%s(%s) : request port error!\n", __FUNCTION__, MODULE_PWR_STR);
	}
    gpio_direction_output(MODULE_PWR_PORT, 1);	s3c_gpio_setpull(MODULE_PWR_PORT, S3C_GPIO_PULL_UP);
	gpio_set_value(MODULE_PWR_PORT, 0);
#endif

	// combo module power & reset control
	combo_module.status_wifi 		= 0;
	combo_module.status_wifi_wakeup	= 0;
	combo_module.status_bt 			= 0;
	combo_module.status_bt_wakeup 	= 0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	int		hkc1xx_sysfs_probe		(struct platform_device *pdev)	
{
	combo_module_init();
	combo_module_control();

#if defined(SLEEP_DISABLE_FLAG)
	#ifdef CONFIG_HAS_WAKELOCK
		wake_lock(&sleep_wake_lock);
	#endif
#endif

	return	sysfs_create_group(&pdev->dev.kobj, &hkc1xx_sysfs_attr_group);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	int		hkc1xx_sysfs_remove		(struct platform_device *pdev)	
{
	gpio_free(WIFI_RST_PORT);	

#if !defined(CONFIG_RFKILL)
	gpio_free(BT_RST_PORT);
#endif

	gpio_free(WIFI_PD_PORT);

#if !defined(CONFIG_FB_S3C_LP101WH1)
	gpio_free(MODULE_PWR_PORT);
#endif
	
#if defined(SLEEP_DISABLE_FLAG)
	#ifdef CONFIG_HAS_WAKELOCK
		wake_unlock(&sleep_wake_lock);
	#endif
#endif

    sysfs_remove_group(&pdev->dev.kobj, &hkc1xx_sysfs_attr_group);
    
    return	0;
}
//[*]--------------------------------------------------------------------------------------------------[*]
static int __init hkc1xx_sysfs_init(void)
{	
#if defined(SLEEP_DISABLE_FLAG)
	#ifdef CONFIG_HAS_WAKELOCK
		printk("--------------------------------------------------------\n");
		printk("\n%s(%d) : Sleep Disable Flag SET!!(Wake_lock_init)\n\n", __FUNCTION__, __LINE__);
		printk("--------------------------------------------------------\n");

	    wake_lock_init(&sleep_wake_lock, WAKE_LOCK_SUSPEND, "sleep_wake_lock");
	#endif
#endif
    return platform_driver_register(&hkc1xx_sysfs_driver);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void __exit hkc1xx_sysfs_exit(void)
{
#if defined(SLEEP_DISABLE_FLAG)
	#ifdef CONFIG_HAS_WAKELOCK
	    wake_lock_destroy(&sleep_wake_lock);
	#endif
#endif
    platform_driver_unregister(&hkc1xx_sysfs_driver);
}

//[*]--------------------------------------------------------------------------------------------------[*]
