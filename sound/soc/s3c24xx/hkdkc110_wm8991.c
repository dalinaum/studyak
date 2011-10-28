/*
 * hkdkc110_wm8991.c
 *
 * Copyright (C) 2009, Samsung Elect. Ltd. - Jaswinder Singh <jassisinghbrar@gmail.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <linux/irq.h>
#include <linux/timer.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/io.h>
#include <asm/gpio.h> 
#include <plat/gpio-cfg.h> 
#include <plat/map-base.h>
#include <mach/regs-clock.h>
#include <mach/regs-audss.h>

#include <mach/gpio-bank.h>

#include "../codecs/wm8991.h"
#include "s3c64xx-i2s.h"
//#include "s5pc11x-pcm.h"


//[*]--------------------------------------------------------------------------------------------------[*]
#define IRQ_HPJACK			IRQ_EINT6
#define GPIO_HPJACK			S5PV210_GPH0(6)
#define GPIO_HPJACK_NAME	"GPH0"
#define GPIO_HPJACK_CHK_BIT	(0x0 | (1<<6))

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
static struct delayed_work dwork_ear_detect;

u8		g_current_out;
bool	g_btvout;

static int lowpower = 0;
extern struct snd_soc_platform s3c_dma_wrapper;
extern const struct snd_kcontrol_new s5p_idma_control;

extern bool get_hdmi_onoff_status(void);
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
irqreturn_t	hkdkc110_hpjack_isr(int irq, void *dev_id)
{
	
//	disable_irq(IRQ_HPJACK);
	// Kernel Timer restart
	schedule_delayed_work(&dwork_ear_detect, msecs_to_jiffies(500));

	return	IRQ_HANDLED;
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
int	hkdkc110_hpjack_detect(struct delayed_work *dwork)
{
	int err; 

	if (gpio_is_valid(GPIO_HPJACK))
	{
		err = gpio_request( GPIO_HPJACK, GPIO_HPJACK_NAME);
		if(err)	return err;

		if(gpio_get_value(GPIO_HPJACK) && (g_current_out!=WM8991_SPK))
			g_current_out=WM8991_SPK;
		else if(!gpio_get_value(GPIO_HPJACK) && (g_current_out!=WM8991_HP))
			g_current_out=WM8991_HP;
		gpio_free(GPIO_HPJACK);
	}
	if(!g_btvout) wm8991_set_outpath(g_current_out);

//	enable_irq(IRQ_HPJACK);
	
	return err;
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
void	hkdkc110_audio_tvout(bool tvout)
{
#if defined(CONFIG_TOUCHSCREEN_X10)
	g_btvout = false;
#else
	u8 temp; 
	
	if(tvout) {
		g_btvout = true;
		temp = WM8991_NONE;
	}
	else {
		g_btvout = false;
		temp = g_current_out;
	}
	wm8991_set_outpath(temp);
#endif
}
EXPORT_SYMBOL(hkdkc110_audio_tvout);

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
static int set_epll_rate(unsigned long rate)
{
	struct clk *fout_epll;
	
	fout_epll = clk_get(NULL, "fout_epll");
	if (IS_ERR(fout_epll)) {
		printk(KERN_ERR "%s: failed to get fout_epll\n", __func__);
		return -ENOENT;
	}
	
	if (rate == clk_get_rate(fout_epll))
		goto out;
	
	clk_disable(fout_epll);
	clk_set_rate(fout_epll, rate);
	clk_enable(fout_epll);

out:
    clk_put(fout_epll);

    return 0;
}

/* XXX BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2) XXX */
/* XXX BFS --> RFS(must be a multiple of BFS)                                 XXX */
static int hkdkc110_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	unsigned int rclk, psr=1;
	int bfs, rfs, ret;
	unsigned long epll_out_rate;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_U24:
	case SNDRV_PCM_FORMAT_S24:
		bfs = 48;
		break;
	case SNDRV_PCM_FORMAT_U16_LE:
	case SNDRV_PCM_FORMAT_S16_LE:
		bfs = 32;
		break;
	default:
		return -EINVAL;
	}

	/* The Fvco for WM8580 PLLs must fall within [90,100]MHz.
	 * This criterion can't be met if we request PLL output
	 * as {8000x256, 64000x256, 11025x256}Hz.
	 * As a wayout, we rather change rfs to a minimum value that
	 * results in (params_rate(params) * rfs), and itself, acceptable
	 * to both - the CODEC and the CPU.
	 */
	switch (params_rate(params)) {
	case 16000:
	case 22050:
	case 24000:
	case 32000:
	case 44100:
	case 48000:
	case 88200:
	case 96000:
		if (bfs == 48)
			rfs = 384;
		else
			rfs = 256;
		break;
	case 64000:
		rfs = 384;
		break;
	case 8000:
	case 11025:
	case 12000:
		if (bfs == 48)
			rfs = 768;
		else
			rfs = 512;
		break;
	default:
		return -EINVAL;
	}

	rclk = params_rate(params) * rfs;

	switch (rclk) {
	case 4096000:
	case 5644800:
	case 6144000:
	case 8467200:
	case 9216000:
		psr = 8;
		break;
	case 8192000:
	case 11289600:
	case 12288000:
	case 16934400:
	case 18432000:
		psr = 4;
		break;
	case 22579200:
	case 24576000:
	case 33868800:
	case 36864000:
		psr = 2;
		break;
	case 67737600:
	case 73728000:
		psr = 1;
		break;
	default:
		printk("Not yet supported!\n");
		return -EINVAL;
	}

	epll_out_rate = rclk * psr;

	/* Set EPLL clock rate */
	ret = set_epll_rate(epll_out_rate);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_CDCLK,
					0, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

	/* We use MUX for basic ops in SoC-Master mode */
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_MUX,
					0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_PRESCALER, psr-1);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_BCLK, bfs);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_RCLK, rfs);
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * WM8991 DAI operations.
 */
static struct snd_soc_ops hkdkc110_ops = {
	.hw_params = hkdkc110_hw_params,
};

/* SMDK64xx Capture widgets */
static const struct snd_soc_dapm_widget wm8991_dapm_widgets_cpt[] = {
	SND_SOC_DAPM_MIC("MicIn", NULL),
};

/* SMDK-PAIFTX connections */
static const struct snd_soc_dapm_route audio_map_tx[] = {
	/* MicIn feeds AINL */
	{"AINL", NULL, "MicIn"},
};

static int hkdkc110_wm8991_init(struct snd_soc_codec *codec)
{
	int err, ret;

	/* Setup Default Route */
	g_current_out = WM8991_NONE;

	if (gpio_is_valid(GPIO_HPJACK))
	{
		err = gpio_request( GPIO_HPJACK, GPIO_HPJACK_NAME);
		if(err)
		{
			printk(KERN_ERR "failed to request GPJ0 for wm8991 i2c clk..\n");
			return err;
		}
		gpio_direction_input(GPIO_HPJACK);
		s3c_gpio_setpull(GPIO_HPJACK, S3C_GPIO_PULL_UP);
		
		if(gpio_get_value(GPIO_HPJACK)) g_current_out=WM8991_SPK;
		else 							g_current_out=WM8991_HP;
		
		gpio_free(GPIO_HPJACK);
	}
	err = request_irq(IRQ_HPJACK, hkdkc110_hpjack_isr, IRQF_DISABLED, "hpjack-irq", NULL);
	if(err) printk("\nDEBUG -> IRQ_HPJACK request error!!!\n\n");
	set_irq_type(IRQ_HPJACK, IRQ_TYPE_EDGE_FALLING);

	INIT_DELAYED_WORK(&dwork_ear_detect, hkdkc110_hpjack_detect);

#if defined(CONFIG_TOUCHSCREEN_X10) || defined(CONFIG_FB_S3C_SMD1024x600)
	g_btvout = false;
	wm8991_set_outpath(g_current_out);
#else
	if(get_hdmi_onoff_status()) {
		g_btvout = true;
		wm8991_set_outpath(WM8991_NONE);
	} else {
		g_btvout = false;
		wm8991_set_outpath(g_current_out);
	}
#endif

	/* add iDMA controls */
	ret = snd_soc_add_controls(codec, &s5p_idma_control, 1);
	if (ret < 0)
		return ret;

	/* signal a DAPM event */
	snd_soc_dapm_sync(codec);

	/* Set the Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(&wm8991_dai, SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set the AP DAI configuration */
	ret = snd_soc_dai_set_fmt(&s3c64xx_i2s_dai[0], SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	return 0;
}

static int hkdkc110_wm8991_init_tx(struct snd_soc_codec *codec)
{
	int ret;
	printk("\n%s\n",__FUNCTION__);

	snd_soc_dapm_new_controls(codec, wm8991_dapm_widgets_cpt,
				  ARRAY_SIZE(wm8991_dapm_widgets_cpt));

	/* Set up PAIFTX audio path */
	snd_soc_dapm_add_routes(codec, audio_map_tx, ARRAY_SIZE(audio_map_tx));

	/* Enabling the microphone requires the fitting of a 0R
	 * resistor to connect the line from the microphone jack.
	 */
	snd_soc_dapm_disable_pin(codec, "MicIn");

	/* signal a DAPM event */
	snd_soc_dapm_sync(codec);

	/* Set the Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(&wm8991_dai, SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;
	/* Set the AP DAI configuration */
	ret = snd_soc_dai_set_fmt(&s3c64xx_i2s_dai[0], SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_dai_link hkdkc110_dai[] = {

{ /* Primary Playback i/f */
	.name = "WM8991 RX",
	.stream_name = "Playback",
	.cpu_dai = &s3c64xx_i2s_dai[0],
	.codec_dai = &wm8991_dai,
	.init = hkdkc110_wm8991_init,
	.ops = &hkdkc110_ops,
},
{ /* Primary Capture i/f */
	.name = "WM8991 TX",
	.stream_name = "Capture",
	.cpu_dai = &s3c64xx_i2s_dai[0],
	.codec_dai = &wm8991_dai,
	.init = hkdkc110_wm8991_init_tx,
	.ops = &hkdkc110_ops,
},
};

static struct snd_soc_card hkdkc110 = {
	.name = "hkdkc110",
	.platform =  &s3c_dma_wrapper,
	.dai_link = hkdkc110_dai,
	.num_links = ARRAY_SIZE(hkdkc110_dai),
};

static struct wm8991_setup_data hkdkc110_wm8991_setup = {
	.i2c_bus = 2,
	.i2c_address = 0x1a,
};

static struct snd_soc_device hkdkc110_snd_devdata = {
	.card = &hkdkc110,
	.codec_dev = &soc_codec_dev_wm8991,
	.codec_data = &hkdkc110_wm8991_setup,
};

static struct platform_device *hkdkc110_snd_device;

static int __init hkdkc110_audio_init(void)
{
	int ret;
	u32 val;
	u32 reg;

#include <mach/map.h>
#define S3C_VA_AUDSS	S3C_ADDR(0x01600000)	/* Audio SubSystem */
#include <mach/regs-audss.h>
	/* We use I2SCLK for rate generation, so set EPLLout as
	 * the parent of I2SCLK.
	 */
	val = readl(S5P_CLKSRC_AUDSS);
	val &= ~(0x3<<2);
	val |= (1<<0);
	writel(val, S5P_CLKSRC_AUDSS);

	val = readl(S5P_CLKGATE_AUDSS);
	val |= (0x7f<<0);
	writel(val, S5P_CLKGATE_AUDSS);

#ifdef CONFIG_S5P_LPAUDIO
	/* yman.seo CLKOUT is prior to CLK_OUT of SYSCON. XXTI & XUSBXTI work in sleep mode */
	reg = __raw_readl(S5P_OTHERS);
	reg &= ~(0x0003 << 8);
	reg |= 0x0003 << 8;	/* XUSBXTI */
	__raw_writel(reg, S5P_OTHERS);
#else
	/* yman.seo Set XCLK_OUT as 24MHz (XUSBXTI -> 24MHz) */
	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~S5P_CLKOUT_CLKSEL_MASK;
	reg |= S5P_CLKOUT_CLKSEL_XUSBXTI;
	reg &= ~S5P_CLKOUT_DIV_MASK;
	reg |= 0x0001 << S5P_CLKOUT_DIV_SHIFT;	/* DIVVAL = 1, Ratio = 2 = DIVVAL + 1 */
	__raw_writel(reg, S5P_CLK_OUT);

	reg = __raw_readl(S5P_OTHERS);
	reg &= ~(0x0003 << 8);
	reg |= 0x0000 << 8;	/* Clock from SYSCON */
	__raw_writel(reg, S5P_OTHERS);
#endif
	printk("In %s function\n",__FUNCTION__);

	hkdkc110_snd_device = platform_device_alloc("soc-audio", 0);
	if (!hkdkc110_snd_device)
		return -ENOMEM;

	platform_set_drvdata(hkdkc110_snd_device, &hkdkc110_snd_devdata);
	hkdkc110_snd_devdata.dev = &hkdkc110_snd_device->dev;
	ret = platform_device_add(hkdkc110_snd_device);

	if (ret)
		platform_device_put(hkdkc110_snd_device);

	return ret;
}

static void __exit hkdkc110_audio_exit(void)
{
	platform_device_unregister(hkdkc110_snd_device);
}

module_init(hkdkc110_audio_init);
module_exit(hkdkc110_audio_exit);

module_param (lowpower, int, 0444);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC HKDKC110 WM8991");
MODULE_LICENSE("GPL");
