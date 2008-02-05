/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <console/console.h>
#include <device/device.h>
#include "chip.h"

static void init(struct device *dev)
{
	printk_debug("ALIX1.C ENTER %s\n", __FUNCTION__);
	printk_debug("ALIX1.C EXIT %s\n", __FUNCTION__);
}

static void enable_dev(struct device *dev)
{
	dev->ops->init = init;
}

struct chip_operations mainboard_pcengines_alix1c_ops = {
	CHIP_NAME("PC Engines ALIX1.C Mainboard")
	.enable_dev = enable_dev,
};

