/*
 * arch/arm/mach-omap2/board-44xx-tablet-panel.c
 *
 * Copyright (C) 2011 Texas Instruments
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/o2micro_bl.h>
#include <linux/platform_device.h>
#include <linux/omapfb.h>
#include <video/omapdss.h>
#include <linux/leds_pwm.h>
#include <linux/leds.h>
#include <linux/delay.h>
#include <linux/i2c/twl.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>

#include <plat/vram.h>
#include <plat/omap_apps_brd_id.h>
#include <plat/dmtimer.h>
#include <plat/omap_device.h>
#include <plat/android-display.h>
#include <plat/sgx_omaplfb.h>

#include "board-4430kc1-tablet.h"
#include "control.h"
#include "mux.h"
#include "mux44xx.h"
//#include "dmtimer.h"

#define OTTER_FB_RAM_SIZE                SZ_16M + SZ_4M /* 1920×1080*4 * 2 */

#define DEFAULT_BACKLIGHT_BRIGHTNESS	105

static struct omap_dss_device tablet_lcd_device = {
	.clocks		= {
		.dispc	= {
			.channel	= {
				.lck_div        = 1,
				.pck_div        = 4,
				.lcd_clk_src    = OMAP_DSS_CLK_SRC_DSI2_PLL_HSDIV_DISPC,
			},
			.dispc_fclk_src = OMAP_DSS_CLK_SRC_DSI2_PLL_HSDIV_DISPC,
		},
	},
        .panel          = {
		.config		= OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				  OMAP_DSS_LCD_IHS,
		.timings	= {
			.x_res          = 1024,
			.y_res          = 600,
			.pixel_clock    = 51200, /* in kHz */
			.hfp            = 160,   /* HFP fix 160 */
			.hsw            = 10,    /* HSW = 1~140 */
			.hbp            = 150,   /* HSW + HBP = 160 */
			.vfp            = 12,    /* VFP fix 12 */
			.vsw            = 3,     /* VSW = 1~20 */
			.vbp            = 20,    /* VSW + VBP = 23 */
		},
        	.width_in_um = 158000,
        	.height_in_um = 92000,
        },
	.ctrl = {
		.pixel_size = 24,
	},
	.name			= "lcd2",
	.driver_name		= "otter1_panel_drv",
	.type			= OMAP_DISPLAY_TYPE_DPI,
	.phy.dpi.data_lines	= 24,
	.channel		= OMAP_DSS_CHANNEL_LCD2,
};

static bool enable_suspend_off = true;
module_param(enable_suspend_off, bool, S_IRUSR | S_IRGRP | S_IROTH);

static struct omap_dss_device *sdp4430_dss_devices[] = {
	&tablet_lcd_device,
};

static struct omap_dss_board_info sdp4430_dss_data = {
	.num_devices	=	ARRAY_SIZE(sdp4430_dss_devices),
	.devices	=	sdp4430_dss_devices,
	.default_device	=	&tablet_lcd_device,
};

static struct spi_board_info tablet_spi_board_info[] __initdata = {
	{
		.modalias		= "otter1_disp_spi",
		.bus_num		= 4,     /* McSPI4 */
		.chip_select		= 0,
		.max_speed_hz		= 375000,
	},
};

static struct o2micro_backlight_platform_data kc1_led_data = {
	.name         = "lcd-backlight",
	.gpio_en_o2m  = -1,
	.gpio_vol_o2m = -1,
	.gpio_en_lcd  = 47,
	.gpio_cabc_en = -1,
	.timer        = 10,        /* use GPTimer 10 for backlight */
	.sysclk       = 38400000,  /* input frequency to the timer, 38.4M */
	.pwmfreq      = 75000,     /* output frequency from timer (10k on bowser) */
	.totalsteps   = 256,       /* how many backlight steps for the user, also the max brightness */
	.initialstep  = DEFAULT_BACKLIGHT_BRIGHTNESS
};

static struct platform_device kc1_led_device = {
	.name   = "o2micro-bl",
	.id     = -1,
	.dev	= {
		.platform_data = &kc1_led_data,
	},
};


static struct platform_device __initdata *sdp4430_panel_devices[] = {
	&kc1_led_device,
};

static struct dsscomp_platform_data dsscomp_config_tablet = {
	.tiler1d_slotsz = (SZ_16M + SZ_4M),
};

static struct sgx_omaplfb_config omaplfb_config_tablet[] = {
	{
		.tiler2d_buffers = 2,
		.swap_chain_length = 2,
	},
	{
		.vram_buffers = 2,
		.swap_chain_length = 2,
	},
};

static struct sgx_omaplfb_platform_data omaplfb_plat_data_tablet = {
	.num_configs = ARRAY_SIZE(omaplfb_config_tablet),
	.configs = omaplfb_config_tablet,
};

static struct omapfb_platform_data sdp4430_fb_data = {
	.mem_desc = {
		.region_cnt = ARRAY_SIZE(omaplfb_config_tablet),
	},
};

void __init omap4_kc1_android_display_setup(void)
{
	omap_android_display_setup(&sdp4430_dss_data,
				   &dsscomp_config_tablet,
				   &omaplfb_plat_data_tablet,
				   &sdp4430_fb_data);
}

void __init omap4_kc1_display_init(void)
{
	platform_add_devices(sdp4430_panel_devices, ARRAY_SIZE(sdp4430_panel_devices));

	omapfb_set_platform_data(&sdp4430_fb_data);
	omap_vram_set_sdram_vram(OTTER_FB_RAM_SIZE, 0);
	spi_register_board_info(tablet_spi_board_info,	ARRAY_SIZE(tablet_spi_board_info));

//	omap_mux_enable_wkup("sys_nirq1");
//	omap_mux_enable_wkup("sys_nirq2");

	omap_display_init(&sdp4430_dss_data);
}

