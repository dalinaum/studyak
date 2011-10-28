//[*]--------------------------------------------------------------------------------------------------[*]
/*
 *	
 * HardKernel-C1XX _QT602240_H_ Header file(charles.park) 
 *
 */
//[*]--------------------------------------------------------------------------------------------------[*]
#ifndef	_QT602240_H_
#define	_QT602240_H_

#define RESERVED_T0                               0u
#define RESERVED_T1                               1u
#define DEBUG_DELTAS_T2                           2u
#define DEBUG_REFERENCES_T3                       3u
#define DEBUG_SIGNALS_T4                          4u
#define GEN_MESSAGEPROCESSOR_T5                   5u
#define GEN_COMMANDPROCESSOR_T6                   6u
#define GEN_POWERCONFIG_T7                        7u
#define GEN_ACQUISITIONCONFIG_T8                  8u
#define TOUCH_MULTITOUCHSCREEN_T9                 9u
#define TOUCH_SINGLETOUCHSCREEN_T10               10u
#define TOUCH_XSLIDER_T11                         11u
#define TOUCH_YSLIDER_T12                         12u
#define TOUCH_XWHEEL_T13                          13u
#define TOUCH_YWHEEL_T14                          14u
#define TOUCH_KEYARRAY_T15                        15u
#define PROCG_SIGNALFILTER_T16                    16u
#define PROCI_LINEARIZATIONTABLE_T17              17u
#define SPT_COMCONFIG_T18                         18u
#define SPT_GPIOPWM_T19                           19u
#define PROCI_GRIPFACESUPPRESSION_T20             20u
#define RESERVED_T21                              21u
#define PROCG_NOISESUPPRESSION_T22                22u
#define TOUCH_PROXIMITY_T23                       23u
#define PROCI_ONETOUCHGESTUREPROCESSOR_T24        24u
#define SPT_SELFTEST_T25                          25u
#define DEBUG_CTERANGE_T26                        26u
#define PROCI_TWOTOUCHGESTUREPROCESSOR_T27        27u
#define SPT_CTECONFIG_T28                         28u
#define SPT_GPI_T29                               29u
#define SPT_GATE_T30                              30u
#define TOUCH_KEYSET_T31                          31u
#define TOUCH_XSLIDERSET_T32                      32u
#define RESERVED_T33                              33u
#define GEN_MESSAGEBLOCK_T34                      34u
#define SPT_GENERICDATA_T35                       35u
#define RESERVED_T36                              36u
#define DEBUG_DIAGNOSTIC_T37                      37u
#define SPARE_T38                                 38u
#define SPARE_T39                                 39u
#define SPARE_T40                                 40u
#define SPARE_T41                                 41u
#define SPARE_T42                                 42u
#define SPARE_T43                                 43u
#define SPARE_T44                                 44u
#define SPARE_T45                                 45u
#define SPARE_T46                                 46u
#define SPARE_T47                                 47u
#define SPARE_T48                                 48u
#define SPARE_T49                                 49u
#define SPARE_T50                                 50u
/*
 * All entries spare up to 255
*/
#define RESERVED_T255                             255u

/*! Offset to RESET register from the beginning of command processor. */
#define RESET_OFFSET                    0u

/*! Offset to BACKUP register from the beginning of command processor. */
#define BACKUP_OFFSET       			1u

/*! Offset to CALIBRATE register from the beginning of command processor. */
#define CALIBRATE_OFFSET    			2u

/*! Offset to REPORTALL register from the beginning of command processor. */
#define REPORTATLL_OFFSET   			3u

/*! Offset to DEBUG_CTRL register from the beginning of command processor. */
#define DEBUG_CTRL_OFFSET   			4u

/*! Offset to DIAGNOSTIC_CTRL register from the beginning of command processor. */
#define DIAGNOSTIC_OFFSET   			5u

/*! Size of one object table element in touch IC memory map. */
#define OBJECT_TABLE_ELEMENT_SIZE       6U

/*! Address where object table starts at touch IC memory map. */
#define OBJECT_TABLE_START_ADDRESS      7U

/* Touch status */
#define QT602240_SUPPRESS		(1 << 1)
#define QT602240_AMP			(1 << 2)
#define QT602240_VECTOR			(1 << 3)
#define QT602240_MOVE			(1 << 4)
#define QT602240_RELEASE		(1 << 5)
#define QT602240_PRESS			(1 << 6)
#define QT602240_DETECT			(1 << 7)

/* Touch message offset */
#define	TS_REPORT_ID		0
#define	TS_STATUS			1
#define	TS_X_UPPER			2
#define	TS_Y_UPPER			3
#define	TS_XY_LOWER			4
#define	TS_AREA				5

typedef struct qt602240_info__t	{
	unsigned char	family_id;
	unsigned char	variant_id;
	unsigned char	version;
	unsigned char	build;
	unsigned char	matrix_xsize;
	unsigned char	matrix_ysize;
	unsigned char	object_num;
}	__attribute__ ((packed))	qt602240_info_t;

typedef	struct qt602240_object__t	{
	unsigned char	type;
	unsigned short	start_address;
	unsigned char	size;
	unsigned char	instances;
	unsigned char	num_report_ids;
}	__attribute__ ((packed))	qt602240_object_t;

typedef struct qt602240_report_id_map__t	{
	unsigned char	type;     	/*!< Object type. */
	unsigned char	instance;	/*!< Instance number. */
}	__attribute__ ((packed))	qt602240_report_id_map_t;

typedef	struct 	qt602240_finger__t	{
	int 	status;
	int 	x;
	int 	y;
	int 	area;
}	__attribute__ ((packed))	qt602240_finger_t;

typedef struct	qt602240__t	{
	qt602240_info_t				info;
	qt602240_object_t 			*pobject;
	qt602240_report_id_map_t 	*preport_id_map;
	qt602240_finger_t			finger[TS_MAX_FINGER];
	
	unsigned int				max_report_id;
	unsigned int				max_message_length;
	unsigned char				*pmessage;
}	__attribute__ ((packed))	qt602240_t;

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
typedef struct	power_config__t		{
	unsigned char	idleacqint;    /*!< Idle power mode sleep length in ms           */
	unsigned char	actvacqint;    /*!< Active power mode sleep length in ms         */
	unsigned char	actv2idleto;   /*!< Active to idle power mode delay length in units of 0.2s*/
}	__attribute__ ((packed)) power_config_t;

typedef struct	acquisition_config__t	{ 
	unsigned char	chrgtime;		/*!< Charge-transfer dwell time             */  
	unsigned char	reserved;       /*!< reserved                               */
	unsigned char	tchdrift;       /*!< Touch drift compensation period        */
	unsigned char	driftst;        /*!< Drift suspend time                     */
	unsigned char	tchautocal;     /*!< Touch automatic calibration delay in units of 0.2s*/
	unsigned char	sync;           /*!< Measurement synchronisation control    */
	unsigned char	atchcalst;      /*!< recalibration suspend time after last detection */
	unsigned char	atchcalsthr;    /*!< Anti-touch calibration suspend threshold */
}	__attribute__ ((packed)) acquisition_config_t;

typedef struct	touchscreen_config__t	{
	/* Screen Configuration */
	unsigned char	ctrl;			/*!< ACENABLE LCENABLE Main configuration field  */
	
	/* Physical Configuration */
	unsigned char	xorigin;        /*!< LCMASK ACMASK Object x start position on matrix  */
	unsigned char	yorigin;        /*!< LCMASK ACMASK Object y start position on matrix  */
	unsigned char	xsize;          /*!< LCMASK ACMASK Object x size (i.e. width)         */
	unsigned char	ysize;          /*!< LCMASK ACMASK Object y size (i.e. height)        */
	
	/* Detection Configuration */
	unsigned char	akscfg;         /*!< Adjacent key suppression config     */
	unsigned char	blen;           /*!< Sets the gain of the analog circuits in front of the ADC. The gain should be set in
	                        			conjunction with the burst length to optimize the signal acquisition. Maximum gain values for
	                        			a given object/burst length can be obtained following a full calibration of the system. GAIN
	                        			has a maximum setting of 4; settings above 4 are capped at 4.*/
	unsigned char	tchthr;         /*!< ACMASK Threshold for all object channels   */
	unsigned char	tchdi;          /*!< Detect integration config           */
	
	unsigned char	orient;  		/*!< LCMASK Controls flipping and rotating of touchscreen
	                    			 *   object */
	unsigned char	mrgtimeout; 	/*!< Timeout on how long a touch might ever stay
	                    			 *   merged - units of 0.2s, used to tradeoff power
	                    			 *   consumption against being able to detect a touch
	                    			 *   de-merging early */
	
	/* Position Filter Configuration */
	unsigned char	movhysti;   	/*!< Movement hysteresis setting used after touchdown */
	unsigned char	movhystn;  		/*!< Movement hysteresis setting used once dragging   */
	unsigned char	movfilter;  	/*!< Position filter setting controlling the rate of  */
	
	/* Multitouch Configuration */
	unsigned char	numtouch;   /*!< The number of touches that the screen will attempt
	                    		 *   to track */
	unsigned char	mrghyst;    /*!< The hysteresis applied on top of the merge threshold
	                			 *   to stop oscillation */
	unsigned char	mrgthr;     /*!< The threshold for the point when two peaks are
	                    		 *   considered one touch */
	unsigned char	amphyst;	/*!< TBD */
	
	/* Resolution Controls */
	unsigned short	xrange;     /*!< LCMASK */
	unsigned short	yrange;     /*!< LCMASK */
	unsigned char		xloclip;    /*!< LCMASK */
	unsigned char		xhiclip;    /*!< LCMASK */
	unsigned char		yloclip;    /*!< LCMASK */
	unsigned char		yhiclip;    /*!< LCMASK */
	/* edge correction controls */
	unsigned char		xedgectrl;  /*!< LCMASK */
	unsigned char		xedgedist;  /*!< LCMASK */
	unsigned char		yedgectrl;  /*!< LCMASK */
	unsigned char		yedgedist;  /*!< LCMASK */
}	__attribute__ ((packed)) touchscreen_config_t;

typedef struct	keyarray_config__t	{
	/* Key Array Configuration */
	unsigned char	ctrl;			/*!< ACENABLE LCENABLE Main configuration field       */
	
	/* Physical Configuration */
	unsigned char	xorigin;		/*!< ACMASK LCMASK Object x start position on matrix  */
	unsigned char	yorigin;		/*!< ACMASK LCMASK Object y start position on matrix  */
	unsigned char	xsize;      	/*!< ACMASK LCMASK Object x size (i.e. width)         */
	unsigned char	ysize;      	/*!< ACMASK LCMASK Object y size (i.e. height)        */
	
	/* Detection Configuration */
	unsigned char	akscfg;     	/*!< Adjacent key suppression config     */
	unsigned char	blen;       	/*!< ACMASK Burst length for all object channels*/
	unsigned char	tchthr;     	/*!< ACMASK LCMASK Threshold for all object channels   */
	unsigned char	tchdi;      	/*!< Detect integration config           */
	unsigned char	reserved[2];	/*!< Spare x2 */
}	__attribute__ ((packed)) 	keyarray_config_t;

typedef struct	comc_config__t	{
	unsigned char	ctrl;
	unsigned char	cmd;
}	__attribute__ ((packed)) 	comc_config_t;

typedef struct	gpiopwm_config___t	{
	/* GPIOPWM Configuration */
	unsigned char	ctrl;             /*!< Main configuration field           */
	unsigned char	reportmask;       /*!< Event mask for generating messages to
	                		           *   the host */
	unsigned char	dir;              /*!< Port DIR register   */
	unsigned char	intpullup;        /*!< Port pull-up per pin enable register */
	unsigned char	out;              /*!< Port OUT register*/
	unsigned char	wake;             /*!< Port wake on change enable register  */
	unsigned char	pwm;              /*!< Port pwm enable register    */
	unsigned char	period;           /*!< PWM period (min-max) percentage*/
	unsigned char	duty[4];          /*!< PWM duty cycles percentage */
}	__attribute__ ((packed)) 	gpiopwm_config_t;

typedef struct	gripfacesuppression_config__t	{
	unsigned char	ctrl;
	unsigned char	xlogrip;
	unsigned char	xhigrip;
	unsigned char	ylogrip;
	unsigned char	yhigrip;
	unsigned char	maxtchs;
	unsigned char	reserved;
	unsigned char	szthr1;
	unsigned char	szthr2;
	unsigned char	shpthr1;
	unsigned char	shpthr2;
	unsigned char	supextto;
}	__attribute__ ((packed)) 	gripfacesuppression_config_t;

typedef struct	noise_suppression_config__t	{
	unsigned char	ctrl;
	unsigned char	reserved;
	unsigned char	reserved1;
	
	unsigned short	gcaful;
	unsigned short	gcafll;
	
	unsigned char	actvgcafvalid;        /* LCMASK */
	unsigned char	noisethr;
	unsigned char	reserved2;
	unsigned char	freqhopscale;
	unsigned char	freq[5u];             /* LCMASK ACMASK */
	unsigned char	idlegcafvalid;        /* LCMASK */
}	__attribute__ ((packed)) 	noise_suppression_config_t;

typedef struct	proximity_config__t	{
	/* Prox Configuration */
	unsigned char	ctrl;              /*!< ACENABLE LCENABLE Main configuration field           */
	
	/* Physical Configuration */
	unsigned char	xorigin;           /*!< ACMASK LCMASK Object x start position on matrix  */
	unsigned char	yorigin;           /*!< ACMASK LCMASK Object y start position on matrix  */
	unsigned char	xsize;             /*!< ACMASK LCMASK Object x size (i.e. width)         */
	unsigned char	ysize;             /*!< ACMASK LCMASK Object y size (i.e. height)        */
	unsigned char	reserved_for_future_aks_usage;
	/* Detection Configuration */
	unsigned char	blen;               /*!< ACMASK Burst length for all object channels*/
	unsigned short	tchthr;             /*!< LCMASK Threshold    */
	unsigned char	tchdi;              /*!< Detect integration config           */
	unsigned char	average;            /*!< LCMASK Sets the filter length on the prox signal */
	unsigned short	rate;               /*!< Sets the rate that prox signal must exceed */
}	__attribute__ ((packed)) 	proximity_config_t;

typedef struct	onetouch_gesture_config__t	{
	unsigned char	ctrl;
	unsigned char	numgest;
	
	unsigned short	gesten;
	
	unsigned char	pressproc;
	unsigned char	tapto;
	unsigned char	flickto;
	unsigned char	dragto;
	unsigned char	spressto;
	unsigned char	lpressto;
	unsigned char	reppressto;
	
	unsigned short	flickthr;
	unsigned short	dragthr;
	unsigned short	tapthr;
	unsigned short	throwthr;
}	__attribute__ ((packed)) 	onetouch_gesture_config_t;

typedef struct	selftest_config__t	{
	unsigned char		ctrl;                 /* LCENABLE */
	unsigned char		cmd;
}	__attribute__ ((packed)) 	selftest_config_t;

typedef struct	twotouch_gesture_config__t	{
	unsigned char	ctrl;          /*!< Bit 0 = object enable, bit 1 = report enable */
    unsigned char	numgest;       /*!< Runtime control for how many two touch gestures
                    		        *   to process */
    unsigned char	reserved2;
    unsigned char	gesten;        /*!< Control for turning particular gestures on or
                            		*   off */
    unsigned char	rotatethr;
    unsigned short	zoomthr;
}	__attribute__ ((packed)) 	twotouch_gesture_config_t;

typedef struct	cte_config__t	{
	unsigned char	ctrl;          /*!< Ctrl field reserved for future expansion */
	unsigned char	cmd;           /*!< Cmd field for sending CTE commands */
	unsigned char	mode;          /*!< LCMASK CTE mode configuration field */
	unsigned char	idlegcafdepth; /*!< LCMASK The global gcaf number of averages when idle */
	unsigned char	actvgcafdepth; /*!< LCMASK The global gcaf number of averages when active */
}	__attribute__ ((packed)) 	cte_config_t;


//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
extern	int				qt602240_set_config						(void);
extern	int				qt602240_init							(void);
extern	unsigned char	get_object_size							(unsigned char type);
extern	unsigned int	get_object_address						(unsigned char type, unsigned char instance);

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
#endif		/* _QT602240_H_ */
//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
