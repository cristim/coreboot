/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 AMD
 * Written by Yinghai Lu <yinghailu@amd.com> for AMD.
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

#define RAMINIT_SYSINFO 1

#define K8_ALLOCATE_IO_RANGE 1

#define QRANK_DIMM_SUPPORT 1

#if CONFIG_LOGICAL_CPUS==1
#define SET_NB_CFG_54 1
#endif

//used by init_cpus and fidvid
#define K8_SET_FIDVID 0
//if we want to wait for core1 done before DQS training, set it to 0
#define K8_SET_FIDVID_CORE0_ONLY 1

#if CONFIG_K8_REV_F_SUPPORT == 1
#define K8_REV_F_SUPPORT_F0_F1_WORKAROUND 0
#endif

#define DBGP_DEFAULT 7

#include <stdint.h>
#include <string.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <cpu/x86/lapic.h>
#include "option_table.h"
#include "pc80/mc146818rtc_early.c"

#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#if CONFIG_USBDEBUG_DIRECT
#include "southbridge/nvidia/mcp55/mcp55_enable_usbdebug_direct.c"
#include "pc80/usbdebug_direct_serial.c"
#endif
#include "lib/ramtest.c"

#include <cpu/amd/model_fxx_rev.h>

#include "southbridge/nvidia/mcp55/mcp55_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"

#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "superio/winbond/w83627hf/w83627hf_early_serial.c"
#include "superio/winbond/w83627hf/w83627hf_early_init.c"

#include "cpu/x86/bist.h"

#include "northbridge/amd/amdk8/debug.c"

#include "cpu/amd/mtrr/amd_earlymtrr.c"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#define SERIAL_DEV PNP_DEV(0x2e, W83627HF_SP1)

#include "southbridge/nvidia/mcp55/mcp55_early_ctrl.c"

static void memreset_setup(void)
{
}

static void memreset(int controllers, const struct mem_controller *ctrl)
{
}

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
	/* nothing to do */
}

static inline int spd_read_byte(unsigned device, unsigned address)
{
	return smbus_read_byte(device, address);
}

#include "northbridge/amd/amdk8/amdk8_f.h"
#include "northbridge/amd/amdk8/coherent_ht.c"

#include "northbridge/amd/amdk8/incoherent_ht.c"

#include "northbridge/amd/amdk8/raminit_f.c"

#include "lib/generic_sdram.c"

#include "resourcemap.c"

#include "cpu/amd/dualcore/dualcore.c"

#define MCP55_NUM 1
#define MCP55_USE_NIC 1

#define MCP55_PCI_E_X_0 1

#define MCP55_MB_SETUP \
	RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+37, 0x00, 0x44,/* GPIO38 PCI_REQ3 */ \
	RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+38, 0x00, 0x44,/* GPIO39 PCI_GNT3 */ \
	RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+39, 0x00, 0x44,/* GPIO40 PCI_GNT2 */ \
	RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+40, 0x00, 0x44,/* GPIO41 PCI_REQ2 */ \
	RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+59, 0x00, 0x60,/* GPIP60 FANCTL0 */ \
	RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+60, 0x00, 0x60,/* GPIO61 FANCTL1 */

#include "southbridge/nvidia/mcp55/mcp55_early_setup_ss.h"
#include "southbridge/nvidia/mcp55/mcp55_early_setup_car.c"

#include "cpu/amd/car/copy_and_run.c"

#include "cpu/amd/car/post_cache_as_ram.c"

#include "cpu/amd/model_fxx/init_cpus.c"

#include "cpu/amd/model_fxx/fidvid.c"

#include "southbridge/nvidia/mcp55/mcp55_enable_rom.c"
#include "northbridge/amd/amdk8/early_ht.c"

static void sio_setup(void)
{

	unsigned value;
	uint32_t dword;
	uint8_t byte;

	byte = pci_read_config8(PCI_DEV(0, MCP55_DEVN_BASE+1 , 0), 0x7b);
	byte |= 0x20;
	pci_write_config8(PCI_DEV(0, MCP55_DEVN_BASE+1 , 0), 0x7b, byte);

	dword = pci_read_config32(PCI_DEV(0, MCP55_DEVN_BASE+1 , 0), 0xa0);
	/*serial 0 */
	dword |= (1<<0);
	pci_write_config32(PCI_DEV(0, MCP55_DEVN_BASE+1 , 0), 0xa0, dword);

	dword = pci_read_config32(PCI_DEV(0, MCP55_DEVN_BASE+1 , 0), 0xa4);
	dword |= (1<<16);
	pci_write_config32(PCI_DEV(0, MCP55_DEVN_BASE+1 , 0), 0xa4, dword);

}

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
	static const uint16_t spd_addr [] = {
			(0xa<<3)|0, (0xa<<3)|2, 0, 0,
			(0xa<<3)|1, (0xa<<3)|3, 0, 0,
#if CONFIG_MAX_PHYSICAL_CPUS > 1
			(0xa<<3)|4, (0xa<<3)|6, 0, 0,
			(0xa<<3)|5, (0xa<<3)|7, 0, 0,
#endif
	};

	struct sys_info *sysinfo = (CONFIG_DCACHE_RAM_BASE + CONFIG_DCACHE_RAM_SIZE - CONFIG_DCACHE_RAM_GLOBAL_VAR_SIZE);

	int needs_reset = 0;
	unsigned bsp_apicid = 0;

	if (!cpu_init_detectedx && boot_cpu()) {
		/* Nothing special needs to be done to find bus 0 */
		/* Allow the HT devices to be found */

		enumerate_ht_chain();

		sio_setup();

		/* Setup the mcp55 */
		mcp55_enable_rom();
	}

	if (bist == 0) {
		bsp_apicid = init_cpus(cpu_init_detectedx, sysinfo);
	}

	w83627hf_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);

	setup_mb_resource_map();

	uart_init();

	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);

#if CONFIG_USBDEBUG_DIRECT
	mcp55_enable_usbdebug_direct(DBGP_DEFAULT);
	early_usbdebug_direct_init();
#endif
	console_init();
	printk(BIOS_DEBUG, "*sysinfo range: [%p,%p]\n",sysinfo,sysinfo+1);

	print_debug("bsp_apicid="); print_debug_hex8(bsp_apicid); print_debug("\r\n");

#if CONFIG_MEM_TRAIN_SEQ == 1
	set_sysinfo_in_ram(0); // in BSP so could hold all ap until sysinfo is in ram
#endif
	setup_coherent_ht_domain(); // routing table and start other core0

	wait_all_core0_started();
#if CONFIG_LOGICAL_CPUS==1
	// It is said that we should start core1 after all core0 launched
	/* becase optimize_link_coherent_ht is moved out from setup_coherent_ht_domain,
	 * So here need to make sure last core0 is started, esp for two way system,
	 * (there may be apic id conflicts in that case)
	 */
	start_other_cores();
	wait_all_other_cores_started(bsp_apicid);
#endif

	/* it will set up chains and store link pair for optimization later */
	ht_setup_chains_x(sysinfo); // it will init sblnk and sbbusn, nodes, sbdn

#if K8_SET_FIDVID == 1

	{
		msr_t msr;
		msr=rdmsr(0xc0010042);
		print_debug("begin msr fid, vid "); print_debug_hex32( msr.hi ); print_debug_hex32(msr.lo); print_debug("\r\n");

	}

	enable_fid_change();

	enable_fid_change_on_sb(sysinfo->sbbusn, sysinfo->sbdn);

	init_fidvid_bsp(bsp_apicid);

	// show final fid and vid
	{
		msr_t msr;
		msr=rdmsr(0xc0010042);
		print_debug("end   msr fid, vid "); print_debug_hex32( msr.hi ); print_debug_hex32(msr.lo); print_debug("\r\n");

	}
#endif

	needs_reset |= optimize_link_coherent_ht();
	needs_reset |= optimize_link_incoherent_ht(sysinfo);
	needs_reset |= mcp55_early_setup_x();

	// fidvid change will issue one LDTSTOP and the HT change will be effective too
	if (needs_reset) {
		print_info("ht reset -\r\n");
	      	soft_reset();
	}

	allow_all_aps_stop(bsp_apicid);

	//It's the time to set ctrl in sysinfo now;
	fill_mem_ctrl(sysinfo->nodes, sysinfo->ctrl, spd_addr);

	enable_smbus();

	memreset_setup();

	//do we need apci timer, tsc...., only debug need it for better output
	/* all ap stopped? */
//	init_timer(); // Need to use TMICT to synconize FID/VID

	sdram_initialize(sysinfo->nodes, sysinfo->ctrl, sysinfo);

	post_cache_as_ram(); // bsp swtich stack to ram and copy sysinfo ram now

}

