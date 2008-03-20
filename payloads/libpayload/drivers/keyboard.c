/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arch/io.h>
#include <libpayload.h>

unsigned char map[2][0x57] = {
	{
	 0x00, 0x1B, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
	 0x37, 0x38, 0x39, 0x30, 0x2D, 0x3D, 0x08, 0x09,
	 0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69,
	 0x6F, 0x70, 0x5B, 0x5D, 0x0A, 0x00, 0x61, 0x73,
	 0x64, 0x66, 0x67, 0x68, 0x6A, 0x6B, 0x6C, 0x3B,
	 0x27, 0x60, 0x00, 0x5C, 0x7A, 0x78, 0x63, 0x76,
	 0x62, 0x6E, 0x6D, 0x2C, 0x2E, 0x2F, 0x00, 0x2A,
	 0x00, 0x20,
	 },
	{
	 0x00, 0x1B, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5E,
	 0x26, 0x2A, 0x28, 0x29, 0x5F, 0x2B, 0x08, 0x00,
	 0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49,
	 0x4F, 0x50, 0x7B, 0x7D, 0x0A, 0x00, 0x41, 0x53,
	 0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x3A,
	 0x22, 0x7E, 0x00, 0x7C, 0x5A, 0x58, 0x43, 0x56,
	 0x42, 0x4E, 0x4D, 0x3C, 0x3E, 0x3F, 0x00, 0x2A,
	 0x00, 0x20,
	 }
};

#define MOD_SHIFT    1
#define MOD_CTRL     2
#define MOD_CAPSLOCK 4

int keyboard_havechar(void)
{
	unsigned char c = inb(0x64);
	return c & 1;
}

unsigned char keyboard_get_scancode(void)
{
	unsigned char ch = 0;

	if (keyboard_havechar())
		ch = inb(0x60);

	return ch;
}

int keyboard_getchar(void)
{
	static int modifier;
	unsigned char ch;
	int shift;
	int ret = 0;

	while (!keyboard_havechar()) ;

	ch = keyboard_get_scancode();

	switch (ch) {
	case 0x36:
	case 0x2a:
		modifier &= ~MOD_SHIFT;
		break;
	case 0x80 | 0x36:
	case 0x80 | 0x2a:
		modifier |= MOD_SHIFT;
	case 0x1d:
		modifier &= ~MOD_CTRL;
		break;
	case 0x80 | 0x1d:
		modifier |= MOD_CTRL;
		break;
	case 0x3a:
		if (modifier & MOD_CAPSLOCK)
			modifier &= ~MOD_CAPSLOCK;
		else
			modifier |= MOD_CAPSLOCK;
		break;
	}

	if (!(ch & 0x80) && ch < 0x57) {
		shift =
		    (modifier & MOD_SHIFT) ^ (modifier & MOD_CAPSLOCK) ? 1 : 0;
		ret = map[shift][ch];

		if (modifier & MOD_CTRL)
			ret = (ret >= 0x3F && ret <= 0x5F) ? ret & 0x1f : 0;

		return ret;
	}

	return ret;
}
