/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
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

#ifndef _SB700_EARLY_SETUP_C_
#define _SB700_EARLY_SETUP_C_

#include <reset.h>
#include <arch/cpu.h>
#include "sb700.h"
#include "smbus.c"

#define SMBUS_IO_BASE 0x6000	/* Is it a temporary SMBus I/O base address? */
	 /*SIZE 0x40 */

static void pmio_write(u8 reg, u8 value)
{
	outb(reg, PM_INDEX);
	outb(value, PM_INDEX + 1);
}

static u8 pmio_read(u8 reg)
{
	outb(reg, PM_INDEX);
	return inb(PM_INDEX + 1);
}

/* RPR 2.28: Get SB ASIC Revision. */
static u8 set_sb700_revision(void)
{
	device_t dev;
	u8 rev_id, enable_14Mhz, byte;
	u8 rev = 0;

	/* if (rev != 0) return rev; */

	dev = pci_locate_device(PCI_ID(0x1002, 0x4385), 0);

	if (dev == PCI_DEV_INVALID) {
		die("SMBUS controller not found\n");
		/* NOT REACHED */
	}
	rev_id =  pci_read_config8(dev, 0x08);

	if (rev_id == 0x39) {
		enable_14Mhz = (pmio_read(0x53) >> 6) & 1;
		if (enable_14Mhz == 0x0)
			rev = 0x11;	/* A11 */
		else if (enable_14Mhz == 0x1) {
			/* This happens, if does, only once. So later if we need to get
			 * the revision ID, we don't have to make such a big function.
			 * We just get reg 0x8 in smbus dev. 0x39 is A11, 0x3A is A12. */
			rev = 0x12;
			byte = pci_read_config8(dev, 0x40);
			byte |= 1 << 0;
			pci_write_config8(dev, 0x40, byte);

			pci_write_config8(dev, 0x08, 0x3A); /* Change 0x39 to 0x3A. */

			byte &= ~(1 << 0);
			pci_write_config8(dev, 0x40, byte);
		}
	} else if (rev_id == 0x3A) { /* A12 will be 0x3A after BIOS is initialized */
		rev = 0x12;
	} else if (rev_id == 0x3C) {
		rev = 0x14;
	} else if (rev_id == 0x3D) {
		rev = 0x15;
	} else
		die("It is not SB700 or SB710\n");

	return rev;
}

/***************************************
* Legacy devices are mapped to LPC space.
*	Serial port 0
*	KBC Port
*	ACPI Micro-controller port
*	LPC ROM size
*	This function does not change port 0x80 decoding.
*	Console output through any port besides 0x3f8 is unsupported.
*	If you use FWH ROMs, you have to setup IDSEL.
***************************************/
static void sb700_lpc_init(void)
{
	u8 reg8;
	u32 reg32;
	device_t dev;

	dev = pci_locate_device(PCI_ID(0x1002, 0x4385), 0);	/* SMBUS controller */
	/* NOTE: Set BootTimerDisable, otherwise it would keep rebooting!!
	 * This bit has no meaning if debug strap is not enabled. So if the
	 * board keeps rebooting and the code fails to reach here, we could
	 * disable the debug strap first. */
	reg32 = pci_read_config32(dev, 0x4C);
	reg32 |= 1 << 31;
	pci_write_config32(dev, 0x4C, reg32);

	/* Enable lpc controller */
	reg32 = pci_read_config32(dev, 0x64);
	reg32 |= 1 << 20;
	pci_write_config32(dev, 0x64, reg32);

	dev = pci_locate_device(PCI_ID(0x1002, 0x439d), 0);	/* LPC Controller */
	/* Decode port 0x3f8-0x3ff (Serial 0) */
	// XXX Serial port decode on LPC is hardcoded to 0x3f8
	reg8 = pci_read_config8(dev, 0x44);
	reg8 |= 1 << 6;
	pci_write_config8(dev, 0x44, reg8);

	/* Decode port 0x60 & 0x64 (PS/2 keyboard) and port 0x62 & 0x66 (ACPI)*/
	reg8 = pci_read_config8(dev, 0x47);
	reg8 |= (1 << 5) | (1 << 6);
	pci_write_config8(dev, 0x47, reg8);

	/* Enable PrefetchEnSPIFromHost to speed up SPI flash read (does not affect LPC) */
	reg8 = pci_read_config8(dev, 0xbb);
	reg8 |= 1 << 0;
	pci_write_config8(dev, 0xbb, reg8);

	/* SuperIO, LPC ROM */
	reg8 = pci_read_config8(dev, 0x48);
	/* Decode ports 0x2e-0x2f, 0x4e-0x4f (SuperI/O configuration) */
	reg8 |= (1 << 1) | (1 << 0);
	/* Decode variable LPC ROM address ranges 1&2 (see register 0x68-0x6b, 0x6c-0x6f) */
	reg8 |= (1 << 3) | (1 << 4);
	/* Decode port 0x70-0x73 (RTC) */
	reg8 |= 1 << 6;
	pci_write_config8(dev, 0x48, reg8);

	/* hardware should enable LPC ROM by pin straps */
	/* ROM access at 0xFFF80000/0xFFF00000 - 0xFFFFFFFF */
	/* See detail in 43366_sb700_bdg_nda_1.01.pdf page 17. */
	/* enable LPC ROM range mirroring start 0x000e(0000) */
	pci_write_config16(dev, 0x68, 0x000e);
	/* enable LPC ROM range mirroring end   0x000f(ffff) */
	pci_write_config16(dev, 0x6a, 0x000f);
	/* enable LPC ROM range start, 0xfff8(0000): 512KB, 0xfff0(0000): 1MB */
	pci_write_config16(dev, 0x6c, 0xfff0);
	/* enable LPC ROM range end at 0xffff(ffff) */
	pci_write_config16(dev, 0x6e, 0xffff);
}

/* what is its usage? */
static u32 get_sbdn(u32 bus)
{
	device_t dev;

	/* Find the device. */
	dev = pci_locate_device_on_bus(PCI_ID(0x1002, 0x4385), bus);
	return (dev >> 15) & 0x1f;
}

static u8 dual_core(void)
{
	return (pci_read_config32(PCI_DEV(0, 0x18, 3), 0xE8) & (0x3<<12)) != 0;
}

/*
 * RPR 2.4 C-state and VID/FID change for the K8 platform.
 */
static void enable_fid_change_on_sb(u32 sbbusn, u32 sbdn)
{
	u8 byte;
	byte = pmio_read(0x9a);
	byte &= ~0x34;
	if (dual_core())
		byte |= 0x34;
	else
		byte |= 0x04;
	pmio_write(0x9a, byte);

	byte = pmio_read(0x8f);
	byte &= ~0x30;
	byte |= 0x20;
	pmio_write(0x8f, byte);

	pmio_write(0x8b, 0x01);	/* TODO: if the HT Link is 200 MHz, it is 0x0A. It doesnt often happen. */
	pmio_write(0x8a, 0x90);

	pmio_write(0x88, 0x10);

	byte = pmio_read(0x7c);
	byte |= 0x03;
	pmio_write(0x7c, byte);

	/* Must be 0 for K8 platform. */
	byte = pmio_read(0x68);
	byte &= ~0x01;
	pmio_write(0x68, byte);
	/* Must be 0 for K8 platform. */
	byte = pmio_read(0x8d);
	byte &= ~(1<<6);
	pmio_write(0x8d, byte);

	byte = pmio_read(0x61);
	byte &= ~0x04;
	pmio_write(0x61, byte);

	byte = pmio_read(0x42);
	byte &= ~0x04;
	pmio_write(0x42, byte);

	pmio_write(0x89, 0x10);
}

void hard_reset(void)
{
	set_bios_reset();

	/* full reset */
	outb(0x0a, 0x0cf9);
	outb(0x0e, 0x0cf9);
}

void soft_reset(void)
{
	set_bios_reset();
	/* link reset */
	outb(0x06, 0x0cf9);
}

void sb700_pci_port80(void)
{
	u8 byte;
	device_t dev;

	/* P2P Bridge */
	dev = pci_locate_device(PCI_ID(0x1002, 0x4384), 0);

	/* Chip Control: Enable subtractive decoding */
	byte = pci_read_config8(dev, 0x40);
	byte |= 1 << 5;
	pci_write_config8(dev, 0x40, byte);

	/* Misc Control: Enable subtractive decoding if 0x40 bit 5 is set */
	byte = pci_read_config8(dev, 0x4B);
	byte |= 1 << 7;
	pci_write_config8(dev, 0x4B, byte);

	/* The same IO Base and IO Limit here is meaningful because we set the
	 * bridge to be subtractive. During early setup stage, we have to make
	 * sure that data can go through port 0x80.
	 */
	/* IO Base: 0xf000 */
	byte = pci_read_config8(dev, 0x1C);
	byte |= 0xF << 4;
	pci_write_config8(dev, 0x1C, byte);

	/* IO Limit: 0xf000 */
	byte = pci_read_config8(dev, 0x1D);
	byte |= 0xF << 4;
	pci_write_config8(dev, 0x1D, byte);

	/* PCI Command: Enable IO response */
	byte = pci_read_config8(dev, 0x04);
	byte |= 1 << 0;
	pci_write_config8(dev, 0x04, byte);

	/* LPC controller */
	dev = pci_locate_device(PCI_ID(0x1002, 0x439D), 0);

	byte = pci_read_config8(dev, 0x4A);
	byte &= ~(1 << 5);	/* disable lpc port 80 */
	pci_write_config8(dev, 0x4A, byte);
}

void sb700_lpc_port80(void)
{
	u8 byte;
	device_t dev;
	u32 reg32;

	/* Enable LPC controller */
	dev = pci_locate_device(PCI_ID(0x1002, 0x4385), 0);
	reg32 = pci_read_config32(dev, 0x64);
	reg32 |= 0x00100000;	/* lpcEnable */
	pci_write_config32(dev, 0x64, reg32);

	/* Enable port 80 LPC decode in pci function 3 configuration space. */
	dev = pci_locate_device(PCI_ID(0x1002, 0x439d), 0);
	byte = pci_read_config8(dev, 0x4a);
	byte |= 1 << 5;		/* enable port 80 */
	pci_write_config8(dev, 0x4a, byte);
}

/* sbDevicesPorInitTable */
static void sb700_devices_por_init(void)
{
	device_t dev;
	u8 byte;

	printk(BIOS_INFO, "sb700_devices_por_init()\n");
	/* SMBus Device, BDF:0-20-0 */
	printk(BIOS_INFO, "sb700_devices_por_init(): SMBus Device, BDF:0-20-0\n");
	dev = pci_locate_device(PCI_ID(0x1002, 0x4385), 0);

	if (dev == PCI_DEV_INVALID) {
		die("SMBUS controller not found\n");
		/* NOT REACHED */
	}
	printk(BIOS_INFO, "SMBus controller enabled, sb revision is A%x\n",
		    set_sb700_revision());

	/* sbPorAtStartOfTblCfg */
	/* Set A-Link bridge access address. This address is set at device 14h, function 0, register 0xf0.
	 * This is an I/O address. The I/O address must be on 16-byte boundry.  */
	pci_write_config32(dev, 0xf0, AB_INDX);

	/* To enable AB/BIF DMA access, a specific register inside the BIF register space needs to be configured first. */
	/* 4.3:Enables the SB700 to send transactions upstream over A-Link Express interface. */
	axcfg_reg(0x04, 1 << 2, 1 << 2);
	axindxc_reg(0x21, 0xff, 0);

	/* 2.5:Enabling Non-Posted Memory Write for the K8 Platform */
	axindxc_reg(0x10, 1 << 9, 1 << 9);
	/* END of sbPorAtStartOfTblCfg */

	/* sbDevicesPorInitTables */
	/* set smbus iobase */
	pci_write_config32(dev, 0x90, SMBUS_IO_BASE | 1);

	/* enable smbus controller interface */
	byte = pci_read_config8(dev, 0xd2);
	byte |= (1 << 0);
	pci_write_config8(dev, 0xd2, byte);

	/* KB2RstEnable */
	pci_write_config8(dev, 0x40, 0x44);

	/* Enable ISA Address 0-960K decoding */
	pci_write_config8(dev, 0x48, 0x0f);

	/* Enable ISA  Address 0xC0000-0xDFFFF decode */
	pci_write_config8(dev, 0x49, 0xff);

	/* Enable decode cycles to IO C50, C51, C52 GPM controls. */
	byte = pci_read_config8(dev, 0x41);
	byte &= 0x80;
	byte |= 0x33;
	pci_write_config8(dev, 0x41, byte);

	/* Legacy DMA Prefetch Enhancement, CIM masked it. */
	/* pci_write_config8(dev, 0x43, 0x1); */

	/* Disabling Legacy USB Fast SMI# */
	byte = pci_read_config8(dev, 0x62);
	byte |= 0x24;
	pci_write_config8(dev, 0x62, byte);

	/* Features Enable */
	pci_write_config32(dev, 0x64, 0x829E79BF); /* bit10: Enables the HPET interrupt. */

	/* SerialIrq Control */
	pci_write_config8(dev, 0x69, 0x90);

	/* Test Mode, PCIB_SReset_En Mask is set. */
	pci_write_config8(dev, 0x6c, 0x20);

	/* IO Address Enable, CIM set 0x78 only and masked 0x79. */
	/*pci_write_config8(dev, 0x79, 0x4F); */
	pci_write_config8(dev, 0x78, 0xFF);

	/* Set smbus iospace enable, I don't know why write 0x04 into reg5 that is reserved */
	pci_write_config16(dev, 0x4, 0x0407);

	/* clear any lingering errors, so the transaction will run */
	outb(inb(SMBUS_IO_BASE + SMBHSTSTAT), SMBUS_IO_BASE + SMBHSTSTAT);

	/* IDE Device, BDF:0-20-1 */
	printk(BIOS_INFO, "sb700_devices_por_init(): IDE Device, BDF:0-20-1\n");
	dev = pci_locate_device(PCI_ID(0x1002, 0x439C), 0);
	/* Disable prefetch */
	byte = pci_read_config8(dev, 0x63);
	byte |= 0x1;
	pci_write_config8(dev, 0x63, byte);

	/* LPC Device, BDF:0-20-3 */
	printk(BIOS_INFO, "sb700_devices_por_init(): LPC Device, BDF:0-20-3\n");
	dev = pci_locate_device(PCI_ID(0x1002, 0x439D), 0);
	/* DMA enable */
	pci_write_config8(dev, 0x40, 0x04);

	/* IO Port Decode Enable */
	pci_write_config8(dev, 0x44, 0xFF);
	pci_write_config8(dev, 0x45, 0xFF);
	pci_write_config8(dev, 0x46, 0xC3);
	pci_write_config8(dev, 0x47, 0xFF);

	/* IO/Mem Port Decode Enable, I don't know why CIM disable some ports.
	 *  Disable LPC TimeOut counter, enable SuperIO Configuration Port (2e/2f),
	 * Alternate SuperIO Configuration Port (4e/4f), Wide Generic IO Port (64/65).
	 * Enable bits for LPC ROM memory address range 1&2 for 1M ROM setting.*/
	byte = pci_read_config8(dev, 0x48);
	byte |= (1 << 1) | (1 << 0);	/* enable Super IO config port 2e-2h, 4e-4f */
	byte |= (1 << 3) | (1 << 4);	/* enable for LPC ROM address range1&2, Enable 512KB rom access at 0xFFF80000 - 0xFFFFFFFF */
	byte |= 1 << 6;		/* enable for RTC I/O range */
	pci_write_config8(dev, 0x48, byte);
	pci_write_config8(dev, 0x49, 0xFF);
	/* Enable 0x480-0x4bf, 0x4700-0x470B */
	byte = pci_read_config8(dev, 0x4A);
	byte |= ((1 << 1) + (1 << 6));	/*0x42, save the configuraion for port 0x80. */
	pci_write_config8(dev, 0x4A, byte);

	/* Set LPC ROM size, it has been done in sb700_lpc_init().
	 * enable LPC ROM range, 0xfff8: 512KB, 0xfff0: 1MB;
	 * enable LPC ROM range, 0xfff8: 512KB, 0xfff0: 1MB
	 * pci_write_config16(dev, 0x68, 0x000e)
	 * pci_write_config16(dev, 0x6c, 0xfff0);*/

	/* Enable Tpm12_en and Tpm_legacy. I don't know what is its usage and copied from CIM. */
	pci_write_config8(dev, 0x7C, 0x05);

	/* P2P Bridge, BDF:0-20-4, the configuration of the registers in this dev are copied from CIM,
	 */
	printk(BIOS_INFO, "sb700_devices_por_init(): P2P Bridge, BDF:0-20-4\n");
	dev = pci_locate_device(PCI_ID(0x1002, 0x4384), 0);

	/* Arbiter enable. */
	pci_write_config8(dev, 0x43, 0xff);

	/* Set PCDMA request into hight priority list. */
	/* pci_write_config8(dev, 0x49, 0x1) */ ;

	pci_write_config8(dev, 0x40, 0x26);

	pci_write_config8(dev, 0x0d, 0x40);
	pci_write_config8(dev, 0x1b, 0x40);
	/* Enable PCIB_DUAL_EN_UP will fix potential problem with PCI cards. */
	pci_write_config8(dev, 0x50, 0x01);

	/* SATA Device, BDF:0-17-0, Non-Raid-5 SATA controller */
	printk(BIOS_INFO, "sb700_devices_por_init(): SATA Device, BDF:0-18-0\n");
	dev = pci_locate_device(PCI_ID(0x1002, 0x4390), 0);

	/*PHY Global Control*/
	pci_write_config16(dev, 0x86, 0x2C00);
}

/* sbPmioPorInitTable, Pre-initializing PMIO register space
* The power management (PM) block is resident in the PCI/LPC/ISA bridge.
* The PM regs are accessed via IO mapped regs 0xcd6 and 0xcd7.
* The index address is first programmed into IO reg 0xcd6.
* Read or write values are accessed through IO reg 0xcd7.
*/
static void sb700_pmio_por_init(void)
{
	u8 byte;

	printk(BIOS_INFO, "sb700_pmio_por_init()\n");
	/* K8KbRstEn, KB_RST# control for K8 system. */
	byte = pmio_read(0x66);
	byte |= 0x20;
	pmio_write(0x66, byte);

	/* RPR2.31 PM_TURN_OFF_MSG during ASF Shutdown. */
	if (get_sb700_revision(pci_locate_device(PCI_ID(0x1002, 0x4385), 0)) <= 0x12) {
		byte = pmio_read(0x65);
		byte &= ~(1 << 7);
		pmio_write(0x65, byte);

		byte = pmio_read(0x75);
		byte &= 0xc0;
		byte |= 0x05;
		pmio_write(0x75, byte);

		byte = pmio_read(0x52);
		byte &= 0xc0;
		byte |= 0x08;
		pmio_write(0x52, byte);
	} else {
		byte = pmio_read(0xD7);
		byte |= 1 << 0;
		pmio_write(0xD7, byte);

		byte = pmio_read(0x65);
		byte |= 1 << 7;
		pmio_write(0x65, byte);

		byte = pmio_read(0x75);
		byte &= 0xc0;
		byte |= 0x01;
		pmio_write(0x75, byte);

		byte = pmio_read(0x52);
		byte &= 0xc0;
		byte |= 0x02;
		pmio_write(0x52, byte);

	}

	/* Watch Dog Timer Control
	 * Set watchdog time base to 0xfec000f0 to avoid SCSI card boot failure.
	 * But I don't find WDT is enabled in SMBUS 0x41 bit3 in CIM.
	 */
	pmio_write(0x6c, 0xf0);
	pmio_write(0x6d, 0x00);
	pmio_write(0x6e, 0xc0);
	pmio_write(0x6f, 0xfe);

	/* rpr2.15: Enabling Spread Spectrum */
	byte = pmio_read(0x42);
	byte |= 1 << 7;
	pmio_write(0x42, byte);
	/* TODO: Check if it is necessary. IDE reset */
	byte = pmio_read(0xB2);
	byte |= 1 << 0;
	pmio_write(0xB2, byte);
}

/*
* Add any south bridge setting.
*/
static void sb700_pci_cfg(void)
{
	device_t dev;
	u8 byte;

	/* SMBus Device, BDF:0-20-0 */
	dev = pci_locate_device(PCI_ID(0x1002, 0x4385), 0);
	/* Enable watchdog decode timer */
	byte = pci_read_config8(dev, 0x41);
	byte |= (1 << 3);
	pci_write_config8(dev, 0x41, byte);

	/* Set to 1 to reset USB on the software (such as IO-64 or IO-CF9 cycles)
	 * generated PCIRST#. */
	byte = pmio_read(0x65);
	byte |= (1 << 4);
	pmio_write(0x65, byte);

	/* IDE Device, BDF:0-20-1 */
	dev = pci_locate_device(PCI_ID(0x1002, 0x439C), 0);
	/* Enable IDE Explicit prefetch, 0x63[0] clear */
	byte = pci_read_config8(dev, 0x63);
	byte &= 0xfe;
	pci_write_config8(dev, 0x63, byte);

	/* LPC Device, BDF:0-20-3 */
	/* The code below is ported from old chipset. It is not
	 * mentioned in RPR. But I keep them. The registers and the
	 * comments are compatible. */
	dev = pci_locate_device(PCI_ID(0x1002, 0x439D), 0);
	/* Enabling LPC DMA function. */
	byte = pci_read_config8(dev, 0x40);
	byte |= (1 << 2);
	pci_write_config8(dev, 0x40, byte);
	/* Disabling LPC TimeOut. 0x48[7] clear. */
	byte = pci_read_config8(dev, 0x48);
	byte &= 0x7f;
	pci_write_config8(dev, 0x48, byte);
	/* Disabling LPC MSI Capability, 0x78[1] clear. */
	byte = pci_read_config8(dev, 0x78);
	byte &= 0xfd;
	pci_write_config8(dev, 0x78, byte);

	/* SATA Device, BDF:0-17-0, Non-Raid-5 SATA controller */
	dev = pci_locate_device(PCI_ID(0x1002, 0x4390), 0);
	/* rpr7.12 SATA MSI and D3 Power State Capability. */
	byte = pci_read_config8(dev, 0x40);
	byte |= 1 << 0;
	pci_write_config8(dev, 0x40, byte);
	if (get_sb700_revision(pci_locate_device(PCI_ID(0x1002, 0x4385), 0)) <= 0x12)
		pci_write_config8(dev, 0x34, 0x70); /* set 0x61 to 0x70 if S1 is not supported. */
	else
		pci_write_config8(dev, 0x34, 0x50); /* set 0x61 to 0x50 if S1 is not supported. */
	byte &= ~(1 << 0);
	pci_write_config8(dev, 0x40, byte);
}

/*
*/
static void sb700_por_init(void)
{
	/* sbDevicesPorInitTable + sbK8PorInitTable */
	sb700_devices_por_init();

	/* sbPmioPorInitTable + sbK8PmioPorInitTable */
	sb700_pmio_por_init();
}

/*
* It should be called during early POST after memory detection and BIOS shadowing but before PCI bus enumeration.
*/
static void sb700_before_pci_init(void)
{
	sb700_pci_cfg();
}

/*
* This function should be called after enable_sb700_smbus().
*/
static void sb700_early_setup(void)
{
	printk(BIOS_INFO, "sb700_early_setup()\n");
	sb700_por_init();
}

static int smbus_read_byte(u32 device, u32 address)
{
	return do_smbus_read_byte(SMBUS_IO_BASE, device, address);
}
#endif