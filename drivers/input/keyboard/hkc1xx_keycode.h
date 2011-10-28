//[*]--------------------------------------------------------------------------------------------------[*]
/*
 *	
 * HKC1XX Dev Board key-pad header file (charles.park) 
 *
 */
//[*]--------------------------------------------------------------------------------------------------[*]
#ifndef	_HKC1XX_KEYCODE_H_
#define	_HKC1XX_KEYCODE_H_

	#if defined(CONFIG_KEYPAD_ODROID_T) || defined(CONFIG_KEYPAD_HKC1XX)
	
		#define	MAX_KEYCODE_CNT		6
	
		int HKC1XX_Keycode[MAX_KEYCODE_CNT] = {
			#if defined(CONFIG_KEYPAD_ODROID_T)
				KEY_BACK,		KEY_MENU,		KEY_HOME,				
				KEY_VOLUMEUP,	KEY_POWER,		KEY_VOLUMEDOWN,	
			#else
				KEY_BACK,		KEY_DOWN,		KEY_UP,				
				KEY_RIGHT,		KEY_ENTER,		KEY_LEFT
			#endif		
		};
	
		#if	defined(DEBUG_MSG)
			const char HKC1XX_KeyMapStr[MAX_KEYCODE_CNT][20] = {
				#if defined(CONFIG_KEYPAD_ODROID_T)
					"KEY_BACK\n",		"KEY_MENU\n",		"KEY_HOME\n",
					"KEY_VOLUMEUP\n",	"KEY_POWER\n",		"KEY_VOLUMEDOWN\n"
				#else
					"KEY_BACK\n",		"KEY_DOWN\n",		"KEY_UP\n",
					"KEY_RIGHT\n",		"KEY_ENTER\n",		"KEY_LEFT\n"
				#endif
			};
		#endif	// DEBUG_MSG
	#endif	//	defined(CONFIG_KEYPAD_ODROID_T) || defined(CONFIG_KEYPAD_HKC1XX)
	
	#if defined(CONFIG_KEYPAD_ODROID_S)

		#define	MAX_KEYCODE_CNT		15
	
		int HKC1XX_Keycode[MAX_KEYCODE_CNT] = {
			KEY_VOLUMEDOWN,	KEY_VOLUMEUP,	KEY_UP,			KEY_RIGHT,
			KEY_DOWN,		KEY_LEFT,		KEY_HOME,		KEY_MENU,
			KEY_BACK,		KEY_ENTER,		KEY_X,			KEY_A,
			KEY_B,			KEY_Y,			KEY_POWER,
		};
	
		#if	defined(DEBUG_MSG)
			const char HKC1XX_KeyMapStr[MAX_KEYCODE_CNT][20] = {
				"KEY_VOLUMEDOWN\n",	"KEY_VOLUMEUP\n",	"KEY_UP\n",		"KEY_RIGHT\n",
				"KEY_DOWN\n",		"KEY_LEFT\n",		"KEY_HOME\n",	"KEY_MENU\n",	
				"KEY_BACK\n",		"KEY_ENTER\n",		"KEY_X\n",		"KEY_A\n",
				"KEY_B\n",			"KEY_Y\n",			"KEY_POWER\n",	
			};
		#endif	// DEBUG_MSG
	#endif	// defined(CONFIG_KEYPAD_ODROID_S)

#endif		/* _HKC1XX_KEYPAD_H_*/
