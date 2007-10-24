#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <device/pci_ids.h>

#include <pc80/mc146818rtc.h>

#include "vt8231.h"
#include "chip.h"

/* PIRQ init
 */
void pci_assign_irqs(unsigned bus, unsigned slot, const unsigned char pIntAtoD[4]);
static const unsigned char southbridgeIrqs[4] = { 11, 5, 10, 12 };
static const unsigned char enetIrqs[4] = { 11, 5, 10, 12 };
static const unsigned char slotIrqs[4] = { 5, 10, 12, 11 };

/*
	Our IDSEL mappings are as follows
	PCI slot is AD31          (device 15) (00:14.0)
	Southbridge is AD28       (device 12) (00:11.0)
*/
static void pci_routing_fixup(struct device *dev)
{

	printk_info("%s: dev is %p\n", __FUNCTION__, dev);
	if (dev) {
		/* initialize PCI interupts - these assignments depend
		   on the PCB routing of PINTA-D 

		   PINTA = IRQ11
		   PINTB = IRQ5
		   PINTC = IRQ10
		   PINTD = IRQ12
		*/
		pci_write_config8(dev, 0x55, 0xb0);
		pci_write_config8(dev, 0x56, 0xa5);
		pci_write_config8(dev, 0x57, 0xc0);
	}

	// Standard southbridge components
	printk_info("setting southbridge\n");
	pci_assign_irqs(0, 0x11, southbridgeIrqs);

	// Ethernet built into southbridge
	printk_info("setting ethernet\n");
	pci_assign_irqs(0, 0x12, enetIrqs);

	// PCI slot
	printk_info("setting pci slot\n");
	pci_assign_irqs(0, 0x14, slotIrqs);
	printk_info("%s: DONE\n", __FUNCTION__);
}

static void vt8231_init(struct device *dev)
{
	unsigned char enables;
	struct southbridge_via_vt8231_config *conf = dev->chip_info;

	printk_debug("vt8231 init\n");

	// enable the internal I/O decode
	enables = pci_read_config8(dev, 0x6C);
	enables |= 0x80;
	pci_write_config8(dev, 0x6C, enables);
	
	// Map 4MB of FLASH into the address space
	pci_write_config8(dev, 0x41, 0x7f);
	
	// Set bit 6 of 0x40, because Award does it (IO recovery time)
	// IMPORTANT FIX - EISA 0x4d0 decoding must be on so that PCI 
	// interrupts can be properly marked as level triggered.
	enables = pci_read_config8(dev, 0x40);
	pci_write_config8(dev, 0x40, enables);
	
	// Set 0x42 to 0xf0 to match Award bios
	enables = pci_read_config8(dev, 0x42);
	enables |= 0xf0;
	pci_write_config8(dev, 0x42, enables);
	
	// Set bit 3 of 0x4a, to match award (dummy pci request)
	enables = pci_read_config8(dev, 0x4a);
	enables |= 0x08;
	pci_write_config8(dev, 0x4a, enables);
	
	// Set bit 3 of 0x4f to match award (use INIT# as cpu reset)
	enables = pci_read_config8(dev, 0x4f);
	enables |= 0x08;
	pci_write_config8(dev, 0x4f, enables);
	
	// Set 0x58 to 0x03 to match Award
	pci_write_config8(dev, 0x58, 0x03);
	
	// enable the ethernet/RTC
	if (dev) {
		enables = pci_read_config8(dev, 0x51);
		enables |= 0x18; 
		pci_write_config8(dev, 0x51, enables);
	}

	// enable IDE, since Linux won't do it.
	// First do some more things to devfn (17,0)
	// note: this should already be cleared, according to the book. 
	enables = pci_read_config8(dev, 0x50);
	printk_debug("IDE enable in reg. 50 is 0x%x\n", enables);
	enables &= ~8; // need manifest constant here!
	printk_debug("set IDE reg. 50 to 0x%x\n", enables);
	pci_write_config8(dev, 0x50, enables);
	
	// set default interrupt values (IDE)
	enables = pci_read_config8(dev, 0x4c);
	printk_debug("IRQs in reg. 4c are 0x%x\n", enables & 0xf);
	// clear out whatever was there. 
	enables &= ~0xf;
	enables |= 4;
	printk_debug("setting reg. 4c to 0x%x\n", enables);
	pci_write_config8(dev, 0x4c, enables);
	
	// set up the serial port interrupts. 
	// com2 to 3, com1 to 4
	pci_write_config8(dev, 0x46, 0x04);
	pci_write_config8(dev, 0x47, 0x03);
	pci_write_config8(dev, 0x6e, 0x98);

	/* set up isa bus -- i/o recovery time, rom write enable, extend-ale */
	pci_write_config8(dev, 0x40, 0x54);
	//ethernet_fixup();
	
	// Start the rtc
	rtc_init(0);
}

static void southbridge_init(struct device *dev)
{
	vt8231_init(dev);
	pci_routing_fixup(dev);
}

static struct device_operations vt8231_lpc_ops = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init		  = &southbridge_init,
	.scan_bus	  = scan_static_bus,
	.enable           = 0,
	.ops_pci          = 0,
};

static const struct pci_driver lpc_driver __pci_driver = {
	.ops	= &vt8231_lpc_ops,
	.vendor = PCI_VENDOR_ID_VIA,
	.device = PCI_DEVICE_ID_VIA_8231,
};
