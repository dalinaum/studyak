//[*]--------------------------------------------------------------------------------------------------[*]
/*
 *	
 * HKC1XX Dev Board key-pad header file (charles.park) 
 *
 */
//[*]--------------------------------------------------------------------------------------------------[*]

#ifndef	_HKC1XX_KEYPAD_H_
#define	_HKC1XX_KEYPAD_H_

	#define TRUE 				1
	#define FALSE 				0
	
	#define	ON					1
	#define	OFF					0
	
	#define	KEY_PRESS			1
	#define	KEY_RELEASE			0
	
	#define	KEYPAD_STATE_BOOT	0
	#define	KEYPAD_STATE_RESUME	1
	
	// keypad sampling rate
	#define	PERIOD_10MS			(HZ/100)	// 10ms
	#define	PERIOD_20MS			(HZ/50)		// 20ms
	#define	PERIOD_50MS			(HZ/20)		// 50ms

	// power off timer	
	#define	PERIOD_1SEC			(1*HZ)		// 1sec
	#define	PERIOD_3SEC			(3*HZ)		// 3sec
	#define	PERIOD_5SEC			(5*HZ)		// 5sec

	// Keypad wake up delay
	#define	KEYPAD_WAKEUP_DELAY		100		// 1 sec delay
	
	//
	// Power Off Enable
	//
	#define	PSHOLD_CONTROL			(*(unsigned long *)S5P_PSHOLD_CONTROL)
	#define	POWER_OFF_ENABLE()		{	PSHOLD_CONTROL = 0;	}
	
	//
	// Popwer off port
	//
	#define	POWER_CONTROL_PORT	(S5PV210_GPH0(0))
	#define	POWER_CONTROL_STR	"POWER CONTROL"
	
	//
	// Power LED Port
	//
	#define	POWER_LED_PORT		(S5PV210_GPC1(2))
	#define	POWER_LED_STR		"POWER LED"

	//
	// HDMI Connect event(Switch event used)
	//
	#define	SW_HDMI				0x06

	//
	// Key Hold event(Switch event used)
	//
	#define	SW_KEY_HOLD			0x07

	//
	// Screen rotate event
	// 
	#define	SW_SCREEN_ROTATE	0x08
	
	#if defined(CONFIG_KEYPAD_ODROID_S)
		#define	VOLUME_P_LED_PORT		(S5PV210_GPC1(4))
		#define	VOLUME_P_LED_STR		"VOLUME UP"

		#define	VOLUME_M_LED_PORT		(S5PV210_GPC1(3))
		#define	VOLUME_M_LED_STR		"VOLUME DOWN"
	#endif

	typedef	struct	hkc1xx_keypad__t	{
		
		// keypad control
		struct input_dev	*driver;			// input driver
		struct timer_list 	rd_timer;			// keyscan timer

		// power off control
		struct timer_list	poweroff_timer;		// long power key process

		// sysfs used		
		unsigned char		hold_state;			// key hold off
		unsigned char		sampling_rate;		// 10 msec sampling
		unsigned char		poweroff_time;		// reset off
		
		unsigned int		wakeup_delay;		// key wakeup delay

		#if defined(CONFIG_KEYPAD_ODROID_S)		// slide switch status
			unsigned char	slide_sw_state;
		#endif

	}	hkc1xx_keypad_t;
	
	extern	hkc1xx_keypad_t	hkc1xx_keypad;
#endif		/* _HKC1XX_KEYPAD_H_*/
