/*
 * drivers/media/video/tp2825.c
 *
 * Copyright (C) ROCKCHIP, Inc.
 * Author:zhoupeng<benjo.zhou@rock-chips.com>
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "generic_sensor.h"
#include <linux/moduleparam.h>
/*
*      Driver Version Note
*v0.0.1: this driver is compatible with generic_sensor
*/
static int version = KERNEL_VERSION(0, 0, 1);
module_param(version, int, S_IRUGO);

static int debug;
module_param(debug, int, S_IRUGO|S_IWUSR);

#define dprintk(level, fmt, arg...) do {	\
	if (debug >= level)					\
	printk(KERN_WARNING fmt, ## arg);	\
} while (0)
#define debug_printk(format, ...) dprintk(1, format, ## __VA_ARGS__)
/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_TP2825
#define SENSOR_V4L2_IDENT V4L2_IDENT_TP2825
#define SENSOR_ID 0x2825
#define SENSOR_BUS_PARAM		(V4L2_MBUS_MASTER | \
					V4L2_MBUS_PCLK_SAMPLE_RISING | V4L2_MBUS_HSYNC_ACTIVE_HIGH | V4L2_MBUS_VSYNC_ACTIVE_HIGH | \
					V4L2_MBUS_DATA_ACTIVE_HIGH | SOCAM_MCLK_24MHZ)

#define NTSC_FULL 0
#define YUV_720P_50HZ 1
#define YUV_720P_30HZ 0
#define YUV_720P_25HZ 0
#if NTSC_FULL
#define SENSOR_PREVIEW_W                      960
#define SENSOR_PREVIEW_H                      480

static struct rk_camera_device_signal_config dev_info[] = {
	{
		.type = RK_CAMERA_DEVICE_CVBS_NTSC
	}
};
#endif

#if YUV_720P_50HZ | YUV_720P_25HZ | YUV_720P_30HZ
#define SENSOR_PREVIEW_W                     1280
#define SENSOR_PREVIEW_H                     720

static struct rk_camera_device_signal_config dev_info[] = {
	{
		.type = RK_CAMERA_DEVICE_BT601_8,
		.dvp = {
			.vsync = RK_CAMERA_DEVICE_SIGNAL_HIGH_LEVEL,
			.hsync = RK_CAMERA_DEVICE_SIGNAL_HIGH_LEVEL
		},
		.crop = {
			.top = 20,
			.left = 8
		}
	}
};
#endif

#define SENSOR_PREVIEW_FPS		30000		/* 30fps	*/
#define SENSOR_FULLRES_L_FPS		7500		/* 7.5fps	*/
#define SENSOR_FULLRES_H_FPS		7500		/* 7.5fps	*/
#define SENSOR_720P_FPS				0
#define SENSOR_1080P_FPS			0

#define SENSOR_REGISTER_LEN			1	/* sensor register address bytes */
#define SENSOR_VALUE_LEN			1	/* sensor register value bytes */

static unsigned int SensorConfiguration = (CFG_WhiteBalance | CFG_Effect | CFG_Scene);
static unsigned int SensorChipID[] = {SENSOR_ID};
/* Sensor Driver Configuration End */


#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a, b) CONS4(SensorReg, SENSOR_REGISTER_LEN, Val, SENSOR_VALUE_LEN)(a, b)
#define sensor_write(client, reg, v) CONS4(sensor_write_reg, SENSOR_REGISTER_LEN, val, SENSOR_VALUE_LEN)(client, (reg), (v))
#define sensor_read(client, reg, v) CONS4(sensor_read_reg, SENSOR_REGISTER_LEN, val, SENSOR_VALUE_LEN)(client, (reg), (v))
#define sensor_write_array generic_sensor_write_array

struct sensor_parameter {
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;

	unsigned short int preview_line_width;
	unsigned short int preview_gain;
	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};

struct specific_sensor {
	struct generic_sensor common_sensor;
	struct sensor_parameter parameter;
};

/*
*  The follow setting need been filled.
*
*  Must Filled:
*  sensor_init_data :               Sensor initial setting;
*  sensor_fullres_lowfps_data :     Sensor full resolution setting with best auality, recommand for video;
*  sensor_preview_data :            Sensor preview resolution setting, recommand it is vga or svga;
*  sensor_softreset_data :          Sensor software reset register;
*  sensor_check_id_data :           Sensir chip id register;
*
*  Optional filled:
*  sensor_fullres_highfps_data:     Sensor full resolution setting with high framerate, recommand for video;
*  sensor_720p:                     Sensor 720p setting, it is for video;
*  sensor_1080p:                    Sensor 1080p setting, it is for video;
*
*  :::::WARNING:::::
*  The SensorEnd which is the setting end flag must be filled int the last of each setting;
*/

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] = {
	SensorEnd
};
/* Senor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] = {
	SensorEnd
};
/* Senor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] = {
	SensorEnd
};
/* Preview resolution setting*/
static struct rk_sensor_reg sensor_preview_data[] = {
#if NTSC_FULL
	{0x00, 0x11},
	{0x01, 0x78},
	{0x02, 0xCF},
	{0x03, 0x0E},
	{0x04, 0x00},
	{0x05, 0x00},
	{0x06, 0x32},
	{0x07, 0xC0},
	{0x08, 0x00},
	{0x09, 0x24},
	{0x0A, 0x48},
	{0x0B, 0xC0},
	{0x0C, 0x53},
	{0x0D, 0x10},
	{0x0E, 0x00},
	{0x0F, 0x00},
	{0x10, 0x5e},
	{0x11, 0x40},
	{0x12, 0x44},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x13},
	{0x16, 0x4E},
	{0x17, 0xBC},
	{0x18, 0x15},
	{0x19, 0xF0},
	{0x1A, 0x07},
	{0x1B, 0x00},
	{0x1C, 0x09},
	{0x1D, 0x38},
	{0x1E, 0x80},
	{0x1F, 0x80},
	{0x20, 0xA0},
	{0x21, 0x86},
	{0x22, 0x38},
	{0x23, 0x3C},
	{0x24, 0x56},
	{0x25, 0xFF},
	{0x26, 0x12},
	{0x27, 0x2D},
	{0x28, 0x00},
	{0x29, 0x48},
	{0x2A, 0x30},
	{0x2B, 0x70},
	{0x2C, 0x0A},
	{0x2D, 0x68},
	{0x2E, 0x5E},
	{0x2F, 0x00},
	{0x30, 0x62},
	{0x31, 0xB8},
	{0x32, 0x96},
	{0x33, 0xC0},
	{0x34, 0x00},
	{0x35, 0x65},
	{0x36, 0xDC},
	{0x37, 0x00},
	{0x38, 0x40},
	{0x39, 0x84},
	{0x3A, 0x00},
	{0x3B, 0x03},
	{0x3C, 0x00},
	{0x3D, 0x60},
	{0x3E, 0x00},
	{0x3F, 0x00},
	{0x40, 0x00},
	{0x41, 0x01},
	{0x42, 0x00},
	{0x43, 0x12},
	{0x44, 0x07},
	{0x45, 0x49},
	{0x46, 0x00},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x00},
	{0x4A, 0x00},
	{0x4B, 0x00},
	{0x4C, 0x03},
	{0x4D, 0x03},
	{0x4E, 0x37},
	{0x4F, 0x00},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x00},
	{0x54, 0x00},
	{0x55, 0x00},
	{0x56, 0x00},
	{0x57, 0x00},
	{0x58, 0x00},
	{0x59, 0x00},
	{0x5A, 0x00},
	{0x5B, 0x00},
	{0x5C, 0x00},
	{0x5D, 0x00},
	{0x5E, 0x00},
	{0x5F, 0x00},
	{0x60, 0x00},
	{0x61, 0x00},
	{0x62, 0x00},
	{0x63, 0x00},
	{0x64, 0x00},
	{0x65, 0x00},
	{0x66, 0x00},
	{0x67, 0x00},
	{0x68, 0x00},
	{0x69, 0x00},
	{0x6A, 0x00},
	{0x6B, 0x00},
	{0x6C, 0x00},
	{0x6D, 0x00},
	{0x6E, 0x00},
	{0x6F, 0x00},
	{0x70, 0x00},
	{0x71, 0x00},
	{0x72, 0x00},
	{0x73, 0x00},
	{0x74, 0x00},
	{0x75, 0x00},
	{0x76, 0x00},
	{0x77, 0x00},
	{0x78, 0x00},
	{0x79, 0x00},
	{0x7A, 0x00},
	{0x7B, 0x00},
	{0x7C, 0x00},
	{0x7D, 0x00},
	{0x7E, 0x01},
	{0x7F, 0x00},
	{0x80, 0x00},
	{0x81, 0x00},
	{0x82, 0x00},
	{0x83, 0x00},
	{0x84, 0x00},
	{0x85, 0x00},
	{0x86, 0x00},
	{0x87, 0x00},
	{0x88, 0x00},
	{0x89, 0x00},
	{0x8A, 0x00},
	{0x8B, 0xFE},
	{0x8C, 0x00},
	{0x8D, 0x00},
	{0x8E, 0x00},
	{0x8F, 0x73},
	{0x90, 0xDC},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x00},
	{0x96, 0x00},
	{0x97, 0x00},
	{0x98, 0x00},
	{0x99, 0x00},
	{0x9A, 0x00},
	{0x9B, 0x00},
	{0x9C, 0x00},
	{0x9D, 0x00},
	{0x9E, 0x00},
	{0x9F, 0x00},
	{0xA0, 0x00},
	{0xA1, 0x00},
	{0xA2, 0x00},
	{0xA3, 0x00},
	{0xA4, 0x00},
	{0xA5, 0x00},
	{0xA6, 0x00},
	{0xA7, 0x00},
	{0xA8, 0x00},
	{0xA9, 0x00},
	{0xAA, 0x00},
	{0xAB, 0x00},
	{0xAC, 0x00},
	{0xAD, 0x00},
	{0xAE, 0x00},
	{0xAF, 0x00},
	{0xB0, 0x00},
	{0xB1, 0x00},
	{0xB2, 0x00},
	{0xB3, 0xFA},
	{0xB4, 0x00},
	{0xB5, 0x00},
	{0xB6, 0x00},
	{0xB7, 0x00},
	{0xB8, 0x00},
	{0xB9, 0x01},
	{0xBA, 0x00},
	{0xBB, 0x00},
	{0xBC, 0x03},
	{0xBD, 0x00},
	{0xBE, 0x00},
	{0xBF, 0x00},
	{0xC0, 0x00},
	{0xC1, 0x00},
	{0xC2, 0x0B},
	{0xC3, 0x0C},
	{0xC4, 0x00},
	{0xC5, 0x00},
	{0xC6, 0x1F},
	{0xC7, 0x78},
	{0xC8, 0x21},
	{0xC9, 0x00},
	{0xCA, 0x00},
	{0xCB, 0x07},
	{0xCC, 0x08},
	{0xCD, 0x00},
	{0xCE, 0x00},
	{0xCF, 0x04},
	{0xD0, 0x00},
	{0xD1, 0x00},
	{0xD2, 0x60},
	{0xD3, 0x10},
	{0xD4, 0x06},
	{0xD5, 0xBE},
	{0xD6, 0x39},
	{0xD7, 0x27},
	{0xD8, 0x00},
	{0xD9, 0x00},
	{0xDA, 0x00},
	{0xDB, 0x00},
	{0xDC, 0x00},
	{0xDD, 0x00},
	{0xDE, 0x00},
	{0xDF, 0x00},
	{0xE0, 0x00},
	{0xE1, 0x00},
	{0xE2, 0x00},
	{0xE3, 0x00},
	{0xE4, 0x00},
	{0xE5, 0x00},
	{0xE6, 0x00},
	{0xE7, 0x00},
	{0xE8, 0x00},
	{0xE9, 0x00},
	{0xEA, 0x00},
	{0xEB, 0x00},
	{0xEC, 0x00},
	{0xED, 0x00},
	{0xEE, 0x00},
	{0xEF, 0x00},
	{0xF0, 0x00},
	{0xF1, 0x00},
	{0xF2, 0x00},
	{0xF3, 0x00},
	{0xF4, 0x00},
	{0xF5, 0x00},
	{0xF6, 0x00},
	{0xF7, 0x00},
	{0xF8, 0x00},
	{0xF9, 0x00},
	{0xFA, 0x00},
	{0xFB, 0x00},
	{0xFC, 0xC0},
	{0xFD, 0x00},
	{0xFE, 0x28},
	{0xFF, 0xFF},
#endif

#if YUV_720P_50HZ
	/*720p 50hz full*/
	{0x00, 0x11},
	{0x01, 0x7E},
	{0x02, 0xCA},/*0xCA*/
	{0x03, 0x01},
	{0x04, 0x00},
	{0x05, 0x00},
	{0x06, 0x32},
	{0x07, 0xC0},
	{0x08, 0x00},
	{0x09, 0x24},
	{0x0A, 0x48},
	{0x0B, 0xC0},
	{0x0C, 0x43},
	{0x0D, 0x10},
	{0x0E, 0x00},
	{0x0F, 0x00},
	{0x10, 0x00},
	{0x11, 0x40},
	{0x12, 0x40},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x13},/*0x13*/
	{0x16, 0x16},/*0x16*/
	{0x17, 0x00},
	{0x18, 0x18},
	{0x19, 0xD0},
	{0x1A, 0x25},
	{0x1B, 0x00},
	{0x1C, 0x07},
	{0x1D, 0xBC},
	{0x1E, 0x80},
	{0x1F, 0x80},
	{0x20, 0x60},
	{0x21, 0x86},
	{0x22, 0x38},
	{0x23, 0x3C},
	{0x24, 0x56},
	{0x25, 0xFF},
	{0x26, 0x02},
	{0x27, 0x2D},
	{0x28, 0x00},
	{0x29, 0x48},
	{0x2A, 0x30},/*0x30*/
	{0x2B, 0x4A},
	{0x2C, 0x0A},
	{0x2D, 0x30},
	{0x2E, 0x70},
	{0x2F, 0x00},
	{0x30, 0x48},
	{0x31, 0xBB},
	{0x32, 0x2E},
	{0x33, 0x90},
	{0x34, 0x00},
	{0x35, 0x05},
	{0x36, 0xDC},
	{0x37, 0x00},
	{0x38, 0x40},
	{0x39, 0x8C},
	{0x3A, 0x00},
	{0x3B, 0x03},
	{0x3C, 0x00},
	{0x3D, 0x60},
	{0x3E, 0x00},
	{0x3F, 0x00},
	{0x40, 0x00},
	{0x41, 0x00},
	{0x42, 0x00},
	{0x43, 0x12},
	{0x44, 0x07},
	{0x45, 0x49},
	{0x46, 0x00},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x00},
	{0x4A, 0x00},
	{0x4B, 0x00},
	{0x4C, 0x03},
	{0x4D, 0x03},
	{0x4E, 0x03},/*0x03*/
	{0x4F, 0x00},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x00},
	{0x54, 0x00},
	{0x55, 0x00},
	{0x56, 0x00},
	{0x57, 0x00},
	{0x58, 0x00},
	{0x59, 0x00},
	{0x5A, 0x00},
	{0x5B, 0x00},
	{0x5C, 0x00},
	{0x5D, 0x00},
	{0x5E, 0x00},
	{0x5F, 0x00},
	{0x60, 0x00},
	{0x61, 0x00},
	{0x62, 0x00},
	{0x63, 0x00},
	{0x64, 0x00},
	{0x65, 0x00},
	{0x66, 0x00},
	{0x67, 0x00},
	{0x68, 0x00},
	{0x69, 0x00},
	{0x6A, 0x00},
	{0x6B, 0x00},
	{0x6C, 0x00},
	{0x6D, 0x00},
	{0x6E, 0x00},
	{0x6F, 0x00},
	{0x70, 0x00},
	{0x71, 0x00},
	{0x72, 0x00},
	{0x73, 0x00},
	{0x74, 0x00},
	{0x75, 0x00},
	{0x76, 0x00},
	{0x77, 0x00},
	{0x78, 0x00},
	{0x79, 0x00},
	{0x7A, 0x00},
	{0x7B, 0x00},
	{0x7C, 0x00},
	{0x7D, 0x00},
	{0x7E, 0x01},
	{0x7F, 0x00},
	{0x80, 0x00},
	{0x81, 0x00},
	{0x82, 0x00},
	{0x83, 0x00},
	{0x84, 0x00},
	{0x85, 0x00},
	{0x86, 0x00},
	{0x87, 0x00},
	{0x88, 0x00},
	{0x89, 0x00},
	{0x8A, 0x00},
	{0x8B, 0xFF},
	{0x8C, 0xFF},
	{0x8D, 0xFF},
	{0x8E, 0xFF},
	{0x8F, 0x80},
	{0x90, 0xFF},
	{0x91, 0xFF},
	{0x92, 0xFF},
	{0x93, 0xFF},
	{0x94, 0x80},
	{0x95, 0x00},
	{0x96, 0x00},
	{0x97, 0x00},
	{0x98, 0x00},
	{0x99, 0x00},
	{0x9A, 0x00},
	{0x9B, 0x00},
	{0x9C, 0x00},
	{0x9D, 0x00},
	{0x9E, 0x00},
	{0x9F, 0x00},
	{0xA0, 0x00},
	{0xA1, 0x00},
	{0xA2, 0x00},
	{0xA3, 0x00},
	{0xA4, 0x00},
	{0xA5, 0x00},
	{0xA6, 0x00},
	{0xA7, 0x00},
	{0xA8, 0x00},
	{0xA9, 0x00},
	{0xAA, 0x00},
	{0xAB, 0x00},
	{0xAC, 0x00},
	{0xAD, 0x00},
	{0xAE, 0x00},
	{0xAF, 0x00},
	{0xB0, 0x00},
	{0xB1, 0x00},
	{0xB2, 0x00},
	{0xB3, 0xFA},
	{0xB4, 0x00},
	{0xB5, 0x00},
	{0xB6, 0x00},
	{0xB7, 0x00},
	{0xB8, 0x00},
	{0xB9, 0x01},
	{0xBA, 0x00},
	{0xBB, 0x00},
	{0xBC, 0x03},
	{0xBD, 0x00},
	{0xBE, 0x00},
	{0xBF, 0x00},
	{0xC0, 0x00},
	{0xC1, 0x00},
	{0xC2, 0x0B},
	{0xC3, 0x0C},
	{0xC4, 0x00},
	{0xC5, 0x00},
	{0xC6, 0x1F},
	{0xC7, 0x78},
	{0xC8, 0x21},
	{0xC9, 0x00},
	{0xCA, 0x00},
	{0xCB, 0x07},
	{0xCC, 0x08},
	{0xCD, 0x00},
	{0xCE, 0x00},
	{0xCF, 0x04},
	{0xD0, 0x00},
	{0xD1, 0x00},
	{0xD2, 0x60},
	{0xD3, 0x10},
	{0xD4, 0x06},
	{0xD5, 0xBE},
	{0xD6, 0x39},
	{0xD7, 0x27},
	{0xD8, 0x00},
	{0xD9, 0x00},
	{0xDA, 0x00},
	{0xDB, 0x00},
	{0xDC, 0x00},
	{0xDD, 0x00},
	{0xDE, 0x00},
	{0xDF, 0x00},
	{0xE0, 0x00},
	{0xE1, 0x00},
	{0xE2, 0x00},
	{0xE3, 0x00},
	{0xE4, 0x00},
	{0xE5, 0x00},
	{0xE6, 0x00},
	{0xE7, 0x00},
	{0xE8, 0x00},
	{0xE9, 0x00},
	{0xEA, 0x00},
	{0xEB, 0x00},
	{0xEC, 0x00},
	{0xED, 0x00},
	{0xEE, 0x00},
	{0xEF, 0x00},
	{0xF0, 0x00},
	{0xF1, 0x00},
	{0xF2, 0x00},
	{0xF3, 0x00},
	{0xF4, 0x00},
	{0xF5, 0x00},
	{0xF6, 0x00},
	{0xF7, 0x00},
	{0xF8, 0x00},
	{0xF9, 0x00},
	{0xFA, 0x00},
	{0xFB, 0x00},
	{0xFC, 0xC0},
	{0xFD, 0x00},
	{0xFE, 0x28},
#endif

#if YUV_720P_30HZ
	{0x00, 0x11},
	{0x01, 0x7E},
	{0x02, 0xDA},
	{0x03, 0x2C},
	{0x04, 0x04},
	{0x05, 0x00},
	{0x06, 0x32},
	{0x07, 0xC0},
	{0x08, 0x00},
	{0x09, 0x24},
	{0x0A, 0x48},
	{0x0B, 0xC0},
	{0x0C, 0x53},
	{0x0D, 0x10},
	{0x0E, 0x00},
	{0x0F, 0x00},
	{0x10, 0x00},
	{0x11, 0x40},
	{0x12, 0x40},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x13},
	{0x16, 0x16}, /*0x16*/
	{0x17, 0x00},
	{0x18, 0x19},
	{0x19, 0xD0},
	{0x1A, 0x25},
	{0x1B, 0x00},
	{0x1C, 0x06},
	{0x1D, 0x72},
	{0x1E, 0x80},
	{0x1F, 0x80},
	{0x20, 0x60},
	{0x21, 0x86},
	{0x22, 0x38},
	{0x23, 0x3C},
	{0x24, 0x56},
	{0x25, 0xFF},
	{0x26, 0x02},
	{0x27, 0x2D},
	{0x28, 0x00},
	{0x29, 0x48},
	{0x2A, 0x30},/*0x30*/
	{0x2B, 0x4A},
	{0x2C, 0x0A},
	{0x2D, 0x30},
	{0x2E, 0x70},
	{0x2F, 0x00},
	{0x30, 0x48},
	{0x31, 0xBB},
	{0x32, 0x2E},
	{0x33, 0x90},
	{0x34, 0x00},
	{0x35, 0x25},
	{0x36, 0xDC},
	{0x37, 0x00},
	{0x38, 0x40},
	{0x39, 0x88},
	{0x3A, 0x00},
	{0x3B, 0x03},
	{0x3C, 0x00},
	{0x3D, 0x60},
	{0x3E, 0x00},
	{0x3F, 0x00},
	{0x40, 0x03},
	{0x41, 0x00},
	{0x42, 0x00},
	{0x43, 0x12},
	{0x44, 0x07},
	{0x45, 0x49},
	{0x46, 0x00},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x00},
	{0x4A, 0x00},
	{0x4B, 0x00},
	{0x4C, 0x03},/*0x03*/
	{0x4D, 0x03},
	{0x4E, 0x17},
	{0x4F, 0x00},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x00},
	{0x54, 0x00},
	{0x55, 0x00},
	{0x56, 0x00},
	{0x57, 0x00},
	{0x58, 0x00},
	{0x59, 0x00},
	{0x5A, 0x00},
	{0x5B, 0x00},
	{0x5C, 0x00},
	{0x5D, 0x00},
	{0x5E, 0x00},
	{0x5F, 0x00},
	{0x60, 0x00},
	{0x61, 0x00},
	{0x62, 0x00},
	{0x63, 0x00},
	{0x64, 0x00},
	{0x65, 0x00},
	{0x66, 0x00},
	{0x67, 0x00},
	{0x68, 0x00},
	{0x69, 0x00},
	{0x6A, 0x00},
	{0x6B, 0x00},
	{0x6C, 0x00},
	{0x6D, 0x00},
	{0x6E, 0x00},
	{0x6F, 0x00},
	{0x70, 0x00},
	{0x71, 0x00},
	{0x72, 0x00},
	{0x73, 0x00},
	{0x74, 0x00},
	{0x75, 0x00},
	{0x76, 0x00},
	{0x77, 0x00},
	{0x78, 0x00},
	{0x79, 0x00},
	{0x7A, 0x00},
	{0x7B, 0x00},
	{0x7C, 0x00},
	{0x7D, 0x00},
	{0x7E, 0x01},
	{0x7F, 0x00},
	{0x80, 0x00},
	{0x81, 0x00},
	{0x82, 0x00},
	{0x83, 0x00},
	{0x84, 0x00},
	{0x85, 0x00},
	{0x86, 0x00},
	{0x87, 0x00},
	{0x88, 0x00},
	{0x89, 0x00},
	{0x8A, 0x00},
	{0x8B, 0xFF},
	{0x8C, 0xFF},
	{0x8D, 0xFF},
	{0x8E, 0x00},
	{0x8F, 0x00},
	{0x90, 0xFF},
	{0x91, 0xFF},
	{0x92, 0xFF},
	{0x93, 0xFF},
	{0x94, 0x80},
	{0x95, 0x00},
	{0x96, 0x00},
	{0x97, 0x00},
	{0x98, 0x00},
	{0x99, 0x00},
	{0x9A, 0x00},
	{0x9B, 0x00},
	{0x9C, 0x00},
	{0x9D, 0x00},
	{0x9E, 0x00},
	{0x9F, 0x00},
	{0xA0, 0x00},
	{0xA1, 0x00},
	{0xA2, 0x00},
	{0xA3, 0x00},
	{0xA4, 0x00},
	{0xA5, 0x00},
	{0xA6, 0x00},
	{0xA7, 0x00},
	{0xA8, 0x00},
	{0xA9, 0x00},
	{0xAA, 0x00},
	{0xAB, 0x00},
	{0xAC, 0x00},
	{0xAD, 0x00},
	{0xAE, 0x00},
	{0xAF, 0x00},
	{0xB0, 0x00},
	{0xB1, 0x00},
	{0xB2, 0x00},
	{0xB3, 0xFA},
	{0xB4, 0x00},
	{0xB5, 0x00},
	{0xB6, 0x00},
	{0xB7, 0x00},
	{0xB8, 0x00},
	{0xB9, 0x01},
	{0xBA, 0x00},
	{0xBB, 0x00},
	{0xBC, 0x03},
	{0xBD, 0x00},
	{0xBE, 0x00},
	{0xBF, 0x00},
	{0xC0, 0x00},
	{0xC1, 0x00},
	{0xC2, 0x0B},
	{0xC3, 0x0C},
	{0xC4, 0x00},
	{0xC5, 0x00},
	{0xC6, 0x1F},
	{0xC7, 0x78},
	{0xC8, 0x21},
	{0xC9, 0x00},
	{0xCA, 0x00},
	{0xCB, 0x07},
	{0xCC, 0x08},
	{0xCD, 0x00},
	{0xCE, 0x00},
	{0xCF, 0x04},
	{0xD0, 0x00},
	{0xD1, 0x00},
	{0xD2, 0x60},
	{0xD3, 0x10},
	{0xD4, 0x06},
	{0xD5, 0xBE},
	{0xD6, 0x39},
	{0xD7, 0x27},
	{0xD8, 0x00},
	{0xD9, 0x00},
	{0xDA, 0x00},
	{0xDB, 0x00},
	{0xDC, 0x00},
	{0xDD, 0x00},
	{0xDE, 0x00},
	{0xDF, 0x00},
	{0xE0, 0x00},
	{0xE1, 0x00},
	{0xE2, 0x00},
	{0xE3, 0x00},
	{0xE4, 0x00},
	{0xE5, 0x00},
	{0xE6, 0x00},
	{0xE7, 0x00},
	{0xE8, 0x00},
	{0xE9, 0x00},
	{0xEA, 0x00},
	{0xEB, 0x00},
	{0xEC, 0x00},
	{0xED, 0x00},
	{0xEE, 0x00},
	{0xEF, 0x00},
	{0xF0, 0x00},
	{0xF1, 0x00},
	{0xF2, 0x00},
	{0xF3, 0x00},
	{0xF4, 0x00},
	{0xF5, 0x00},
	{0xF6, 0x00},
	{0xF7, 0x00},
	{0xF8, 0x00},
	{0xF9, 0x00},
	{0xFA, 0x00},
	{0xFB, 0x00},
	{0xFC, 0xC0},
	{0xFD, 0x00},
	{0xFE, 0x28},
	{0xFF, 0x25},
#endif

#if YUV_720P_25HZ
	/*720p 25 full*/
	{0x00, 0x11},
	{0x01, 0x7E},
	{0x02, 0xCA},
	{0x03, 0x0D},
	{0x04, 0x00},
	{0x05, 0x00},
	{0x06, 0x32},
	{0x07, 0xC0},
	{0x08, 0x00},
	{0x09, 0x24},
	{0x0A, 0x48},
	{0x0B, 0xC0},
	{0x0C, 0x53},
	{0x0D, 0x10},
	{0x0E, 0x00},
	{0x0F, 0x00},
	{0x10, 0x00},
	{0x11, 0x40},
	{0x12, 0x40},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x13},/*0x13*//**/
	{0x16, 0x16},/*0x16*/
	{0x17, 0x00},
	{0x18, 0x19},
	{0x19, 0xD0},/*0xd0*/
	{0x1A, 0x25},/*0x25*/
	{0x1B, 0x00},
	{0x1C, 0x07},/*0x07*/
	{0x1D, 0xBC},/*0xbc*/
	{0x1E, 0x80},
	{0x1F, 0x80},
	{0x20, 0x60},
	{0x21, 0x86},
	{0x22, 0x38},
	{0x23, 0x3C},
	{0x24, 0x56},
	{0x25, 0xFF},
	{0x26, 0x02},
	{0x27, 0x2D},
	{0x28, 0x00},
	{0x29, 0x48},
	{0x2A, 0x30},/*0x30*/
	{0x2B, 0x70},
	{0x2C, 0x0A},
	{0x2D, 0x30},
	{0x2E, 0x70},
	{0x2F, 0x00},
	{0x30, 0x48},
	{0x31, 0xBB},
	{0x32, 0x2E},
	{0x33, 0x90},
	{0x34, 0x00},
	{0x35, 0x25}, /*0x25*/
	{0x36, 0xDC},
	{0x37, 0x00},
	{0x38, 0x40},
	{0x39, 0x88},
	{0x3A, 0x00},
	{0x3B, 0x03},
	{0x3C, 0x00},
	{0x3D, 0x60},
	{0x3E, 0x00},
	{0x3F, 0x00},
	{0x40, 0x00},
	{0x41, 0x00},
	{0x42, 0x00},
	{0x43, 0x12},
	{0x44, 0x07},
	{0x45, 0x49},
	{0x46, 0x00},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x00},
	{0x4A, 0x00},
	{0x4B, 0x00},
	{0x4C, 0x03}, /*0x00*/
	{0x4D, 0x03},
	{0x4E, 0x17}, /*0x17*/
	{0x4F, 0x00},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x00},
	{0x54, 0x00},
	{0x55, 0x00},
	{0x56, 0x00},
	{0x57, 0x00},
	{0x58, 0x00},
	{0x59, 0x00},
	{0x5A, 0x00},
	{0x5B, 0x00},
	{0x5C, 0x00},
	{0x5D, 0x00},
	{0x5E, 0x00},
	{0x5F, 0x00},
	{0x60, 0x00},
	{0x61, 0x00},
	{0x62, 0x00},
	{0x63, 0x00},
	{0x64, 0x00},
	{0x65, 0x00},
	{0x66, 0x00},
	{0x67, 0x00},
	{0x68, 0x00},
	{0x69, 0x00},
	{0x6A, 0x00},
	{0x6B, 0x00},
	{0x6C, 0x00},
	{0x6D, 0x00},
	{0x6E, 0x00},
	{0x6F, 0x00},
	{0x70, 0x00},
	{0x71, 0x00},
	{0x72, 0x00},
	{0x73, 0x00},
	{0x74, 0x00},
	{0x75, 0x00},
	{0x76, 0x00},
	{0x77, 0x00},
	{0x78, 0x00},
	{0x79, 0x00},
	{0x7A, 0x00},
	{0x7B, 0x00},
	{0x7C, 0x00},
	{0x7D, 0x00},
	{0x7E, 0x01},
	{0x7F, 0x00},
	{0x80, 0x00},
	{0x81, 0x00},
	{0x82, 0x00},
	{0x83, 0x00},
	{0x84, 0x00},
	{0x85, 0x00},
	{0x86, 0x00},
	{0x87, 0x00},
	{0x88, 0x00},
	{0x89, 0x00},
	{0x8A, 0x00},
	{0x8B, 0xFF},
	{0x8C, 0xFF},
	{0x8D, 0xF0}, /*f0*/
	{0x8E, 0x1F}, /*1f*/
	{0x8F, 0xFF},
	{0x90, 0xFF},
	{0x91, 0xFF},
	{0x92, 0xF0}, /*f0*/
	{0x93, 0x1F}, /*1f*/
	{0x94, 0xFF},
	{0x95, 0x00},
	{0x96, 0x00},
	{0x97, 0x00},
	{0x98, 0x00},
	{0x99, 0x00},
	{0x9A, 0x00},
	{0x9B, 0x00},
	{0x9C, 0x00},
	{0x9D, 0x00},
	{0x9E, 0x00},
	{0x9F, 0x00},
	{0xA0, 0x00},
	{0xA1, 0x00},
	{0xA2, 0x00},
	{0xA3, 0x00},
	{0xA4, 0x00},
	{0xA5, 0x00},
	{0xA6, 0x00},
	{0xA7, 0x00},
	{0xA8, 0x00},
	{0xA9, 0x00},
	{0xAA, 0x00},
	{0xAB, 0x00},
	{0xAC, 0x00},
	{0xAD, 0x00},
	{0xAE, 0x00},
	{0xAF, 0x00},
	{0xB0, 0x00},
	{0xB1, 0x00},
	{0xB2, 0x00},
	{0xB3, 0xFA},
	{0xB4, 0x00},
	{0xB5, 0x00},
	{0xB6, 0x00},
	{0xB7, 0x00},
	{0xB8, 0x00},
	{0xB9, 0x01},
	{0xBA, 0x00},
	{0xBB, 0x00},
	{0xBC, 0x03},
	{0xBD, 0x00},
	{0xBE, 0x00},
	{0xBF, 0x00},
	{0xC0, 0x00},
	{0xC1, 0x00},
	{0xC2, 0x0B},
	{0xC3, 0x0C},
	{0xC4, 0x00},
	{0xC5, 0x00},
	{0xC6, 0x1F},
	{0xC7, 0x78},
	{0xC8, 0x21},
	{0xC9, 0x00},
	{0xCA, 0x00},
	{0xCB, 0x07},
	{0xCC, 0x08},
	{0xCD, 0x00},
	{0xCE, 0x00},
	{0xCF, 0x04},
	{0xD0, 0x00},
	{0xD1, 0x00},
	{0xD2, 0x60},
	{0xD3, 0x10},
	{0xD4, 0x06},
	{0xD5, 0xBE},
	{0xD6, 0x39},
	{0xD7, 0x27},
	{0xD8, 0x00},
	{0xD9, 0x00},
	{0xDA, 0x00},
	{0xDB, 0x00},
	{0xDC, 0x00},
	{0xDD, 0x00},
	{0xDE, 0x00},
	{0xDF, 0x00},
	{0xE0, 0x00},
	{0xE1, 0x00},
	{0xE2, 0x00},
	{0xE3, 0x00},
	{0xE4, 0x00},
	{0xE5, 0x00},
	{0xE6, 0x00},
	{0xE7, 0x00},
	{0xE8, 0x00},
	{0xE9, 0x00},
	{0xEA, 0x00},
	{0xEB, 0x00},
	{0xEC, 0x00},
	{0xED, 0x00},
	{0xEE, 0x00},
	{0xEF, 0x00},
	{0xF0, 0x00},
	{0xF1, 0x00},
	{0xF2, 0x00},
	{0xF3, 0x00},
	{0xF4, 0x00},
	{0xF5, 0x00},
	{0xF6, 0x00},
	{0xF7, 0x00},
	{0xF8, 0x00},
	{0xF9, 0x00},
	{0xFA, 0x00},
	{0xFB, 0x00},
	{0xFC, 0xC0},
	{0xFD, 0x00},
	{0xFE, 0x28},
	{0xFF, 0x25},
#endif

	SensorEnd
};
/* 1280x720 */
static struct rk_sensor_reg sensor_720p[] = {
	SensorEnd
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_softreset_data[] = {
	SensorRegVal(0x06, 0x32 | 0x80),
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[] = {
	SensorRegVal(0xfe, 0x0),
	SensorRegVal(0xff, 0x0),
	SensorEnd
};
/*
*  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
*/
static struct rk_sensor_reg sensor_WhiteB_Auto[] = {
	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[] = {
	SensorEnd
};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[] = {
	SensorEnd
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[] = {
	SensorEnd
};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[] = {
	SensorEnd
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {
	sensor_WhiteB_Auto,
	sensor_WhiteB_TungstenLamp1,
	sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay,
	sensor_WhiteB_Cloudy,
	NULL,
};

static struct rk_sensor_reg sensor_Brightness0[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness1[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness2[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness3[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness4[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness5[] = {
	SensorEnd
};

static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {
	sensor_Brightness0,
	sensor_Brightness1,
	sensor_Brightness2,
	sensor_Brightness3,
	sensor_Brightness4,
	sensor_Brightness5,
	NULL,
};

static struct rk_sensor_reg sensor_Effect_Normal[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Effect_WandB[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Effect_Sepia[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Effect_Negative[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Effect_Bluish[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Effect_Green[] = {
	SensorEnd
};

static struct rk_sensor_reg *sensor_EffectSeqe[] = {
	sensor_Effect_Normal,
	sensor_Effect_WandB,
	sensor_Effect_Negative,
	sensor_Effect_Sepia,
	sensor_Effect_Bluish,
	sensor_Effect_Green,
	NULL,
};

static struct rk_sensor_reg sensor_Exposure0[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure1[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure2[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure3[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure4[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure5[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure6[] = {
	SensorEnd
};

static struct rk_sensor_reg *sensor_ExposureSeqe[] = {
	sensor_Exposure0,
	sensor_Exposure1,
	sensor_Exposure2,
	sensor_Exposure3,
	sensor_Exposure4,
	sensor_Exposure5,
	sensor_Exposure6,
	NULL,
};

static struct rk_sensor_reg sensor_Saturation0[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Saturation1[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Saturation2[] = {
	SensorEnd
};
static struct rk_sensor_reg *sensor_SaturationSeqe[] = {
	sensor_Saturation0,
	sensor_Saturation1,
	sensor_Saturation2,
	NULL,
};

static struct rk_sensor_reg sensor_Contrast0[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast1[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast2[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast3[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast4[] = {
	SensorEnd
};


static struct rk_sensor_reg sensor_Contrast5[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast6[] = {
	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {
	sensor_Contrast0,
	sensor_Contrast1,
	sensor_Contrast2,
	sensor_Contrast3,
	sensor_Contrast4,
	sensor_Contrast5,
	sensor_Contrast6,
	NULL,
};

static struct rk_sensor_reg sensor_SceneAuto[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_SceneNight[] = {
	SensorEnd
};
static struct rk_sensor_reg *sensor_SceneSeqe[] = {
	sensor_SceneAuto,
	sensor_SceneNight,
	NULL,
};

static struct rk_sensor_reg sensor_Zoom0[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom1[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom2[] = {
	SensorEnd
};


static struct rk_sensor_reg sensor_Zoom3[] = {
	SensorEnd
};

static struct rk_sensor_reg *sensor_ZoomSeqe[] = {
	sensor_Zoom0,
	sensor_Zoom1,
	sensor_Zoom2,
	sensor_Zoom3,
	NULL,
};

/*
* User could be add v4l2_querymenu in sensor_controls by new_usr_v4l2menu
*/
static struct v4l2_querymenu sensor_menus[] = {

};

/*
* User could be add v4l2_queryctrl in sensor_controls by new_user_v4l2ctrl
*/
static struct sensor_v4l2ctrl_usr_s sensor_controls[] = {

};

/*
* MUST define the current used format as the first item
*/
static struct rk_sensor_datafmt sensor_colour_fmts[] = {
	{V4L2_MBUS_FMT_UYVY8_2X8, V4L2_COLORSPACE_JPEG},
	{V4L2_MBUS_FMT_YUYV8_2X8, V4L2_COLORSPACE_JPEG}
};

/*
**********************************************************
* Following is local code:
*
* Please codeing your program here
**********************************************************
*/
static int sensor_parameter_record(struct i2c_client *client)
{
	return 0;
}

static int sensor_ae_transfer(struct i2c_client *client)
{
	return 0;
}
/*
**********************************************************
* Following is callback
* If necessary, you could coding these callback
**********************************************************
*/
/*
* the function is called in open sensor
*/
static int sensor_activate_cb(struct i2c_client *client)
{
	return 0;
}
/*
* the function is called in close sensor
*/
static int sensor_deactivate_cb(struct i2c_client *client)
{
	return 0;
}
/*
* the function is called before sensor register setting in VIDIOC_S_FMT
*/
static int sensor_s_fmt_cb_th(struct i2c_client *client, struct v4l2_mbus_framefmt *mf, bool capture)
{
	if (capture)
		sensor_parameter_record(client);

	return 0;
}
/*
* the function is called after sensor register setting finished in VIDIOC_S_FMT
*/
static int sensor_s_fmt_cb_bh(struct i2c_client *client, struct v4l2_mbus_framefmt *mf, bool capture)
{
	if (capture)
		sensor_ae_transfer(client);

	return 0;
}
static int sensor_try_fmt_cb_th(struct i2c_client *client, struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_softrest_usr_cb(struct i2c_client *client, struct rk_sensor_reg *series)
{
	return 0;
}
static int sensor_check_id_usr_cb(struct i2c_client *client, struct rk_sensor_reg *series)
{
	return 0;
}

static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{
	SENSOR_DG("Resume");

	return 0;
}
static int sensor_mirror_cb(struct i2c_client *client, int mirror)
{
	return 0;
}
/*
* the function is v4l2 control V4L2_CID_HFLIP callback
*/
static int sensor_v4l2ctrl_mirror_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info,
				     struct v4l2_ext_control *ext_ctrl)
{
	SENSOR_DG("sensor_mirror success, value:0x%x", ext_ctrl->value);
	return 0;
}

static int sensor_flip_cb(struct i2c_client *client, int flip)
{
	return 0;
}
/*
* the function is v4l2 control V4L2_CID_VFLIP callback
*/
static int sensor_v4l2ctrl_flip_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info,
				   struct v4l2_ext_control *ext_ctrl)
{
	SENSOR_DG("sensor_flip success, value:0x%x", ext_ctrl->value);
	return 0;
}
/*
* the functions are focus callbacks
*/
static int sensor_focus_init_usr_cb(struct i2c_client *client)
{
	return 0;
}

static int sensor_focus_af_single_usr_cb(struct i2c_client *client)
{
	return 0;
}

static int sensor_focus_af_near_usr_cb(struct i2c_client *client)
{
	return 0;
}

static int sensor_focus_af_far_usr_cb(struct i2c_client *client)
{
	return 0;
}

static int sensor_focus_af_specialpos_usr_cb(struct i2c_client *client, int pos)
{
	return 0;
}

static int sensor_focus_af_const_usr_cb(struct i2c_client *client)
{
	return 0;
}
static int sensor_focus_af_const_pause_usr_cb(struct i2c_client *client)
{
	return 0;
}
static int sensor_focus_af_close_usr_cb(struct i2c_client *client)
{
	return 0;
}

static int sensor_focus_af_zoneupdate_usr_cb(struct i2c_client *client, int *zone_tm_pos)
{
	return 0;
}

/*
face defect call back
*/
static int 	sensor_face_detect_usr_cb(struct i2c_client *client, int on)
{
	return 0;
}

/*
*   The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
* initialization in the function.
*/
static void sensor_init_parameters_user(struct specific_sensor *spsensor, struct soc_camera_device *icd)
{
	memcpy(&spsensor->common_sensor.info_priv.dev_sig_cnf, &dev_info, sizeof(dev_info));
}

/*
* :::::WARNING:::::
* It is not allowed to modify the following code
*/

sensor_init_parameters_default_code();

sensor_v4l2_struct_initialization();

sensor_probe_default_code();

sensor_remove_default_code();

sensor_driver_default_module_code();
