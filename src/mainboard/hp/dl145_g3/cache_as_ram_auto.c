/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2006 Tyan
 * Copyright (C) 2006 AMD
 * Written by Yinghai Lu <yinghailu@gmail.com> for Tyan and AMD.
 *
 * Copyright (C) 2007 University of Mannheim
 * Written by Philipp Degler <pdegler@rumms.uni-mannheim.de> for University of Mannheim
 * Copyright (C) 2009 University of Heidelberg
 * Written by Mondrian Nuessle <nuessle@uni-heidelberg.de> for University of Heidelberg
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

#define ASSEMBLY 1
#define __ROMCC__

#define RAMINIT_SYSINFO 1

#define K8_ALLOCATE_IO_RANGE 1
//#define K8_SCAN_PCI_BUS 1

#define QRANK_DIMM_SUPPORT 1

#if CONFIG_LOGICAL_CPUS==1
#define SET_NB_CFG_54 1
#endif

//used by init_cpus and fidvid
#define K8_SET_FIDVID 1
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


#if CONFIG_USE_FAILOVER_IMAGE==0
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "ram/ramtest.c"

#include <cpu/amd/model_fxx_rev.h>

#include "southbridge/broadcom/bcm5785/bcm5785_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"

#endif

#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"

#include "superio/serverengines/pilot/pilot_early_serial.c"
#include "superio/serverengines/pilot/pilot_early_init.c"
#include "superio/nsc/pc87417/pc87417_early_serial.c"


#if CONFIG_USE_FAILOVER_IMAGE==0

#include "cpu/x86/bist.h"

#include "northbridge/amd/amdk8/debug.c"

#include "cpu/amd/mtrr/amd_earlymtrr.c"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#define SERIAL_DEV PNP_DEV(0x2e, PILOT_SP1)
#define RTC_DEV PNP_DEV(0x4e, PC87417_RTC)

#include "southbridge/broadcom/bcm5785/bcm5785_early_setup.c"

static void memreset_setup(void)
{
}

static void memreset(int controllers, const struct mem_controller *ctrl)
{
}

static inline void activate_spd_rom(const struct mem_controller *ctrl)
{
#define SMBUS_SWITCH1 0x70
#define SMBUS_SWITCH2 0x72
	 unsigned device = (ctrl->channel0[0]) >> 8;
	 smbus_send_byte(SMBUS_SWITCH1, device & 0x0f);
	 smbus_send_byte(SMBUS_SWITCH2, (device >> 4) & 0x0f );
}

static inline int spd_read_byte(unsigned device, unsigned address)
{
	 return smbus_read_byte(device, address);
}

#include "northbridge/amd/amdk8/amdk8_f.h"
#include "northbridge/amd/amdk8/coherent_ht.c"

#include "northbridge/amd/amdk8/incoherent_ht.c"

#include "northbridge/amd/amdk8/raminit_f.c"

#include "sdram/generic_sdram.c"

//#include "resourcemap.c"

#include "cpu/amd/dualcore/dualcore.c"

//first node
#define DIMM0 0x50
#define DIMM1 0x51
#define DIMM2 0x52
#define DIMM3 0x53
//second node
#define DIMM4 0x54
#define DIMM5 0x55
#define DIMM6 0x56
#define DIMM7 0x57


#include "cpu/amd/car/copy_and_run.c"

#include "cpu/amd/car/post_cache_as_ram.c"

#include "cpu/amd/model_fxx/init_cpus.c"

#include "cpu/amd/model_fxx/fidvid.c"

#endif

#if ((CONFIG_HAVE_FAILOVER_BOOT==1) && (CONFIG_USE_FAILOVER_IMAGE == 1)) || ((CONFIG_HAVE_FAILOVER_BOOT==0) && (CONFIG_USE_FALLBACK_IMAGE == 1))

#include "northbridge/amd/amdk8/early_ht.c"

#if 0
#include "ipmi.c"

static void setup_early_ipmi_serial()
{
	unsigned char result;
	char channel_access[]={0x06<<2,0x40,0x04,0x80,0x05};
	char serialmodem_conf[]={0x0c<<2,0x10,0x04,0x08,0x00,0x0f};
	char serial_mux1[]={0x0c<<2,0x12,0x04,0x06};
	char serial_mux2[]={0x0c<<2,0x12,0x04,0x03};
	char serial_mux3[]={0x0c<<2,0x12,0x04,0x07};

//	earlydbg(0x0d);
	//set channel access system only
	ipmi_request(5,channel_access);
//	earlydbg(result);
/*
	//Set serial/modem config
	result=ipmi_request(6,serialmodem_conf);
	earlydbg(result);

	//Set serial mux 1
	result=ipmi_request(4,serial_mux1);
	earlydbg(result);

	//Set serial mux 2
	result=ipmi_request(4,serial_mux2);
	earlydbg(result);

	//Set serial mux 3
	result=ipmi_request(4,serial_mux3);
	earlydbg(result);
*/
//	earlydbg(0x0e);

}
#endif


void failover_process(unsigned long bist, unsigned long cpu_init_detectedx)
{
	 /* Is this a cpu only reset? Is this a secondary cpu? */
	 if ((cpu_init_detectedx) || (!boot_cpu())) {
		if (last_boot_normal()) { // RTC already inited
			goto normal_image; //normal_image;
		} else {
			goto fallback_image;
		}
	 }

	 /* Nothing special needs to be done to find bus 0 */
	 /* Allow the HT devices to be found */

	 enumerate_ht_chain();
	 bcm5785_enable_rom();
	 bcm5785_enable_lpc();
	 //enable RTC
	pc87417_enable_dev(RTC_DEV);

	 /* Is this a deliberate reset by the bios */

	 if (bios_reset_detected() && last_boot_normal()) {
		goto normal_image;
	 }
	 /* This is the primary cpu how should I boot? */
	 else if (do_normal_boot()) {
		goto normal_image;
	 }
	 else {
		goto fallback_image;
	 }
 normal_image:
	 __asm__ volatile ("jmp __normal_image"
		: /* outputs */
		: "a" (bist), "b" (cpu_init_detectedx) /* inputs */
		);

 fallback_image:
#if CONFIG_HAVE_FAILOVER_BOOT==1
	__asm__ volatile ("jmp __fallback_image"
		: /* outputs */
		: "a" (bist), "b" (cpu_init_detectedx) /* inputs */
		)
#endif
	 ;

}
#endif

void real_main(unsigned long bist, unsigned long cpu_init_detectedx);

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
#if CONFIG_HAVE_FAILOVER_BOOT==1
    #if CONFIG_USE_FAILOVER_IMAGE==1
	failover_process(bist, cpu_init_detectedx);
    #else
	real_main(bist, cpu_init_detectedx);
    #endif
#else
    #if CONFIG_USE_FALLBACK_IMAGE == 1
	failover_process(bist, cpu_init_detectedx);
    #endif
	real_main(bist, cpu_init_detectedx);
#endif
}

#if CONFIG_USE_FAILOVER_IMAGE==0

void real_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
	static const uint16_t spd_addr[] = {
		//first node
		 DIMM0, DIMM2, 0, 0,
		 DIMM1, DIMM3, 0, 0,
#if CONFIG_MAX_PHYSICAL_CPUS > 1
		//second node
		DIMM4, DIMM6, 0, 0,
		DIMM5, DIMM7, 0, 0,
#endif

	};

	struct sys_info *sysinfo = (CONFIG_DCACHE_RAM_BASE + CONFIG_DCACHE_RAM_SIZE - CONFIG_DCACHE_RAM_GLOBAL_VAR_SIZE);

	 int needs_reset;
	 unsigned bsp_apicid = 0;


	 if (bist == 0) {
		 bsp_apicid = init_cpus(cpu_init_detectedx, sysinfo);
	 }

	pilot_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);

	//setup_mp_resource_map();

	uart_init();

	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);


	console_init();
//	setup_early_ipmi_serial();
	pilot_early_init(SERIAL_DEV); //config port is being taken from SERIAL_DEV
	print_debug("*sysinfo range: ["); print_debug_hex32(sysinfo); print_debug(","); print_debug_hex32((unsigned long)sysinfo+sizeof(struct sys_info)); print_debug(")\r\n");

	print_debug("bsp_apicid="); print_debug_hex8(bsp_apicid); print_debug("\r\n");

#if CONFIG_MEM_TRAIN_SEQ == 1
	set_sysinfo_in_ram(0); // in BSP so could hold all ap until sysinfo is in ram
#endif
	setup_coherent_ht_domain();

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
	bcm5785_early_setup();

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

	needs_reset = optimize_link_coherent_ht();
	needs_reset |= optimize_link_incoherent_ht(sysinfo);

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

	post_cache_as_ram();

}

#endif
