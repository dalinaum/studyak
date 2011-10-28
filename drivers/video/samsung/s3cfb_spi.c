/*
 * drivers/video/s3c/s3c24xxfb_spi.c
 *
 * $Id: s3cfb_spi.c,v 1.1 2008/11/17 11:12:08 jsgood Exp $
 *
 * Copyright (C) 2008 Jinsung Yang <jsgood.yang@samsung.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	S3C Frame Buffer Driver
 *	based on skeletonfb.c, sa1100fb.h, s3c2410fb.c
 */
//[*]--------------------------------------------------------------------------------------------------------------------------------------------
#include <linux/delay.h>

#include <asm/mach/map.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-clock.h>

//[*]--------------------------------------------------------------------------------------------------------------------------------------------
#define	S3C_FB_SPI_CH			1

#define S5P_FB_SPI_CLK(x)		(S5PV210_GPB(0 + (x * 4)))
#define S5P_FB_SPI_CS(x)		(S5PV210_GPB(1 + (x * 4)))
//#define S5P_FB_SPI_RESET(x)		(S5PV210_GPB(2 + (x * 4)))
#define S5P_FB_SPI_MOSI(x)		(S5PV210_GPB(3 + (x * 4)))

//[*]--------------------------------------------------------------------------------------------------------------------------------------------
int s3cfb_spi_gpio_request(int ch)
{
	if(gpio_request(S5P_FB_SPI_CLK(ch), "GPB"))		{
		printk("spi clk request error!\n");		
		return	1;
	}
	if(gpio_request(S5P_FB_SPI_MOSI(ch), "GPB"))	{
		printk("spi mosi request error!\n");		
		goto err_mosi;
	}
	if(gpio_request(S5P_FB_SPI_CS(ch), "GPB"))	{
		printk("spi cs request error!\n");		
		goto err_cs;
	}
	
	return	0;
err_cs:
        gpio_free(S5P_FB_SPI_MOSI(ch));
	
err_mosi:
        gpio_free(S5P_FB_SPI_CLK(ch));
	
	return	1;	
}

//[*]--------------------------------------------------------------------------------------------------------------------------------------------
inline void s3cfb_spi_lcd_dclk(int ch, int value)
{
        gpio_set_value(S5P_FB_SPI_CLK(ch), value);
}

//[*]--------------------------------------------------------------------------------------------------------------------------------------------
inline void s3cfb_spi_lcd_dseri(int ch, int value)
{
        gpio_set_value(S5P_FB_SPI_MOSI(ch), value);
}

//[*]--------------------------------------------------------------------------------------------------------------------------------------------
inline void s3cfb_spi_lcd_den(int ch, int value)
{
        gpio_set_value(S5P_FB_SPI_CS(ch), value);
}

//[*]--------------------------------------------------------------------------------------------------------------------------------------------
inline void s3cfb_spi_set_lcd_data(int ch)
{
        gpio_direction_output(S5P_FB_SPI_CLK(ch), 1);
        gpio_direction_output(S5P_FB_SPI_MOSI(ch), 1);
        gpio_direction_output(S5P_FB_SPI_CS(ch), 1);

        s3c_gpio_setpull(S5P_FB_SPI_CLK(ch), S3C_GPIO_PULL_NONE);
        s3c_gpio_setpull(S5P_FB_SPI_MOSI(ch), S3C_GPIO_PULL_NONE);
        s3c_gpio_setpull(S5P_FB_SPI_CS(ch), S3C_GPIO_PULL_NONE);
}

//[*]--------------------------------------------------------------------------------------------------------------------------------------------
void s3cfb_spi_gpio_free(int ch)
{
        gpio_free(S5P_FB_SPI_CLK(ch));
        gpio_free(S5P_FB_SPI_MOSI(ch));
        gpio_free(S5P_FB_SPI_CS(ch));
}

//[*]--------------------------------------------------------------------------------------------------------------------------------------------
void s3cfb_spi_write(int address, int data)
{
	unsigned int 	delay = 50;
	unsigned char 	dev_id = 0x1d;
	int 			i;

	s3cfb_spi_lcd_den(S3C_FB_SPI_CH, 1);
	s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 1);
	s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 1);
	udelay(delay);

	s3cfb_spi_lcd_den(S3C_FB_SPI_CH, 0);
	udelay(delay);

	for (i = 5; i >= 0; i--) {
		s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 0);

		if ((dev_id >> i) & 0x1)
			s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 1);
		else
			s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 0);

		udelay(delay);

		s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 1);
		udelay(delay);
	}

	s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 0);
	s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 0);
	udelay(delay);

	s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 1);
	udelay(delay);

	s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 0);
	s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 0);
	udelay(delay);

	s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 1);
	udelay(delay);

	for (i = 15; i >= 0; i--) {
		s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 0);

		if ((address >> i) & 0x1)
			s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 1);
		else
			s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 0);

		udelay(delay);

		s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 1);
		udelay(delay);
	}

	s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 1);
	udelay(delay);

	s3cfb_spi_lcd_den(S3C_FB_SPI_CH, 1);
	udelay(delay * 10);

	s3cfb_spi_lcd_den(S3C_FB_SPI_CH, 0);
	udelay(delay);

	for (i = 5; i >= 0; i--) {
		s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 0);

		if ((dev_id >> i) & 0x1)
			s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 1);
		else
			s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 0);

		udelay(delay);

		s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 1);
		udelay(delay);

	}

	s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 0);
	s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 1);
	udelay(delay);

	s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 1);
	udelay(delay);

	s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 0);
	s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 0);
	udelay(delay);

	s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 1);
	udelay(delay);

	for (i = 15; i >= 0; i--) {
		s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 0);

		if ((data >> i) & 0x1)
			s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 1);
		else
			s3cfb_spi_lcd_dseri(S3C_FB_SPI_CH, 0);

		udelay(delay);

		s3cfb_spi_lcd_dclk(S3C_FB_SPI_CH, 1);
		udelay(delay);
	}

	s3cfb_spi_lcd_den(S3C_FB_SPI_CH, 1);

	udelay(1000);
}

//[*]--------------------------------------------------------------------------------------------------------------------------------------------
