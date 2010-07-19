/* linux/drivers/video/samsung/s3cfb_lvds.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 **/
#include "s3cfb.h"

static struct s3cfb_lcd lvds = {
        .width = 1024,
        .height = 600,
        .bpp = 16,
        .freq = 60,

        .timing = {
                 .h_fp = 79,
                .h_bp = 200,
                .h_sw = 4,
                .v_fp = 10,
                .v_fpe = 1,
                .v_bp = 11,
                .v_bpe = 1,
                .v_sw = 5,
	
        },

        .polarity = {
                .rise_vclk = 1,
                .inv_hsync = 1,
                .inv_vsync = 1,
                .inv_vden = 0,
        },
};

/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
        lvds.init_ldi = NULL;
        ctrl->lcd = &lvds;
}
