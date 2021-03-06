/* linux/arch/arm/plat-s5p/include/plat/fb.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Core file for Samsung Display Controller (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_PLAT_FB_H
#define __ASM_PLAT_FB_H __FILE__

#define FB_SWAP_WORD	(1 << 24)
#define FB_SWAP_HWORD	(1 << 16)
#define FB_SWAP_BYTE	(1 << 8)
#define FB_SWAP_BIT	(1 << 0)

struct platform_device;
struct clk;

struct s3c_platform_fb {
	int		hw_ver;
	char		clk_name[16];
	int		nr_wins;
	int		nr_buffers[5];
	int		default_win;
	int		swap;

	void		(*cfg_gpio)(struct platform_device *dev);
	int		(*backlight_on)(struct platform_device *dev);
	int		(*backlight_off)(struct platform_device *dev);
#if defined(CONFIG_HKDKC110_BOARD)
	int		(*reset_lcd)(struct platform_device *dev);
#endif
	int		(*lcd_on)(struct platform_device *dev);
	int		(*lcd_off)(struct platform_device *dev);
	int		(*clk_on)(struct platform_device *pdev, struct clk **s3cfb_clk);
	int		(*clk_off)(struct platform_device *pdev, struct clk **clk);
};

extern void s3cfb_set_platdata(struct s3c_platform_fb *fimd);

#if defined(CONFIG_FB_S3C_LMS700KF23)
extern int s3cfb_reset_lcd(struct platform_device *pdev);
#endif

/* defined by architecture to configure gpio */
extern void s3cfb_cfg_gpio(struct platform_device *pdev);
extern int s3cfb_backlight_on(struct platform_device *pdev);
extern int s3cfb_backlight_off(struct platform_device *pdev);
extern int s3cfb_lcd_on(struct platform_device *pdev);
extern int s3cfb_lcd_off(struct platform_device *pdev);
extern int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk);
extern int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk);
extern void s3cfb_get_clk_name(char *clk_name);
#endif /* __ASM_PLAT_FB_H */
