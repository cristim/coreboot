#define ASSEMBLY 1
#define __PRE_RAM__

#define QRANK_DIMM_SUPPORT 1

#if CONFIG_LOGICAL_CPUS==1
#define SET_NB_CFG_54 1
#endif

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
#include "lib/ramtest.c"

#include <cpu/amd/model_fxx_rev.h>

#include "northbridge/amd/amdk8/incoherent_ht.c"
#include "southbridge/nvidia/ck804/ck804_early_smbus.c"
#include "northbridge/amd/amdk8/raminit.h"
#include "cpu/amd/model_fxx/apic_timer.c"
#include "lib/delay.c"

#include "cpu/x86/lapic/boot_cpu.c"
#include "northbridge/amd/amdk8/reset_test.c"
#include "northbridge/amd/amdk8/debug.c"
#include "superio/winbond/w83627hf/w83627hf_early_serial.c"

#include "cpu/amd/mtrr/amd_earlymtrr.c"
#include "cpu/x86/bist.h"

#include "northbridge/amd/amdk8/setup_resource_map.c"

#define SERIAL_DEV PNP_DEV(0x2e, W83627HF_SP1)

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

#include "northbridge/amd/amdk8/raminit.c"
#include "northbridge/amd/amdk8/coherent_ht.c"
#include "lib/generic_sdram.c"

 /* tyan does not want the default */
#include "resourcemap.c"

#include "cpu/amd/dualcore/dualcore.c"

#define CK804_NUM 1
#include "southbridge/nvidia/ck804/ck804_early_setup_ss.h"
//set GPIO to input mode
#define CK804_MB_SETUP \
	RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+15, ~(0xff),((0<<4)|(0<<2)|(0<<0)),/* M8,GPIO16, PCIXA_PRSNT2_L*/ \
	RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+44, ~(0xff),((0<<4)|(0<<2)|(0<<0)),/* P5,GPIO45, PCIXA_PRSNT1_L*/ \
	RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+16, ~(0xff),((0<<4)|(0<<2)|(0<<0)),/* K4,GPIO17, PCIXB_PRSNT1_L*/ \
	RES_PORT_IO_8, SYSCTRL_IO_BASE + 0xc0+45, ~(0xff),((0<<4)|(0<<2)|(0<<0)),/* P7,GPIO46, PCIXB_PRSNT2_L*/ \

#include "southbridge/nvidia/ck804/ck804_early_setup_car.c"

#include "cpu/amd/car/copy_and_run.c"

#include "cpu/amd/car/post_cache_as_ram.c"

#include "cpu/amd/model_fxx/init_cpus.c"

#include "southbridge/nvidia/ck804/ck804_enable_rom.c"
#include "northbridge/amd/amdk8/early_ht.c"

static void sio_setup(void)
{
	uint32_t dword;
	uint8_t byte;

	byte = pci_read_config8(PCI_DEV(0, CK804_DEVN_BASE+1 , 0), 0x7b);
	byte |= 0x20;
	pci_write_config8(PCI_DEV(0, CK804_DEVN_BASE+1 , 0), 0x7b, byte);

	dword = pci_read_config32(PCI_DEV(0, CK804_DEVN_BASE+1 , 0), 0xa0);
	dword |= (1<<0);
	pci_write_config32(PCI_DEV(0, CK804_DEVN_BASE+1 , 0), 0xa0, dword);
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

	int needs_reset;
	unsigned bsp_apicid = 0;

	struct mem_controller ctrl[8];
	unsigned nodes;

	if (!cpu_init_detectedx && boot_cpu()) {
		/* Nothing special needs to be done to find bus 0 */
		/* Allow the HT devices to be found */

		enumerate_ht_chain();

		sio_setup();

		/* Setup the ck804 */
		ck804_enable_rom();
	}

	if (bist == 0) {
		bsp_apicid = init_cpus(cpu_init_detectedx);
	}

//	post_code(0x32);

	w83627hf_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
	uart_init();
	console_init();

	/* Halt if there was a built in self test failure */
	report_bist_failure(bist);

	setup_mb_resource_map();

	needs_reset = setup_coherent_ht_domain();

	wait_all_core0_started();
#if CONFIG_LOGICAL_CPUS==1
	// It is said that we should start core1 after all core0 launched
	start_other_cores();
	wait_all_other_cores_started(bsp_apicid);
#endif

	needs_reset |= ht_setup_chains_x();

	needs_reset |= ck804_early_setup_x();

	if (needs_reset) {
		printk_info("ht reset -\n");
		soft_reset();
	}

	allow_all_aps_stop(bsp_apicid);

	nodes = get_nodes();
	//It's the time to set ctrl now;
	fill_mem_ctrl(nodes, ctrl, spd_addr);

	enable_smbus();

	sdram_initialize(nodes, ctrl);

	post_cache_as_ram();
}
