/*
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MAX17040_BATTERY_H_
#define __MAX17040_BATTERY_H_
//#if defined(CONFIG_MACH_HKDKC110)
#define GPIO_CHARGER_ONLINE	S5PV210_GPH3(4)
#define GPIO_CHARGER_DONE	S5PV210_GPH3(5)
#define GPIO_CHARGER_STATUS	S5PV210_GPH3(6)
#define GPIO_CHARGER_ENABLE	S5PV210_GPG3(6)
//#endif

struct max17040_platform_data {
	int (*battery_online)(void);
	int (*charger_online)(void);
	int (*charger_enable)(void);
	int (*charger_done)(void);
	void (*charger_disable)(void);
};

#endif
