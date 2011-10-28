//[*]--------------------------------------------------------------------------------------------------[*]
/*
 *
 * HKC1XX Dev Board keypad driver (charles.park)
 *
 */
//[*]--------------------------------------------------------------------------------------------------[*]
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>

#include <linux/delay.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <asm/delay.h>
#include <asm/irq.h>

#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-bank.h>
#include <mach/regs-clock.h>

// Debug message enable flag
//#define	DEBUG_MSG			
#define	DEBUG_PM_MSG

//[*]--------------------------------------------------------------------------------------------------[*]
#include "hkc1xx_keypad.h"
#include "hkc1xx_keycode.h"
#include "hkc1xx_keypad_sysfs.h"

//[*]--------------------------------------------------------------------------------------------------[*]
#define DEVICE_NAME "hkc1xx-keypad"

//[*]--------------------------------------------------------------------------------------------------[*]
hkc1xx_keypad_t	hkc1xx_keypad;

//[*]--------------------------------------------------------------------------------------------------[*]
static void				generate_keycode				(unsigned short prev_key, unsigned short cur_key, int *key_table);
static void 			hkc1xx_keyled_control			(int keycode, unsigned char status);
static void 			hkc1xx_poweroff_timer_run		(void);
static void 			hkc1xx_poweroff_timer_handler	(unsigned long data);

static int				hkc1xx_keypad_get_data			(void);
static void				hkc1xx_keypad_port_init			(void);
static void 			hkc1xx_keypad_control			(void);

static void				hkc1xx_rd_timer_handler			(unsigned long data);

static int              hkc1xx_keypad_open              (struct input_dev *dev);
static void             hkc1xx_keypad_close             (struct input_dev *dev);

static void             hkc1xx_keypad_release_device    (struct device *dev);
static int              hkc1xx_keypad_resume            (struct platform_device *dev);
static int              hkc1xx_keypad_suspend           (struct platform_device *dev, pm_message_t state);

static void				hkc1xx_keypad_config			(unsigned char state);

static int __devinit    hkc1xx_keypad_probe				(struct platform_device *pdev);
static int __devexit    hkc1xx_keypad_remove			(struct platform_device *pdev);

static int __init       hkc1xx_keypad_init				(void);
static void __exit      hkc1xx_keypad_exit				(void);

//[*]--------------------------------------------------------------------------------------------------[*]
static struct platform_driver hkc1xx_platform_device_driver = {
		.probe          = hkc1xx_keypad_probe,
		.remove         = hkc1xx_keypad_remove,
		.suspend        = hkc1xx_keypad_suspend,
		.resume         = hkc1xx_keypad_resume,
		.driver		= {
			.owner	= THIS_MODULE,
			.name	= DEVICE_NAME,
		},
};

//[*]--------------------------------------------------------------------------------------------------[*]
static struct platform_device hkc1xx_platform_device = {
        .name           = DEVICE_NAME,
        .id             = -1,
        .num_resources  = 0,
        .dev    = {
                .release	= hkc1xx_keypad_release_device,
        },
};

//[*]--------------------------------------------------------------------------------------------------[*]
module_init(hkc1xx_keypad_init);
module_exit(hkc1xx_keypad_exit);

//[*]--------------------------------------------------------------------------------------------------[*]
MODULE_AUTHOR("Hard-Kernel");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Keypad interface for hkc1xx-Dev board");

//[*]--------------------------------------------------------------------------------------------------[*]
static void hkc1xx_keyled_control(int keycode, unsigned char status)
{
	if((keycode != KEY_VOLUMEUP) && (keycode != KEY_VOLUMEDOWN))	return;
	switch(keycode)	{
		case	KEY_VOLUMEDOWN:		gpio_set_value(VOLUME_M_LED_PORT, status);	break;
		case	KEY_VOLUMEUP:		gpio_set_value(VOLUME_P_LED_PORT, status);	break;
		default:																break;
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void hkc1xx_poweroff_timer_run(void)
{
	init_timer(&hkc1xx_keypad.poweroff_timer);
	hkc1xx_keypad.poweroff_timer.function = hkc1xx_poweroff_timer_handler;

	switch(hkc1xx_keypad.poweroff_time)	{
		default	:
			hkc1xx_keypad.poweroff_time = 0;
		case	0:
			hkc1xx_keypad.poweroff_timer.expires = jiffies + PERIOD_1SEC;
			break;
		case	1:
			hkc1xx_keypad.poweroff_timer.expires = jiffies + PERIOD_3SEC;
			break;
		case	2:
			hkc1xx_keypad.poweroff_timer.expires = jiffies + PERIOD_5SEC;
			break;
	}
	add_timer(&hkc1xx_keypad.poweroff_timer);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void hkc1xx_poweroff_timer_handler(unsigned long data)
{
	POWER_OFF_ENABLE();
	
	gpio_set_value(POWER_LED_PORT, 0);	    gpio_set_value(POWER_CONTROL_PORT, 0);		
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void	generate_keycode(unsigned short prev_key, unsigned short cur_key, int *key_table)
{
	unsigned short 	press_key, release_key, i;
	
	press_key	= (cur_key ^ prev_key) & cur_key;
	release_key	= (cur_key ^ prev_key) & prev_key;
	
	i = 0;
	while(press_key)	{
		if(press_key & 0x01)	{
			
			if(hkc1xx_keypad.hold_state)	{
				if(key_table[i] == KEY_POWER)	input_report_key(hkc1xx_keypad.driver, key_table[i], KEY_PRESS);					
				else							input_report_switch(hkc1xx_keypad.driver, SW_KEY_HOLD, ON);
			}
			else	{
				input_report_key(hkc1xx_keypad.driver, key_table[i], KEY_PRESS);
				if(hkc1xx_keypad.slide_sw_state)	hkc1xx_keyled_control(key_table[i], KEY_PRESS);
			}
			
			// POWER OFF PRESS
			if(key_table[i] == KEY_POWER)	hkc1xx_poweroff_timer_run();
		}
		i++;	press_key >>= 1;
	}
	
	i = 0;
	while(release_key)	{
		if(release_key & 0x01)	{

			if(hkc1xx_keypad.hold_state)	{
				if(key_table[i] == KEY_POWER)	input_report_key(hkc1xx_keypad.driver, key_table[i], KEY_RELEASE);
				else							input_report_switch(hkc1xx_keypad.driver, SW_KEY_HOLD, OFF);
			}
			else	{
				input_report_key(hkc1xx_keypad.driver, key_table[i], KEY_RELEASE);
				if(hkc1xx_keypad.slide_sw_state)	hkc1xx_keyled_control(key_table[i], KEY_RELEASE);
			}

			// POWER OFF (LONG PRESS)		
			if(key_table[i] == KEY_POWER)		del_timer_sync(&hkc1xx_keypad.poweroff_timer);
		}
		i++;	release_key >>= 1;
	}
}
//[*]--------------------------------------------------------------------------------------------------[*]
#if defined(DEBUG_MSG)
	static void debug_keycode_printf(unsigned short prev_key, unsigned short cur_key, const char *key_table)
	{
		unsigned short 	press_key, release_key, i;
		
		press_key	= (cur_key ^ prev_key) & cur_key;
		release_key	= (cur_key ^ prev_key) & prev_key;
		
		i = 0;
		while(press_key)	{
			if(press_key & 0x01)	printk("PRESS KEY : %s", (char *)&key_table[i * 20]);
			i++;					press_key >>= 1;
		}
		
		i = 0;
		while(release_key)	{
			if(release_key & 0x01)	printk("RELEASE KEY : %s", (char *)&key_table[i * 20]);
			i++;					release_key >>= 1;
		}
	}
#endif

//[*]--------------------------------------------------------------------------------------------------[*]
static void	hkc1xx_keypad_port_init(void)
{
	// Power On Key
	(*(unsigned long *)S5PV210_GPH0CON) &= (~0x000000F0);	
	(*(unsigned long *)S5PV210_GPH0PUD) &= (~0x0000000C);

	// Matrix out
	(*(unsigned long *)S5PV210_GPH3CON) &= (~0xF0000FFF);	
	(*(unsigned long *)S5PV210_GPH3CON) |= ( 0x00000111);	
	(*(unsigned long *)S5PV210_GPH3PUD) &= (~0x0000C03F);

	// Matrix in
	(*(unsigned long *)S5PV210_GPH2CON) &= 0xFF000000;
	(*(unsigned long *)S5PV210_GPH2PUD)  = 0x00000AA0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int	hkc1xx_keypad_get_data(void)
{
	static	unsigned char	next_state = 0, row_0 = 0, row_1 = 0, row_2 = 0;
	int		key_data = -1, temp = 0, temp1 = 0;

	temp  = (*(unsigned long *)S5PV210_GPH3DAT);
	temp1 = (*(unsigned long *)S5PV210_GPH3CON);

	switch(next_state)	{
		default	 :	next_state = 0;
		case	0:	
			temp  &= (~0x07);		temp  |= 0x06;			// ROW0 line Low
			temp1 &= (~0x00000FFF);	temp1 |= 0x000000001;	// ROW0 Output
			
			// key data pros
			key_data  = (((row_2 & 0x3C)<<8) | ((row_1 & 0x3C)<<4) | row_0);

			// Power On Press
			if( ((*(unsigned long *)S5PV210_GPH0DAT) & 0x02) == 0)	key_data |= 0x4000;
			
			// Slide On
			if( ((*(unsigned long *)S5PV210_GPH3DAT) & 0x80) )		key_data |= 0x8000;

			break;				
			
		case	1:
			// ROW0 line low -> Data read
			row_0 = (~(*(unsigned long *)S5PV210_GPH2DAT) & 0x3F); 
			
			temp  &= (~0x07);		temp  |= 0x05;			// ROW1 line Low
			temp1 &= (~0x00000FFF);	temp1 |= 0x000000010;	// ROW1 Output
			break;
		case	2:
			// ROW1 line low -> Data read
			row_1 = (~(*(unsigned long *)S5PV210_GPH2DAT) & 0x3F); 

			temp  &= (~0x07);		temp  |= 0x03;			// ROW2 line Low
			temp1 &= (~0x00000FFF);	temp1 |= 0x000000100;	// ROW2 Output
			break;
		case	3:
			// ROW2 line low -> Data read
			row_2 = (~(*(unsigned long *)S5PV210_GPH2DAT) & 0x3F); 

			temp  &= (~0x07);		temp |= 0x07;	// ROW0-2 line High
			temp1 &= (~0x00000FFF);					// ROW0-2 Input
			break;
	}	

	(*(unsigned long *)S5PV210_GPH3DAT) = temp;
	(*(unsigned long *)S5PV210_GPH3CON) = temp1;

	if(next_state < 3)		next_state ++;
	else					next_state = 0;

	return	key_data;
}

//[*]--------------------------------------------------------------------------------------------------[*]
unsigned char	gVolumeKeyLock = 0;
void	set_volume_key_lock(unsigned char status)		// used odroids_touch.c
{
	gVolumeKeyLock = status;
}

EXPORT_SYMBOL(set_volume_key_lock);
//[*]--------------------------------------------------------------------------------------------------[*]
extern	int		get_hdmi_connect_status(void);

static void hkc1xx_keypad_control(void)
{
	static	unsigned short	prev_keypad_data = 0, cur_keypad_data = 0, hdmi_chk_count = 0, hdmi_status = -1;
	int		status = 0;

	if((status = hkc1xx_keypad_get_data()) < 0)		return;

	// key data process
	cur_keypad_data = status;

	if(cur_keypad_data & 0x8000)	{
		if(hkc1xx_keypad.slide_sw_state == 0)	{
			hkc1xx_keypad.slide_sw_state = 1;
			input_report_switch(hkc1xx_keypad.driver, SW_SCREEN_ROTATE, ON);
			input_sync(hkc1xx_keypad.driver);
		}
		
		// Hardware bug
		if(gVolumeKeyLock)		cur_keypad_data &= (~0x0003);	// Volume mask
	}
	else	{
		if(hkc1xx_keypad.slide_sw_state == 1)	{
			hkc1xx_keypad.slide_sw_state = 0;
			input_report_switch(hkc1xx_keypad.driver, SW_SCREEN_ROTATE, OFF);
			input_sync(hkc1xx_keypad.driver);
			hkc1xx_keyled_control(KEY_VOLUMEUP  , KEY_RELEASE);
			hkc1xx_keyled_control(KEY_VOLUMEDOWN, KEY_RELEASE);
		}
		cur_keypad_data &= (~0x0003);	// Volume mask
	}
	
	if(hdmi_chk_count > 100)	{
		hdmi_chk_count = 0;
		
		if((status = get_hdmi_connect_status()) >= 0)	{
			if(hdmi_status != status)	{

				hdmi_status = status;

				if(hdmi_status)		input_report_switch(hkc1xx_keypad.driver, SW_HDMI, ON);
				else				input_report_switch(hkc1xx_keypad.driver, SW_HDMI, OFF);

				input_sync(hkc1xx_keypad.driver);
			}
		}
	}
	else	hdmi_chk_count++;
	
	cur_keypad_data &= (~0x8000);	// Slide mask

	if(prev_keypad_data != cur_keypad_data)	{
		
		generate_keycode(prev_keypad_data, cur_keypad_data, &HKC1XX_Keycode[0]);

		#if defined(DEBUG_MSG)
			debug_keycode_printf(prev_keypad_data, cur_keypad_data, &HKC1XX_KeyMapStr[0][0]);
		#endif

		prev_keypad_data = cur_keypad_data;

		input_sync(hkc1xx_keypad.driver);
	}
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void	hkc1xx_rd_timer_handler(unsigned long data)
{
    unsigned long flags;

    local_irq_save(flags);

	if(hkc1xx_keypad.wakeup_delay > KEYPAD_WAKEUP_DELAY)	hkc1xx_keypad_control();	
	else													hkc1xx_keypad.wakeup_delay++;
		
	// Kernel Timer restart
	switch(hkc1xx_keypad.sampling_rate)	{
		default	:
			hkc1xx_keypad.sampling_rate = 0;
		case	0:
			mod_timer(&hkc1xx_keypad.rd_timer,jiffies + PERIOD_10MS);
			break;
		case	1:
			mod_timer(&hkc1xx_keypad.rd_timer,jiffies + PERIOD_20MS);
			break;
		case	2:
			mod_timer(&hkc1xx_keypad.rd_timer,jiffies + PERIOD_50MS);
			break;
	}

    local_irq_restore(flags);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int	hkc1xx_keypad_open(struct input_dev *dev)
{
	#if	defined(DEBUG_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
	
	return	0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void	hkc1xx_keypad_close(struct input_dev *dev)
{
	#if	defined(DEBUG_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void	hkc1xx_keypad_release_device(struct device *dev)
{
	#if	defined(DEBUG_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int	hkc1xx_keypad_resume(struct platform_device *dev)
{
	#if	defined(DEBUG_PM_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
	
	hkc1xx_keypad_config(KEYPAD_STATE_RESUME);
	
	// Wakeup dumy key send
	input_report_key(hkc1xx_keypad.driver, KEY_HOME, KEY_PRESS);					
	input_report_key(hkc1xx_keypad.driver, KEY_HOME, KEY_RELEASE);					
	input_sync(hkc1xx_keypad.driver);
	
	hkc1xx_keypad.wakeup_delay = 0;

    return  0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int	hkc1xx_keypad_suspend(struct platform_device *dev, pm_message_t state)
{
	#if	defined(DEBUG_PM_MSG)
		printk("%s\n", __FUNCTION__);
	#endif

	del_timer_sync(&hkc1xx_keypad.rd_timer);
	
    return  0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void	hkc1xx_keypad_config(unsigned char state)
{
	if(state == KEYPAD_STATE_BOOT)	{
		hkc1xx_keypad_port_init();	// GPH2, GPH3 All Input, All Pull up
	
		if(gpio_request(VOLUME_P_LED_PORT, VOLUME_P_LED_STR))	{
			printk("%s(%s) : request port error!\n", __FUNCTION__, VOLUME_P_LED_STR);
		}
	    gpio_direction_output(VOLUME_P_LED_PORT, 1);	s3c_gpio_setpull(VOLUME_P_LED_PORT, S3C_GPIO_PULL_NONE);
		gpio_set_value(VOLUME_P_LED_PORT, 0);
		
		if(gpio_request(VOLUME_M_LED_PORT, VOLUME_M_LED_STR))	{
			printk("%s(%s) : request port error!\n", __FUNCTION__, VOLUME_P_LED_STR);
		}
	    gpio_direction_output(VOLUME_M_LED_PORT, 1);	s3c_gpio_setpull(VOLUME_M_LED_PORT, S3C_GPIO_PULL_NONE);
		gpio_set_value(VOLUME_M_LED_PORT, 0);

		if(gpio_request(POWER_LED_PORT, POWER_LED_STR))	{
			printk("%s(%s) : request port error!\n", __FUNCTION__, POWER_LED_STR);
		}
	    gpio_direction_output(POWER_LED_PORT, 1);	s3c_gpio_setpull(POWER_LED_PORT, S3C_GPIO_PULL_NONE);
		gpio_set_value(POWER_LED_PORT, 1);
	
		if(gpio_request(POWER_CONTROL_PORT, POWER_CONTROL_STR))	{
			printk("%s(%s) : request port error!\n", __FUNCTION__, POWER_CONTROL_STR);
		}
		gpio_set_value(POWER_CONTROL_PORT, 1);
	    gpio_direction_output(POWER_CONTROL_PORT, 1);	s3c_gpio_setpull(POWER_CONTROL_PORT, S3C_GPIO_PULL_NONE);
	}

	/* Scan timer init */
	init_timer(&hkc1xx_keypad.rd_timer);

	hkc1xx_keypad.rd_timer.function = hkc1xx_rd_timer_handler;
	hkc1xx_keypad.rd_timer.expires = jiffies + (HZ/100);

	add_timer(&hkc1xx_keypad.rd_timer);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int __devinit    hkc1xx_keypad_probe(struct platform_device *pdev)
{
    int 	key, code, rc;

	// struct init
	memset(&hkc1xx_keypad, 0x00, sizeof(hkc1xx_keypad_t));
	
	// create sys_fs
	if((rc = hkc1xx_keypad_sysfs_create(pdev)))	{
		printk("%s : sysfs_create_group fail!!\n", __FUNCTION__);
		return	rc;
	}

	hkc1xx_keypad.driver = input_allocate_device();
	
    if(!(hkc1xx_keypad.driver))	{
		printk("ERROR! : %s input_allocate_device() error!!! no memory!!\n", __FUNCTION__);
		hkc1xx_keypad_sysfs_remove(pdev);
		return -ENOMEM;
    }

	set_bit(EV_KEY, hkc1xx_keypad.driver->evbit);
//	set_bit(EV_REP, hkc1xx_keypad.driver->evbit);	// Repeat Key

	set_bit(EV_SW, hkc1xx_keypad.driver->evbit);
	set_bit(SW_HDMI 			& SW_MAX, hkc1xx_keypad.driver->swbit);
	set_bit(SW_KEY_HOLD			& SW_MAX, hkc1xx_keypad.driver->swbit);
	set_bit(SW_SCREEN_ROTATE 	& SW_MAX, hkc1xx_keypad.driver->swbit);

	for(key = 0; key < MAX_KEYCODE_CNT; key++){
		code = HKC1XX_Keycode[key];
		if(code<=0)
			continue;
		set_bit(code & KEY_MAX, hkc1xx_keypad.driver->keybit);
	}

	hkc1xx_keypad.driver->name 	= DEVICE_NAME;
	hkc1xx_keypad.driver->phys 	= "hkc1xx-keypad/input0";
    hkc1xx_keypad.driver->open 	= hkc1xx_keypad_open;
    hkc1xx_keypad.driver->close	= hkc1xx_keypad_close;
	
	hkc1xx_keypad.driver->id.bustype	= BUS_HOST;
	hkc1xx_keypad.driver->id.vendor 	= 0x16B4;
	hkc1xx_keypad.driver->id.product 	= 0x0701;
	hkc1xx_keypad.driver->id.version 	= 0x0001;

	hkc1xx_keypad.driver->keycode = HKC1XX_Keycode;

	if(input_register_device(hkc1xx_keypad.driver))	{
		printk("HardKernel-C100 keypad input register device fail!!\n");

		hkc1xx_keypad_sysfs_remove(pdev);
		input_free_device(hkc1xx_keypad.driver);	return	-ENODEV;
	}

	hkc1xx_keypad_config(KEYPAD_STATE_BOOT);

	printk("--------------------------------------------------------\n");
	printk("HardKernel : ODROID-S Keypad driver initialized!! Ver 1.0\n");
	printk("--------------------------------------------------------\n");

    return 0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int __devexit    hkc1xx_keypad_remove(struct platform_device *pdev)
{
	hkc1xx_keypad_sysfs_remove(pdev);

	input_unregister_device(hkc1xx_keypad.driver);
	
	del_timer_sync(&hkc1xx_keypad.rd_timer);

	gpio_free(VOLUME_P_LED_PORT);	gpio_free(VOLUME_M_LED_PORT);
	gpio_free(POWER_LED_PORT);		gpio_free(POWER_CONTROL_PORT);

	#if	defined(DEBUG_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
	
	return  0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int __init	hkc1xx_keypad_init(void)
{
	int ret = platform_driver_register(&hkc1xx_platform_device_driver);
	
	#if	defined(DEBUG_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
	
	if(!ret)        {
		ret = platform_device_register(&hkc1xx_platform_device);
		
		#if	defined(DEBUG_MSG)
			printk("platform_driver_register %d \n", ret);
		#endif
		
		if(ret)	platform_driver_unregister(&hkc1xx_platform_device_driver);
	}
	return ret;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void __exit	hkc1xx_keypad_exit(void)
{
	#if	defined(DEBUG_MSG)
		printk("%s\n",__FUNCTION__);
	#endif
	
	platform_device_unregister(&hkc1xx_platform_device);
	platform_driver_unregister(&hkc1xx_platform_device_driver);
}

//[*]--------------------------------------------------------------------------------------------------[*]
