#include <stdio.h>
#include <stdlib.h>
#include "pirq_routing.h"

static char *preamble[] = {
	"/* This file was generated by getpir.c, do not modify! \n   (but if you do, please run checkpir on it to verify)\n",
	" * Contains the IRQ Routing Table dumped directly from your memory, which BIOS sets up\n",
	" *\n",
	" * Documentation at : http://www.microsoft.com/hwdev/busbios/PCIIRQ.HTM\n*/\n\n",
	"#ifdef GETPIR\n",
	"#include \"pirq_routing.h\"\n",
	"#else\n"
	"#include <arch/pirq_routing.h>\n",
	"#endif\n\n"
	"const struct irq_routing_table intel_irq_routing_table = {\n",
	"\tPIRQ_SIGNATURE,  /* u32 signature */\n",
	"\tPIRQ_VERSION,    /* u16 version   */\n",
	0
};

void code_gen(char *filename, struct irq_routing_table *rt)
{
	char **code = preamble;
	struct irq_info *se_arr = (struct irq_info *) ((char *) rt + 32);
	int i, ts = (rt->size - 32) / 16;
	FILE *fpir;

	if ((fpir = fopen(filename, "w")) == NULL) {
		printf("Failed creating file!\n");
		exit(2);
	}

	while (*code)
		fprintf(fpir, "%s", *code++);

	fprintf(fpir, "\t32+16*%d,	 /* there can be total %d devices on the bus */\n",
		ts, ts);
	fprintf(fpir, "\t0x%02x,		 /* Where the interrupt router lies (bus) */\n",
		rt->rtr_bus);
	fprintf(fpir, "\t(0x%02x<<3)|0x%01x,   /* Where the interrupt router lies (dev) */\n",
		rt->rtr_devfn >> 3, rt->rtr_devfn & 7);
	fprintf(fpir, "\t%#x,		 /* IRQs devoted exclusively to PCI usage */\n",
		rt->exclusive_irqs);
	fprintf(fpir, "\t%#x,		 /* Vendor */\n", rt->rtr_vendor);
	fprintf(fpir, "\t%#x,		 /* Device */\n", rt->rtr_device);
	fprintf(fpir, "\t%#x,		 /* Crap (miniport) */\n",
		rt->miniport_data);
	fprintf(fpir, "\t{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* u8 rfu[11] */\n");
	fprintf(fpir, "\t%#x,         /*  u8 checksum , this hase to set to some value that would give 0 after the sum of all bytes for this structure (including checksum) */\n",
		rt->checksum);
	fprintf(fpir, "\t{\n");
	fprintf(fpir, "\t\t/* bus,     dev|fn,   {link, bitmap}, {link, bitmap}, {link, bitmap}, {link, bitmap},  slot, rfu */\n");
	for (i = 0; i < ts; i++) {
		fprintf(fpir, "\t\t{0x%02x,(0x%02x<<3)|0x%01x, {{0x%02x, 0x%04x}, {0x%02x, 0x%04x}, {0x%02x, 0x%04x}, {0x%02x, 0x0%04x}}, 0x%x, 0x%x},\n",
			(se_arr+i)->bus, (se_arr+i)->devfn >> 3,
			(se_arr+i)->devfn & 7, (se_arr+i)->irq[0].link,
			(se_arr+i)->irq[0].bitmap, (se_arr+i)->irq[1].link,
			(se_arr+i)->irq[1].bitmap, (se_arr+i)->irq[2].link,
			(se_arr+i)->irq[2].bitmap, (se_arr+i)->irq[3].link,
			(se_arr+i)->irq[3].bitmap, (se_arr+i)->slot,
			(se_arr+i)->rfu);
	}
	fprintf(fpir, "\t}\n");
	fprintf(fpir, "};\n");

	fclose(fpir);
}
