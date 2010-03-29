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

#define ASSEMBLY 1


#define RAMINIT_SYSINFO 1
#define K8_SET_FIDVID 1
#define QRANK_DIMM_SUPPORT 1
#if CONFIG_LOGICAL_CPUS==1
#define SET_NB_CFG_54 1
#endif

#define RC0 (6<<8)
#define RC1 (7<<8)

#define DIMM0 0x50
#define DIMM1 0x51

#define ICS951462_ADDRESS	0x69
#define SMBUS_HUB 0x71

#include <stdint.h>
#include <string.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"

#define post_code(x) outb(x, 0x80)

#include <cpu/amd/model_fxx_rev.h>
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"

#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "superio/ite/it8718f/it8718f_early_serial.c"

#include "cpu/amd/mtrr/amd_earlymtrr.c"
#include "cpu/x86/bist.h"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#include "southbridge/amd/rs780/rs780_early_setup.c"
#include "southbridge/amd/sb700/sb700_early_setup.c"
#include "northbridge/amd/amdk8/debug.c" /* After sb700_early_setup.c! */

/* CAN'T BE REMOVED! crt0.S will use it. I don't know WHY!*/
static void memreset(int controllers, const struct mem_controller *ctrl)
{
}

/* called in raminit_f.c */
static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
}

/*called in raminit_f.c */
static inline int spd_read_byte(u32 device, u32 address)
{
	return smbus_read_byte(device, address);
}

#include "northbridge/amd/amdk8/amdk8.h"
#include "northbridge/amd/amdk8/incoherent_ht.c"
#include "northbridge/amd/amdk8/raminit_f.c"
#include "northbridge/amd/amdk8/coherent_ht.c"
#include "lib/generic_sdram.c"
#include "resourcemap.c"

#include "cpu/amd/dualcore/dualcore.c"

#include "cpu/amd/car/copy_and_run.c"
#include "cpu/amd/car/post_cache_as_ram.c"

#include "cpu/amd/model_fxx/init_cpus.c"

#include "cpu/amd/model_fxx/fidvid.c"

#include "northbridge/amd/amdk8/early_ht.c"

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
	static const u16 spd_addr[] = { DIMM0, 0, 0, 0, DIMM1, 0, 0, 0, };
	int needs_reset = 0;
	u32 bsp_apicid = 0;
	msr_t msr;
	struct cpuid_result cpuid1;
	struct sys_info *sysinfo = (struct sys_info *)(CONFIG_DCACHE_RAM_BASE + CONFIG_DCACHE_RAM_SIZE - CONFIG_DCACHE_RAM_GLOBAL_VAR_SIZE);

	if (!cpu_init_detectedx && boot_cpu()) {
		/* Nothing special needs to be done to find bus 0 */
		/* Allow the HT devices to be found */
		enumerate_ht_chain();

		/* sb700_lpc_port80(); */
		sb700_pci_port80();
	}

	if (bist == 0) {
		bsp_apicid = init_cpus(cpu_init_detectedx, sysinfo);
	}

	enable_rs780_dev8();
	sb700_lpc_init();

	it8718f_enable_serial(0, CONFIG_TTYS0_BASE);
	uart_init();
	console_init();

	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);
	printk(BIOS_DEBUG, "bsp_apicid=0x%x\n", bsp_apicid);

	setup_mahogany_resource_map();

	setup_coherent_ht_domain();

#if CONFIG_LOGICAL_CPUS==1
	/* It is said that we should start core1 after all core0 launched */
	wait_all_core0_started();
	start_other_cores();
#endif
	wait_all_aps_started(bsp_apicid);

	ht_setup_chains_x(sysinfo);

	/* run _early_setup before soft-reset. */
	rs780_early_setup();
	sb700_early_setup();

	/* Check to see if processor is capable of changing FIDVID  */
	/* otherwise it will throw a GP# when reading FIDVID_STATUS */
	cpuid1 = cpuid(0x80000007);
	if( (cpuid1.edx & 0x6) == 0x6 ) {

		/* Read FIDVID_STATUS */
		msr=rdmsr(0xc0010042);
		printk(BIOS_DEBUG, "begin msr fid, vid: hi=0x%x, lo=0x%x\n", msr.hi, msr.lo);

		enable_fid_change();
		enable_fid_change_on_sb(sysinfo->sbbusn, sysinfo->sbdn);
		init_fidvid_bsp(bsp_apicid);

		/* show final fid and vid */
		msr=rdmsr(0xc0010042);
		printk(BIOS_DEBUG, "end msr fid, vid: hi=0x%x, lo=0x%x\n", msr.hi, msr.lo);

	} else {
		printk(BIOS_DEBUG, "Changing FIDVID not supported\n");
	}

	needs_reset = optimize_link_coherent_ht();
	needs_reset |= optimize_link_incoherent_ht(sysinfo);
	rs780_htinit();
	printk(BIOS_DEBUG, "needs_reset=0x%x\n", needs_reset);

	if (needs_reset) {
		print_info("ht reset -\r\n");
		soft_reset();
	}

	allow_all_aps_stop(bsp_apicid);

	/* It's the time to set ctrl now; */
	printk(BIOS_DEBUG, "sysinfo->nodes: %2x  sysinfo->ctrl: %p  spd_addr: %p\n",
		     sysinfo->nodes, sysinfo->ctrl, spd_addr);
	fill_mem_ctrl(sysinfo->nodes, sysinfo->ctrl, spd_addr);
	sdram_initialize(sysinfo->nodes, sysinfo->ctrl, sysinfo);

	rs780_before_pci_init();
	sb700_before_pci_init();

	post_cache_as_ram();
}
