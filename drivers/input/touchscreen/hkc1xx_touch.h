//[*]--------------------------------------------------------------------------------------------------[*]
/*
 *	
 * HardKernel-C1XX _HKC1XX_TOUCH_H_ Header file(charles.park) 
 *
 */
//[*]--------------------------------------------------------------------------------------------------[*]
#ifndef	_HKC1XX_TOUCH_H_
#define	_HKC1XX_TOUCH_H_

//[*]--------------------------------------------------------------------------------------------------[*]
#ifdef CONFIG_HAS_EARLYSUSPEND
	#include <linux/earlysuspend.h>
#endif

//[*]--------------------------------------------------------------------------------------------------[*]
#define HKC1XX_TOUCH_DEVICE_NAME 	"hkc1xx-touch"

//[*]--------------------------------------------------------------------------------------------------[*]
#define	TOUCH_PRESS				1
#define	TOUCH_RELEASE			0
		
//[*]--------------------------------------------------------------------------------------------------[*]
//  Touch Configuration
//[*]--------------------------------------------------------------------------------------------------[*]
#define	TS_ABS_MIN_X			0
#define	TS_ABS_MIN_Y			0

// Touch Interrupt define
#define	HKC1XX_TOUCH_IRQ 		IRQ_EINT8
#define	TS_RESET_OUT			(S5PV210_GPH1(1))

// Interrupt Check port
#define	GET_INT_STATUS()		(((*(unsigned long *)S5PV210_GPH1DAT) & 0x01) ? 1 : 0)

#if defined(CONFIG_TOUCHSCREEN_HKDKC1XX) || defined(CONFIG_TOUCHSCREEN_ODROID_T) || defined(CONFIG_TOUCHSCREEN_ODROID_MT_T)

	#define	TS_ABS_MAX_X			1366
	#define	TS_ABS_MAX_Y			768
	
	#define	TS_X_THRESHOLD			1
	#define	TS_Y_THRESHOLD			1

	#define	TS_ATTB					(S5PV210_GPH1(0))

	// touch register
	#define	MODULE_CALIBRATION		0x37
	#define	MODULE_POWERMODE		0x14
	#define	MODULE_INTMODE			0x15
	#define	MODULE_INTWIDTH			0x16
	
#else	//	defined(CONFIG_TOUCHSCREEN_ODROID_S) || defined(CONFIG_TOUCHSCREEN_ODROID_MT_7)

	#define	TS_ABS_MAX_X			320
	#define	TS_ABS_MAX_Y			480
	
	#define	TS_X_THRESHOLD			5
	#define	TS_Y_THRESHOLD			5

	// touch register
	#define	MODULE_ID_READ			0x01
	#define	SINGLE_TOUCH_READ		0x02
	#define	MULTI_TOUCH_READ		0x03
	#define	SENSITIVITY_CTL			0x04
	#define	SLEEP_CTL				0x05
	#define	RESET_CTL				0x06

#endif		

//[*]--------------------------------------------------------------------------------------------------[*]

//[*]--------------------------------------------------------------------------------------------------[*]
#define	PERIOD_10MS					(HZ/100)	// 10ms
#define	PERIOD_20MS					(HZ/50)		// 20ms
#define	PERIOD_50MS					(HZ/20)		// 50ms

//[*]--------------------------------------------------------------------------------------------------[*]
#define	TOUCH_STATE_BOOT			0
#define	TOUCH_STATE_RESUME			1

//[*]--------------------------------------------------------------------------------------------------[*]
// Touch hold event
//[*]--------------------------------------------------------------------------------------------------[*]
#define	SW_TOUCH_HOLD				0x09

//[*]--------------------------------------------------------------------------------------------------[*]
#if defined(CONFIG_TOUCHSCREEN_ODROID_MT_T)
	
	// multi-touch data process struct
	typedef struct	touch_process_data__t	{
		unsigned 	char	finger_cnt;
		unsigned 	int		x1;
		unsigned 	int		y1;
		unsigned 	int		x2;
		unsigned 	int		y2;
	}	touch_process_data_t;
	
#endif

typedef	struct	hkc1xx_touch__t	{
	struct	input_dev		*driver;

	// seqlock_t
	seqlock_t				lock;
	unsigned int			seq;

	// timer
	struct  timer_list		penup_timer;

	// data store
	unsigned int			status;
	unsigned int			x;
	unsigned int			y;

	unsigned char			rd[10];

	// sysfs used
	unsigned char			hold_status;
	
	unsigned char			sampling_rate;

	unsigned char			threshold_x;	// x data threshold (0-10) : default 3
	unsigned char			threshold_y;	// y data threshold (0-10) : default 3
	
	unsigned char			sensitivity;	// touch sensitivity (0-255) : default 0x14

	#ifdef CONFIG_HAS_EARLYSUSPEND
		struct	early_suspend		power;
	#endif

}	hkc1xx_touch_t;

extern	hkc1xx_touch_t	hkc1xx_touch;

//[*]--------------------------------------------------------------------------------------------------[*]
#endif		/* _HKC100_TOUCH_H_ */
//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
