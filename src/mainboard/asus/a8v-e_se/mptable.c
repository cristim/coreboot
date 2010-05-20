/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Rudolf Marek <r.marek@assembler.cz>
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

#include <string.h>
#include <stdint.h>
#include <arch/smp/mpspec.h>
#include <../../../southbridge/via/vt8237r/vt8237r.h>
#include <../../../southbridge/via/k8t890/k8t890.h>

static void *smp_write_config_table(void *v)
{
	static const char sig[4] = "PCMP";
	static const char oem[8] = "COREBOOT";
	static const char productid[12] = "A8V-E SE    ";
	struct mp_config_table *mc;
	int bus_isa = 42;

	mc = (void *)(((char *)v) + SMP_FLOATING_TABLE_LEN);
	memset(mc, 0, sizeof(*mc));

	memcpy(mc->mpc_signature, sig, sizeof(sig));
	mc->mpc_length = sizeof(*mc); /* Initially just the header. */
	mc->mpc_spec = 0x04;
	mc->mpc_checksum = 0; /* Not yet computed. */
	memcpy(mc->mpc_oem, oem, sizeof(oem));
	memcpy(mc->mpc_productid, productid, sizeof(productid));
	mc->mpc_oemptr = 0;
	mc->mpc_oemsize = 0;
	mc->mpc_entry_count = 0; /* No entries yet. */
	mc->mpc_lapic = LAPIC_ADDR;
	mc->mpe_length = 0;
	mc->mpe_checksum = 0;
	mc->reserved = 0;

	smp_write_processors(mc);


	/* Bus:		Bus ID	Type */
	smp_write_bus(mc, 0, "PCI   ");
	smp_write_bus(mc, 1, "PCI   ");
	smp_write_bus(mc, 2, "PCI   ");
	smp_write_bus(mc, 3, "PCI   ");
	smp_write_bus(mc, 4, "PCI   ");
	smp_write_bus(mc, 5, "PCI   ");
	smp_write_bus(mc, 6, "PCI   ");
	smp_write_bus(mc, bus_isa, "ISA   ");

	/* I/O APICs:	APIC ID	Version	State		Address */
	smp_write_ioapic(mc, VT8237R_APIC_ID, 0x20, VT8237R_APIC_BASE);
	smp_write_ioapic(mc, K8T890_APIC_ID, 0x20, K8T890_APIC_BASE);

	mptable_add_isa_interrupts(mc, bus_isa, VT8237R_APIC_ID, 0);

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xb << 2) | 0, VT8237R_APIC_ID, 0x10); //IRQ16
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xb << 2) | 1, VT8237R_APIC_ID, 0x11); //IRQ17
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xb << 2) | 2, VT8237R_APIC_ID, 0x12); //IRQ18
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xb << 2) | 3, VT8237R_APIC_ID, 0x13); //IRQ19

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xc << 2) | 0, VT8237R_APIC_ID, 0x11); //IRQ17
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xc << 2) | 1, VT8237R_APIC_ID, 0x12); //IRQ18
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xc << 2) | 2, VT8237R_APIC_ID, 0x13); //IRQ19
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xc << 2) | 3, VT8237R_APIC_ID, 0x10); //IRQ16

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xd << 2) | 0, VT8237R_APIC_ID, 0x12); //IRQ18
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xd << 2) | 1, VT8237R_APIC_ID, 0x13); //IRQ19
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xd << 2) | 2, VT8237R_APIC_ID, 0x10); //IRQ16
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xd << 2) | 3, VT8237R_APIC_ID, 0x11); //IRQ17

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xf << 2) | 0, VT8237R_APIC_ID, 0x14);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0xf << 2) | 1, VT8237R_APIC_ID, 0x14);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x10 << 2) | 0, VT8237R_APIC_ID, 0x15);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x10 << 2) | 1, VT8237R_APIC_ID, 0x15);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x10 << 2) | 2, VT8237R_APIC_ID, 0x15);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x11 << 2) | 2, VT8237R_APIC_ID, 0x16);

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x2 << 2) | 0, K8T890_APIC_ID, 0x3);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x2 << 2) | 1, K8T890_APIC_ID, 0x3);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x2 << 2) | 2, K8T890_APIC_ID, 0x3);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x2 << 2) | 3, K8T890_APIC_ID, 0x3);

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x3 << 2) | 0, K8T890_APIC_ID, 0x7);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x3 << 2) | 1, K8T890_APIC_ID, 0xb);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x3 << 2) | 2, K8T890_APIC_ID, 0xf);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x0,  (0x3 << 2) | 3, K8T890_APIC_ID, 0x13);

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x2,  (0x00 << 2) | 0, K8T890_APIC_ID, 0x0);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x2,  (0x00 << 2) | 1, K8T890_APIC_ID, 0x1);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x2,  (0x00 << 2) | 2, K8T890_APIC_ID, 0x2);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x2,  (0x00 << 2) | 3, K8T890_APIC_ID, 0x3);

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x3,  (0x00 << 2) | 0, K8T890_APIC_ID, 0x4);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x3,  (0x00 << 2) | 1, K8T890_APIC_ID, 0x5);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x3,  (0x00 << 2) | 2, K8T890_APIC_ID, 0x6);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x3,  (0x00 << 2) | 3, K8T890_APIC_ID, 0x7);

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x4,  (0x00 << 2) | 0, K8T890_APIC_ID, 0x8);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x4,  (0x00 << 2) | 1, K8T890_APIC_ID, 0x9);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x4,  (0x00 << 2) | 2, K8T890_APIC_ID, 0xa);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x4,  (0x00 << 2) | 3, K8T890_APIC_ID, 0xb);

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x5,  (0x00 << 2) | 0, K8T890_APIC_ID, 0xc);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x5,  (0x00 << 2) | 1, K8T890_APIC_ID, 0xd);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x5,  (0x00 << 2) | 2, K8T890_APIC_ID, 0xe);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x5,  (0x00 << 2) | 3, K8T890_APIC_ID, 0xf);

	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x6,  (0x00 << 2) | 0, K8T890_APIC_ID, 0x10);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x6,  (0x00 << 2) | 1, K8T890_APIC_ID, 0x11);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x6,  (0x00 << 2) | 2, K8T890_APIC_ID, 0x12);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, 0x6,  (0x00 << 2) | 3, K8T890_APIC_ID, 0x13);

	/* Local Ints:	Type	Polarity    Trigger	Bus ID	 IRQ	APIC ID	PIN# */
	smp_write_intsrc(mc, mp_ExtINT,	MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH, bus_isa, 0x0, MP_APIC_ALL, 0x0);
	smp_write_intsrc(mc, mp_NMI,	MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH, bus_isa, 0x0, MP_APIC_ALL, 0x1);
	/* There is no extension information... */

	/* Compute the checksums. */
	mc->mpe_checksum = smp_compute_checksum(smp_next_mpc_entry(mc),
						mc->mpe_length);
	mc->mpc_checksum = smp_compute_checksum(mc, mc->mpc_length);

	return smp_next_mpe_entry(mc);
}

unsigned long write_smp_table(unsigned long addr)
{
	void *v;
	v = smp_write_floating_table(addr);
	return (unsigned long)smp_write_config_table(v);
}
