/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Corey Osgood <corey@slightlyhackish.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#ifndef SUPERIO_FINTEK_F71805F_CHIP_H
#define SUPERIO_FINTEK_F71805F_CHIP_H

#include <device/device.h>
#include <uart8250.h>

/* This chip doesn't have keyboard and mouse support. */

extern struct chip_operations superio_fintek_f71805f_ops;

struct superio_fintek_f71805f_config {
	struct uart8250 com1, com2;
};

#endif
