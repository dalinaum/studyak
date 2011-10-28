//[*]----------------------------------------------------------------------------------------------[*]
//
//
// 
//  HardKernel-C1XX gpio i2c driver (charles.park)
//  2009.07.22
// 
//
//[*]----------------------------------------------------------------------------------------------[*]
#ifndef	_HKC1XX_TOUCH_GPIO_I2C_H_
#define	_HKC1XX_TOUCH_GPIO_I2C_H_

//[*]----------------------------------------------------------------------------------------------[*]
extern	int				hkc1xx_touch_write			(unsigned char addr, unsigned char *wdata, unsigned char wsize);
extern	int 			hkc1xx_touch_read			(unsigned char *rdata, unsigned char rsize);

extern	void			hkc1xx_touch_port_init		(void);

//[*]----------------------------------------------------------------------------------------------[*]
#endif	//_HKC1XX_TOUCH_GPIO_I2C_H_
//[*]----------------------------------------------------------------------------------------------[*]
