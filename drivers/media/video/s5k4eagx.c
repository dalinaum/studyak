/* linux/drivers/media/video/s5k4eagx.c
 *
 * Copyright (c) 2010 Hardkernel Co., Ltd.
 * 		http://www.hardkernel.com/
 *
 * Driver for S5K4EAGX (SXGA camera) from Samsung Electronics
 * 1/4" 5Mp CMOS Image Sensor SoC with an Embedded Image Processor
 * supporting PVI
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-i2c-drv.h>
#include <media/s5k4eagx_platform.h>
#include <mach/gpio.h>
#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include "s5k4eagx.h"

#define S5K4EAGX_DRIVER_NAME	"S5K4EAGX"

/* Default resolution & pixelformat. plz ref s5k4eagx_platform.h */
#define DEFAULT_RES		WVGA	/* Index of resoultion */
#define DEFAUT_FPS_INDEX	S5K4EAGX_15FPS
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */

/*
 * Specification
 * Parallel : ITU-R. 656/601 YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Serial : MIPI CSI2 (single lane) YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Resolution : 1280 (H) x 1024 (V)
 * Image control : Brightness, Contrast, Saturation, Sharpness, Glamour
 * Effect : Mono, Negative, Sepia, Aqua, Sketch
 * FPS : 15fps @full resolution, 30fps @VGA, 24fps @720p
 * Max. pixel clock frequency : 48MHz(upto)
 * Internal PLL (6MHz to 27MHz input frequency)
 */

/* Camera functional setting values configured by user concept */
struct s5k4eagx_userset {
	signed int exposure_bias;	/* V4L2_CID_EXPOSURE */
	unsigned int ae_lock;
	unsigned int awb_lock;
	unsigned int auto_wb;	/* V4L2_CID_AUTO_WHITE_BALANCE */
	unsigned int manual_wb;	/* V4L2_CID_WHITE_BALANCE_PRESET */
	unsigned int wb_temp;	/* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int contrast;	/* V4L2_CID_CONTRAST */
	unsigned int saturation;	/* V4L2_CID_SATURATION */
	unsigned int sharpness;		/* V4L2_CID_SHARPNESS */
	unsigned int glamour;
};

struct s5k4eagx_state {
	struct s5k4eagx_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct s5k4eagx_userset userset;
	int freq;	/* MCLK in KHz */
	int is_mipi;
	int isize;
	int ver;
	int fps;
};

static inline struct s5k4eagx_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct s5k4eagx_state, sd);
}

/*
 * S5K4EAGX register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int s5k4eagx_write(struct v4l2_subdev *sd, u16 addr, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[1];
	unsigned char reg[4];
	int err = 0;
	int retry = 0;


	if (!client->adapter)
		return -ENODEV;

again:
	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 4;
	msg->buf = reg;

	reg[0] = addr >> 8;
	reg[1] = addr & 0xff;
	reg[2] = val >> 8;
	reg[3] = val & 0xff;

	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0)
		return err;	/* Returns here on success */

	/* abnormal case: retry 5 times */
	if (retry < 5) {
		dev_err(&client->dev, "%s: address: 0x%02x%02x, " \
			"value: 0x%02x%02x\n", __func__, \
			reg[0], reg[1], reg[2], reg[3]);
		retry++;
		goto again;
	}

	return err;
}

static int s5k4eagx_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[],
				unsigned char length)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[length], i;
	struct i2c_msg msg = {client->addr, 0, length, buf};

	for (i = 0; i < length; i++)
		buf[i] = i2c_data[i];

	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

static const char *s5k4eagx_querymenu_wb_preset[] = {
	"WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL
};

static const char *s5k4eagx_querymenu_effect_mode[] = {
	"Effect Sepia", "Effect Aqua", "Effect Monochrome",
	"Effect Negative", "Effect Sketch", NULL
};

static const char *s5k4eagx_querymenu_ev_bias_mode[] = {
	"-3EV",	"-2,1/2EV", "-2EV", "-1,1/2EV",
	"-1EV", "-1/2EV", "0", "1/2EV",
	"1EV", "1,1/2EV", "2EV", "2,1/2EV",
	"3EV", NULL
};

static struct v4l2_queryctrl s5k4eagx_controls[] = {
	{
		/*
		 * For now, we just support in preset type
		 * to be close to generic WB system,
		 * we define color temp range for each preset
		 */
		.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "White balance in kelvin",
		.minimum = 0,
		.maximum = 10000,
		.step = 1,
		.default_value = 0,	/* FIXME */
	},
	{
		.id = V4L2_CID_WHITE_BALANCE_PRESET,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "White balance preset",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k4eagx_querymenu_wb_preset) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_AUTO_WHITE_BALANCE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Auto white balance",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_EXPOSURE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Exposure bias",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k4eagx_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value = (ARRAY_SIZE(s5k4eagx_querymenu_ev_bias_mode) \
				- 2) / 2,	/* 0 EV */
	},
	{
		.id = V4L2_CID_COLORFX,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k4eagx_querymenu_effect_mode) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_CONTRAST,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Contrast",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_SATURATION,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Saturation",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_SHARPNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Sharpness",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
};

const char **s5k4eagx_ctrl_get_menu(u32 id)
{
	switch (id) {
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return s5k4eagx_querymenu_wb_preset;

	case V4L2_CID_COLORFX:
		return s5k4eagx_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return s5k4eagx_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *s5k4eagx_find_qctrl(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(s5k4eagx_controls); i++)
		if (s5k4eagx_controls[i].id == id)
			return &s5k4eagx_controls[i];

	return NULL;
}

static int s5k4eagx_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(s5k4eagx_controls); i++) {
		if (s5k4eagx_controls[i].id == qc->id) {
			memcpy(qc, &s5k4eagx_controls[i], \
				sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int s5k4eagx_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	qctrl.id = qm->id;
	s5k4eagx_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, s5k4eagx_ctrl_get_menu(qm->id));
}

/*
 * Clock configuration
 * Configure expected MCLK from host and return EINVAL if not supported clock
 * frequency is expected
 * 	freq : in Hz
 * 	flag : not supported for now
 */
static int s5k4eagx_s_crystal_freq(struct v4l2_subdev *sd, u32  freq, u32 flags)
{
	int err = -EINVAL;

	return err;
}

static int s5k4eagx_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}

static int s5k4eagx_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}
static int s5k4eagx_enum_framesizes(struct v4l2_subdev *sd,
					struct v4l2_frmsizeenum *fsize)
{
	int err = 0;

	return err;
}

static int s5k4eagx_enum_frameintervals(struct v4l2_subdev *sd,
					struct v4l2_frmivalenum *fival)
{
	int err = 0;

	return err;
}

static int s5k4eagx_enum_fmt(struct v4l2_subdev *sd,
				struct v4l2_fmtdesc *fmtdesc)
{
	int err = 0;

	return err;
}

static int s5k4eagx_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}

static int s5k4eagx_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	int err = 0;

	return err;
}

static int s5k4eagx_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	int err = 0;

	return err;
}

static int s5k4eagx_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4eagx_state *state = to_state(sd);
	struct s5k4eagx_userset userset = state->userset;
	int err = -EINVAL;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		ctrl->value = userset.exposure_bias;
		err = 0;
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		ctrl->value = userset.auto_wb;
		err = 0;
		break;
	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = userset.manual_wb;
		err = 0;
		break;
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		ctrl->value = userset.wb_temp;
		err = 0;
		break;
	case V4L2_CID_COLORFX:
		ctrl->value = userset.effect;
		err = 0;
		break;
	case V4L2_CID_CONTRAST:
		ctrl->value = userset.contrast;
		err = 0;
		break;
	case V4L2_CID_SATURATION:
		ctrl->value = userset.saturation;
		err = 0;
		break;
	case V4L2_CID_SHARPNESS:
		ctrl->value = userset.saturation;
		err = 0;
		break;

	case V4L2_CID_CAM_JPEG_MAIN_SIZE:
		break;
	
	case V4L2_CID_CAM_JPEG_MAIN_OFFSET:
		break;

	case V4L2_CID_CAM_JPEG_POSTVIEW_OFFSET:
		break;

		
	default:
		dev_err(&client->dev, "%s: no such ctrl\n", __func__);
		break;
	}

	return err;
}

static int s5k4eagx_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
#ifdef S5K4EAGX_COMPLETE
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4eagx_state *state = to_state(sd);
	struct s5k4eagx_userset userset = state->userset;
	int err = -EINVAL;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		dev_dbg(&client->dev, "%s: V4L2_CID_EXPOSURE\n", \
			__func__);
		err = s5k4eagx_write_regs(sd, s5k4eagx_regs_ev_bias[ctrl->value]);
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		dev_dbg(&client->dev, "%s: V4L2_CID_AUTO_WHITE_BALANCE\n", \
			__func__);
		err = s5k4eagx_write_regs(sd, \
			s5k4eagx_regs_awb_enable[ctrl->value]);
		break;
	case V4L2_CID_WHITE_BALANCE_PRESET:
		dev_dbg(&client->dev, "%s: V4L2_CID_WHITE_BALANCE_PRESET\n", \
			__func__);
		err = s5k4eagx_write_regs(sd, \
			s5k4eagx_regs_wb_preset[ctrl->value]);
		break;
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		dev_dbg(&client->dev, \
			"%s: V4L2_CID_WHITE_BALANCE_TEMPERATURE\n", __func__);
		err = s5k4eagx_write_regs(sd, \
			s5k4eagx_regs_wb_temperature[ctrl->value]);
		break;
	case V4L2_CID_COLORFX:
		dev_dbg(&client->dev, "%s: V4L2_CID_COLORFX\n", __func__);
		err = s5k4eagx_write_regs(sd, \
			s5k4eagx_regs_color_effect[ctrl->value]);
		break;
	case V4L2_CID_CONTRAST:
		dev_dbg(&client->dev, "%s: V4L2_CID_CONTRAST\n", __func__);
		err = s5k4eagx_write_regs(sd, \
			s5k4eagx_regs_contrast_bias[ctrl->value]);
		break;
	case V4L2_CID_SATURATION:
		dev_dbg(&client->dev, "%s: V4L2_CID_SATURATION\n", __func__);
		err = s5k4eagx_write_regs(sd, \
			s5k4eagx_regs_saturation_bias[ctrl->value]);
		break;
	case V4L2_CID_SHARPNESS:
		dev_dbg(&client->dev, "%s: V4L2_CID_SHARPNESS\n", __func__);
		err = s5k4eagx_write_regs(sd, \
			s5k4eagx_regs_sharpness_bias[ctrl->value]);
		break;
	default:
		dev_err(&client->dev, "%s: no such control\n", __func__);
		break;
	}

	if (err < 0)
		goto out;
	else
		return 0;

out:
	dev_dbg(&client->dev, "%s: vidioc_s_ctrl failed\n", __func__);
	return err;
#else
	return 0;
#endif
}

int
__s5k4eagx_init_4bytes(struct v4l2_subdev *sd, unsigned char *reg[], int total)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL, i;
	unsigned char *item;

	for (i = 0; i < total; i++) {
		item = (unsigned char *) &reg[i];
		if (item[0] == REG_DELAY) {
			mdelay(item[1]);
			err = 0;
		} else {
			err = s5k4eagx_i2c_write(sd, item, 4);
		}

		if (err < 0)
			v4l_info(client, "%s: register set failed\n", \
			__func__);
	}

	return err;
}

static int
__s5k4eagx_init_2bytes(struct v4l2_subdev *sd, unsigned short *reg[], int total)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL, i;
	unsigned short *item;

	for (i = 0; i < total; i++) {
		item = (unsigned short *) &reg[i];
		if (item[0] == REG_DELAY) {
			mdelay(item[1]);
			err = 0;
		} else {
			err = s5k4eagx_write(sd, item[0], item[1]);
		}

		if (err < 0)
			v4l_info(client, "%s: register set failed\n", \
			__func__);
	}

	return err;
}

static unsigned short i2c_read_reg(struct i2c_client *client, unsigned short reg_h, unsigned short reg)
{
	int ret,i;
	unsigned char i2c_data[10];

	//{0x00, 0x28, 0x70, 0x00},  
	i2c_data[0]= 0x00;
	i2c_data[1]= 0x2c;
	i2c_data[2]= reg_h >>8;		//0x70;
	i2c_data[3]= reg_h & 0xff;	//0x00;

	i2c_master_send(client,i2c_data,4);
	

	i2c_data[0]= 0x00;
	i2c_data[1]= 0x2e;
	i2c_data[2]= (unsigned char)((reg>>8) & 0xff);
	i2c_data[3]= (unsigned char)(reg & 0xff);	
	

	i2c_master_send(client,i2c_data,4);

	i2c_data[0]=0x0f;
	i2c_data[1]=0x12;
	i2c_master_send(client,i2c_data,2);			
	

	ret = i2c_master_recv(client,i2c_data,2);

#if 0
	for(i=0;i<2;i++)
	printk("retdata %d => %x \n",i,i2c_data[i]);
#endif

#if 0
		if (ret < 0)
			printk( "%s: err %d\n", __func__, ret);
#endif
	
		return i2c_data[0]<<8 | i2c_data[1];
}

static int read_device_id(struct i2c_client *client)
{
	int id1;
	int id2;

	v4l_info(client,"Version infomation reg 0x01e4 :0x%x \n"	,id1=i2c_read_reg(client,0x7000, 0x1e4));
	v4l_info(client,"Revision information reg 0x01e6 :0x%x \n"	,id2=i2c_read_reg(client,0x7000, 0x1e6));

	if((id1 == 0x4ea) && ((id2 == 0x11) || id2 == 0x12)) return 1;

return 0;
}

static int s5k4eagx_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EIO;
	
	if(!read_device_id(client)) return err;

	v4l_info(client, "%s: camera initialization start\n", __func__);
	printk(client, "%s: camera initialization start\n", __func__);

	err = __s5k4eagx_init_4bytes(sd, \
		(unsigned char **) s5k4eagx_init_reg21, S5K4EAGX_INIT_REGS21);

		err = __s5k4eagx_init_2bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg22, S5K4EAGX_INIT_REGS22);

		err = __s5k4eagx_init_4bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg23, S5K4EAGX_INIT_REGS23);

		err = __s5k4eagx_init_4bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg24, S5K4EAGX_INIT_REGS24);
		err = __s5k4eagx_init_4bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg25, S5K4EAGX_INIT_REGS25);
		err = __s5k4eagx_init_4bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg26, S5K4EAGX_INIT_REGS26);
		err = __s5k4eagx_init_4bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg27, S5K4EAGX_INIT_REGS27);
		err = __s5k4eagx_init_4bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg28, S5K4EAGX_INIT_REGS28);
		err = __s5k4eagx_init_4bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg29, S5K4EAGX_INIT_REGS29);
		err = __s5k4eagx_init_4bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg30, S5K4EAGX_INIT_REGS30);
		err = __s5k4eagx_init_4bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg31, S5K4EAGX_INIT_REGS31);
		err = __s5k4eagx_init_4bytes(sd, \
			(unsigned char **) s5k4eagx_init_reg32, S5K4EAGX_INIT_REGS32);

	if (err < 0) {
		v4l_err(client, "%s: camera initialization failed\n", \
			__func__);
		return -EIO;	/* FIXME */
	}
	return 0;
}

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize every single opening time
 * therefor,it is not necessary to be initialized on probe time.
 * except for version checking
 * NOTE: version checking is optional
 */
static int s5k4eagx_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4eagx_state *state = to_state(sd);
	struct s5k4eagx_platform_data *pdata;

	dev_info(&client->dev, "fetching platform data\n");

	pdata = client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -ENODEV;
	}

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	if (!(pdata->default_width && pdata->default_height)) {
		/* TODO: assign driver default resolution */
	} else {
		state->pix.width = pdata->default_width;
		state->pix.height = pdata->default_height;
	}

	if (!pdata->pixelformat)
		state->pix.pixelformat = DEFAULT_FMT;
	else
		state->pix.pixelformat = pdata->pixelformat;

	if (!pdata->freq)
		state->freq = 48000000;	/* 48MHz default */
	else
		state->freq = pdata->freq;

	if (!pdata->is_mipi) {
		state->is_mipi = 0;
		dev_info(&client->dev, "parallel mode\n");
	} else
		state->is_mipi = pdata->is_mipi;

	return 0;
}

static int s5k4eagx_sleep(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL, i;

	v4l_info(client, "%s: sleep mode\n", __func__);

	for (i = 0; i < S5K4EAGX_SLEEP_REGS; i++) {
		if (s5k4eagx_sleep_reg[i][0] == REG_DELAY) {
			mdelay(s5k4eagx_sleep_reg[i][1]);
			err = 0;
		} else {
			err = s5k4eagx_write(sd, s5k4eagx_sleep_reg[i][0], \
				s5k4eagx_sleep_reg[i][1]);
		}

		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
	}

	if (err < 0) {
		v4l_err(client, "%s: sleep failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int s5k4eagx_wakeup(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL, i;

	v4l_info(client, "%s: wakeup mode\n", __func__);

	for (i = 0; i < S5K4EAGX_WAKEUP_REGS; i++) {
		if (s5k4eagx_wakeup_reg[i][0] == REG_DELAY) {
			mdelay(s5k4eagx_wakeup_reg[i][1]);
			err = 0;
		} else {
			err = s5k4eagx_write(sd, s5k4eagx_wakeup_reg[i][0], \
				s5k4eagx_wakeup_reg[i][1]);
		}

		if (err < 0)
			v4l_info(client, "%s: register set failed\n", __func__);
	}

	if (err < 0) {
		v4l_err(client, "%s: wake up failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int s5k4eagx_s_stream(struct v4l2_subdev *sd, int enable)
{
	return enable ? s5k4eagx_wakeup(sd) : s5k4eagx_sleep(sd);
}

static const struct v4l2_subdev_core_ops s5k4eagx_core_ops = {
	.init = s5k4eagx_init,	/* initializing API */
	.s_config = s5k4eagx_s_config,	/* Fetch platform data */
	.queryctrl = s5k4eagx_queryctrl,
	.querymenu = s5k4eagx_querymenu,
	.g_ctrl = s5k4eagx_g_ctrl,
	.s_ctrl = s5k4eagx_s_ctrl,
};

static const struct v4l2_subdev_video_ops s5k4eagx_video_ops = {
	.s_crystal_freq = s5k4eagx_s_crystal_freq,
	.g_fmt = s5k4eagx_g_fmt,
	.s_fmt = s5k4eagx_s_fmt,
	.enum_framesizes = s5k4eagx_enum_framesizes,
	.enum_frameintervals = s5k4eagx_enum_frameintervals,
	.enum_fmt = s5k4eagx_enum_fmt,
	.try_fmt = s5k4eagx_try_fmt,
	.g_parm = s5k4eagx_g_parm,
	.s_parm = s5k4eagx_s_parm,
	.s_stream = s5k4eagx_s_stream,
};

static const struct v4l2_subdev_ops s5k4eagx_ops = {
	.core = &s5k4eagx_core_ops,
	.video = &s5k4eagx_video_ops,
};
#ifdef DELETE_THIS_12341
void s5k4eagx_reset(int power_up)
{
#define S5PC11X_GPH0 S5PV210_GPH0
#define S5PC11X_GPH1 S5PV210_GPH1
#define S5PC11X_GPE1 S5PV210_GPE1

		printk( KERN_INFO "s5k4eagx reset\n");
		gpio_request(S5PC11X_GPE1(4), "GPE14");
		gpio_direction_output(S5PC11X_GPE1(4), 1);
		gpio_request(S5PC11X_GPH0(2), "GPH02");
		gpio_direction_output(S5PC11X_GPH0(2), 1);

		if(power_up)
		{
			//reset  --> L 
			gpio_set_value(S5PC11X_GPH0(2), 0);
			//STBY --> HIGH		
			gpio_set_value(S5PC11X_GPE1(4), 1);
			mdelay(5);
			//reset  --> H 			
			gpio_set_value(S5PC11X_GPH0(2), 1);
		}
		else //power down
		{
			//reset  --> L 
			gpio_set_value(S5PC11X_GPH0(2), 0);
			mdelay(5);
			//STBY --> LOW
			gpio_set_value(S5PC11X_GPE1(4), 0);
	
		}
		gpio_free(S5PC11X_GPE1(4));
		gpio_free(S5PC11X_GPH0(2));

}
#endif
/*
 * s5k4eagx_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int s5k4eagx_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct s5k4eagx_state *state;
	struct v4l2_subdev *sd;

	state = kzalloc(sizeof(struct s5k4eagx_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, S5K4EAGX_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &s5k4eagx_ops);

	dev_info(&client->dev, "s5k4eagx has been probed\n");
//	s5k4eagx_reset(1);
	return 0;
}


static int s5k4eagx_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
//	s5k4eagx_reset(0);
	return 0;
}

static const struct i2c_device_id s5k4eagx_id[] = {
	{ S5K4EAGX_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, s5k4eagx_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = S5K4EAGX_DRIVER_NAME,
	.probe = s5k4eagx_probe,
	.remove = s5k4eagx_remove,
	.id_table = s5k4eagx_id,
};

MODULE_DESCRIPTION("Samsung Electronics S5K4EAGX SXGA camera driver");
MODULE_AUTHOR("Dongsoo Nathaniel Kim<dongsoo45.kim@samsung.com>");
MODULE_LICENSE("GPL");

