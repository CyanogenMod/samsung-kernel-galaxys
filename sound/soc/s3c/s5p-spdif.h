/*
 * s5p-spdif.h  --  ALSA Soc Audio Layer
 *
 * Copyright 2005 Wolfson Microelectronics PLC.
 * Author: Graeme Gregory
 *         graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    1th April 2009   Initial version.
 */

#ifndef S5PC1XXSPDIF_H_
#define S5PC1XXSPDIF_H_

#include <linux/clk.h>

extern struct snd_soc_dai	s5p_spdif_dai;

struct s5p_spdif_info {
	void __iomem	*regs;
	struct clk	*spdif_clk;
	struct clk	*spdif_sclk;
	int master;
};
extern struct s5p_spdif_info s5p_spdif;

enum spdif_intr_src
{
	SPDIF_INT_FIFOLVL,
	SPDIF_INT_USERDATA,
	SPDIF_INT_BUFEMPTY,
	SPDIF_INT_STREAMEND,
	SPDIF_INT_UNKNOWN
};

#endif /*S5PC1XXSPDIF_H_*/

