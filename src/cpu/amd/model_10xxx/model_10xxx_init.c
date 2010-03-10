/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
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

#include <console/console.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/mtrr.h>
#include <device/device.h>
#include <device/pci.h>
#include <string.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/pae.h>
#include <pc80/mc146818rtc.h>
#include <cpu/x86/lapic.h>

#include "../../../northbridge/amd/amdfam10/amdfam10.h"

#include <cpu/amd/model_10xxx_rev.h>
#include <cpu/cpu.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/mem.h>
#include <cpu/amd/quadcore.h>
#include <cpu/amd/model_10xxx_msr.h>

extern device_t get_node_pci(u32 nodeid, u32 fn);
extern int init_processor_name(void);



#define MCI_STATUS 0x401


msr_t rdmsr_amd(u32 index)
{
	 msr_t result;
	 __asm__ __volatile__ (
		 "rdmsr"
		 : "=a" (result.lo), "=d" (result.hi)
		 : "c" (index), "D" (0x9c5a203a)
		 );
	 return result;
}


void wrmsr_amd(u32 index, msr_t msr)
{
	__asm__ __volatile__ (
		"wrmsr"
		: /* No outputs */
		: "c" (index), "a" (msr.lo), "d" (msr.hi), "D" (0x9c5a203a)
		);
}


void model_10xxx_init(device_t dev)
{
	u8 i;
	msr_t msr;
	struct node_core_id id;
#if CONFIG_LOGICAL_CPUS == 1
	u32 siblings;
#endif

	id = get_node_core_id(read_nb_cfg_54()); /* nb_cfg_54 can not be set */
	printk_debug("nodeid = %02d, coreid = %02d\n", id.nodeid, id.coreid);

	/* Turn on caching if we haven't already */
	x86_enable_cache();
	amd_setup_mtrrs();
	x86_mtrr_check();

	disable_cache();

	/* zero the machine check error status registers */
	msr.lo = 0;
	msr.hi = 0;
	for(i=0; i < 5; i++) {
		wrmsr(MCI_STATUS + (i * 4),msr);
	}


	enable_cache();

	/* Enable the local cpu apics */
	setup_lapic();

	/* Set the processor name string */
	init_processor_name();

#if CONFIG_LOGICAL_CPUS == 1
	siblings = cpuid_ecx(0x80000008) & 0xff;

	if (siblings > 0) {
		msr = rdmsr_amd(CPU_ID_FEATURES_MSR);
		msr.lo |= 1 << 28;
		wrmsr_amd(CPU_ID_FEATURES_MSR, msr);

		msr = rdmsr_amd(CPU_ID_EXT_FEATURES_MSR);
		msr.hi |= 1 << (33-32);
		wrmsr_amd(CPU_ID_EXT_FEATURES_MSR, msr);
	}
	printk_debug("siblings = %02d, ", siblings);
#endif

	/* DisableCf8ExtCfg */
	msr = rdmsr(NB_CFG_MSR);
	msr.hi &= ~(1 << (46-32));
	wrmsr(NB_CFG_MSR, msr);

	/* Write protect SMM space with SMMLOCK. */
	msr = rdmsr(HWCR_MSR);
	msr.lo |= (1 << 0);
	wrmsr(HWCR_MSR, msr);

}

static struct device_operations cpu_dev_ops = {
	.init = model_10xxx_init,
};
static struct cpu_device_id cpu_table[] = {
//AMD_GH_SUPPORT
	{ X86_VENDOR_AMD, 0x100f00 },		/* SH-F0 L1 */
	{ X86_VENDOR_AMD, 0x100f10 },		/* M2 */
	{ X86_VENDOR_AMD, 0x100f20 },		/* S1g1 */
	{ X86_VENDOR_AMD, 0x100f21 },
	{ X86_VENDOR_AMD, 0x100f2A },
	{ X86_VENDOR_AMD, 0x100f22 },
	{ X86_VENDOR_AMD, 0x100f23 },
	{ X86_VENDOR_AMD, 0x100f40 },		/* RB-C0 */
	{ X86_VENDOR_AMD, 0x100F42 },           /* RB-C2 */ 
	{ X86_VENDOR_AMD, 0x100F52 },           /* BL-C2 */ 
	{ X86_VENDOR_AMD, 0x100F62 },           /* DA-C2 */ 
	{ X86_VENDOR_AMD, 0x100F80 },           /* HY-D0 */ 
	{ 0, 0 },
};
static struct cpu_driver model_10xxx __cpu_driver = {
	.ops	  = &cpu_dev_ops,
	.id_table = cpu_table,
};
