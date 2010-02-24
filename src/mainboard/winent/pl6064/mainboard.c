/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Win Enterprises, Inc. (anishp@win-ent.com)
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
	printk_debug("Win Enterprises PL-6064/65 ENTER %s\n", __func__);
	printk_debug("Win Enterprises PL-6064/65 EXIT %s\n", __func__);
}

static void enable_dev(struct device *dev)
{
	dev->ops->init = init;
}

struct chip_operations mainboard_ops = {
	CHIP_NAME("Win Enterprises PL-6064/65 Mainboard")
	.enable_dev = enable_dev,
};
