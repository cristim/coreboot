/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2004 Linux Networx
 * (Written by Eric Biederman <ebiederman@lnxi.com> for Linux Networx)
 * Copyright (C) 2009 coresystems GmbH
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
#include <arch/pciconf.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>

/*
 * The only consumer of the return value of get_pbus() is ops_pci_bus().
 * ops_pci_bus() can handle being passed NULL and auto-picks working ops.
 */
static struct bus *get_pbus(device_t dev)
{
	struct bus *pbus = NULL;

	if (!dev)
		die("get_pbus: dev is NULL!\n");
	else
		pbus = dev->bus;

	while (pbus && pbus->dev && !ops_pci_bus(pbus)) {
		if (pbus == pbus->dev->bus) {
			printk(BIOS_ALERT, "%s in endless loop looking for a "
			       "parent bus with ops_pci_bus for %s, breaking "
			       "out.\n", __func__, dev_path(dev));
			break;
		}
		pbus = pbus->dev->bus;
	}

	if (!pbus || !pbus->dev || !pbus->dev->ops
	    || !pbus->dev->ops->ops_pci_bus) {
		/* This can happen before the device tree is fully set up. */

		// printk(BIOS_EMERG, "%s: Cannot find PCI bus operations.\n",
		// dev_path(dev));

		pbus = NULL;
	}

	return pbus;
}

u8 pci_read_config8(device_t dev, unsigned int where)
{
	struct bus *pbus = get_pbus(dev);
	return ops_pci_bus(pbus)->read8(pbus, dev->bus->secondary,
					dev->path.pci.devfn, where);
}

u16 pci_read_config16(device_t dev, unsigned int where)
{
	struct bus *pbus = get_pbus(dev);
	return ops_pci_bus(pbus)->read16(pbus, dev->bus->secondary,
					 dev->path.pci.devfn, where);
}

u32 pci_read_config32(device_t dev, unsigned int where)
{
	struct bus *pbus = get_pbus(dev);
	return ops_pci_bus(pbus)->read32(pbus, dev->bus->secondary,
					 dev->path.pci.devfn, where);
}

void pci_write_config8(device_t dev, unsigned int where, u8 val)
{
	struct bus *pbus = get_pbus(dev);
	ops_pci_bus(pbus)->write8(pbus, dev->bus->secondary,
				  dev->path.pci.devfn, where, val);
}

void pci_write_config16(device_t dev, unsigned int where, u16 val)
{
	struct bus *pbus = get_pbus(dev);
	ops_pci_bus(pbus)->write16(pbus, dev->bus->secondary,
				   dev->path.pci.devfn, where, val);
}

void pci_write_config32(device_t dev, unsigned int where, u32 val)
{
	struct bus *pbus = get_pbus(dev);
	ops_pci_bus(pbus)->write32(pbus, dev->bus->secondary,
				   dev->path.pci.devfn, where, val);
}

#if CONFIG_MMCONF_SUPPORT
u8 pci_mmio_read_config8(device_t dev, unsigned int where)
{
	struct bus *pbus = get_pbus(dev);
	return pci_ops_mmconf.read8(pbus, dev->bus->secondary,
				    dev->path.pci.devfn, where);
}

u16 pci_mmio_read_config16(device_t dev, unsigned int where)
{
	struct bus *pbus = get_pbus(dev);
	return pci_ops_mmconf.read16(pbus, dev->bus->secondary,
				     dev->path.pci.devfn, where);
}

u32 pci_mmio_read_config32(device_t dev, unsigned int where)
{
	struct bus *pbus = get_pbus(dev);
	return pci_ops_mmconf.read32(pbus, dev->bus->secondary,
				     dev->path.pci.devfn, where);
}

void pci_mmio_write_config8(device_t dev, unsigned int where, u8 val)
{
	struct bus *pbus = get_pbus(dev);
	pci_ops_mmconf.write8(pbus, dev->bus->secondary, dev->path.pci.devfn,
			      where, val);
}

void pci_mmio_write_config16(device_t dev, unsigned int where, u16 val)
{
	struct bus *pbus = get_pbus(dev);
	pci_ops_mmconf.write16(pbus, dev->bus->secondary, dev->path.pci.devfn,
			       where, val);
}

void pci_mmio_write_config32(device_t dev, unsigned int where, u32 val)
{
	struct bus *pbus = get_pbus(dev);
	pci_ops_mmconf.write32(pbus, dev->bus->secondary, dev->path.pci.devfn,
			       where, val);
}

#endif
