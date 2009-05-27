/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2009 One Laptop per Child, Association, Inc.
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

typedef struct __UMA_RAM_tag {
	u16 DramSize;
	u8 D0F3Val;
	u8 D1F0Val;
	u8 VgaPortVal;
} UMARAM;
#define UMARAM_512M	7
#define UMARAM_256M	6
#define UMARAM_128M	5
#define UMARAM_64M 	4
#define UMARAM_32M	3
#define UMARAM_16M	2
#define UMARAM_8M	1
#define UMARAM_0M	0


#define FB_512M		0
#define FB_256M		0x40
#define FB_128M		0x60
#define FB_64M		0x70
#define FB_32M		0x78
#define FB_16M		0x7c
#define FB_8M		0x7E
#define FB_4M		0x7F

#define VGA_PORT_512M	0x00
#define VGA_PORT_256M	0x80
#define VGA_PORT_128M	0xC0
#define VGA_PORT_64M	0xE0
#define VGA_PORT_32M	0xF0
#define VGA_PORT_16M	0xF8

static const UMARAM UMARamArr[] = {
	{0, UMARAM_0M, FB_4M, 0xFE},
	{8, UMARAM_8M, FB_8M, 0xFC},
	{16, UMARAM_16M, FB_16M, VGA_PORT_16M},
	{32, UMARAM_32M, FB_32M, VGA_PORT_32M},
	{64, UMARAM_64M, FB_64M, VGA_PORT_64M},
	{128, UMARAM_128M, FB_128M, VGA_PORT_128M},
	{256, UMARAM_256M, FB_256M, VGA_PORT_256M},
	{512, UMARAM_512M, FB_512M, VGA_PORT_512M},
	{0xffff, 0xff, 0xff, 0xFF}
};

void SetUMARam(void)
{
#if 1
	u8 ramregs[] = { 0x43, 0x42, 0x41, 0x40 };
	device_t vga_dev = PCI_DEV(0, 1, 0), d0f0_dev = PCI_DEV(0, 0, 0);
	u8 ByteVal, temp;
	UMARAM *pUMARamTable;
	u16 UmaSize;
	u8 SLD0F3Val, SLD1F0Val, VgaPortVal;
	u32 RamSize, SLBase, Tmp;
	u8 i;
	PRINT_DEBUG_MEM("Entering vx800 SetUMARam.\n");
	SLD0F3Val = 0;
	SLD1F0Val = 0;
	VgaPortVal = 0;


	ByteVal = pci_read_config8(MEMCTRL, 0xa1);
	ByteVal |= 0x80;
	pci_write_config8(MEMCTRL, 0xa1, ByteVal);

	//set VGA Timer
	pci_write_config8(MEMCTRL, 0xa2, 0xee);

	//set agp misc
	//GFX Data Delay to Sync with Clock
	pci_write_config8(MEMCTRL, 0xa4, 0x01);

	//page register life timer
	pci_write_config8(MEMCTRL, 0xa6, 0x76);

	//GMINT and GFX relatate
	//note Bit 3 VGA Enable
	pci_write_config8(MEMCTRL, 0xa7, 0x8c);
	// ByteVal = 0x4c;

	//GMINT Misc.1
	//pci_write_config8(MEMCTRL, 0xb0, 0x80);

	//pci_write_config8(MEMCTRL, 0xb1, 0xaa);

	//AGPCINT MISC
	//pci_write_config8(MEMCTRL, 0xb2, 0x82);
	//ByteVal = 0x8A;

	//GMINT MISC.2
	//disable read pass write
	pci_write_config8(MEMCTRL, 0xb3, 0x9A);

	//EPLL Register
	//pci_write_config8(MEMCTRL, 0xb4, 0x04);

	//enable CHA and CHB merge mode
	pci_write_config8(MEMCTRL, 0xde, 0x06);

	//if can get the value from setup interface, so get the value
	//else use the default value
	UmaSize = CONFIG_VIDEO_MB;

	for (pUMARamTable = UMARamArr; pUMARamTable->DramSize != 0xffff;
	     pUMARamTable++) {
		if (UmaSize == pUMARamTable->DramSize) {
			SLD0F3Val = pUMARamTable->D0F3Val;
			SLD1F0Val = pUMARamTable->D1F0Val;
			VgaPortVal = pUMARamTable->VgaPortVal;
		}
	}
	//set SL size
	//Fill in Fun3_RXA1[6:4] with the Frame Buffer size for the Integrated Graphic Device.
	ByteVal = pci_read_config8(MEMCTRL, 0xa1);
	ByteVal = (ByteVal & 0x8f) | (SLD0F3Val << 4);
	pci_write_config8(MEMCTRL, 0xa1, ByteVal);


//      vga_dev = dev_find_device(PCI_VENDOR_ID_VIA, PCI_DEVICE_ID_VIA_VX855_VGA, 0);

	//RxB2 may be for S.L. and RxB1 may be for L. L. 
	// It is different from Spec.
	ByteVal = SLD1F0Val;
	pci_write_config8(vga_dev, 0xb2, ByteVal);


	//set M1 size
	//ByteVal=pci_read_config8(MEMCTRL, 0xa3);
	//ByteVal = 0x02;
	//pci_write_config8(MEMCTRL, 0xa3, ByteVal);

	PRINT_DEBUG_MEM("UMA setting - 3\n");




	//Enable p2p  IO/mem
	ByteVal = 0x07;
	pci_write_config8(vga_dev, 0x04, ByteVal);




	//must set SL and MMIO base, or else when enable GFX memory space, system will hang
	//set S.L base
	Tmp = pci_read_config32(vga_dev, 0x10);
	Tmp = 0xfffffff8;
	pci_write_config32(vga_dev, 0x10, Tmp);
	Tmp = pci_read_config32(vga_dev, 0x10);
	Tmp = VIACONFIG_VGA_PCI_10;
	pci_write_config32(vga_dev, 0x10, Tmp);

	//set MMIO base
	Tmp = pci_read_config32(vga_dev, 0x14);
	Tmp = 0xfffffffC;
	pci_write_config32(vga_dev, 0x14, Tmp);
	Tmp = pci_read_config32(vga_dev, 0x14);
	Tmp = VIACONFIG_VGA_PCI_14;
	pci_write_config32(vga_dev, 0x14, Tmp);


//enable direct cpu frame buffer access
	i = pci_rawread_config8(PCI_RAWDEV(0, 0, 3), 0xa1);
	i = (i & 0xf0) | (VIACONFIG_VGA_PCI_10 >> 28);
	pci_rawwrite_config8(PCI_RAWDEV(0, 0, 3), 0xa1, i);
	pci_rawwrite_config8(PCI_RAWDEV(0, 0, 3), 0xa0, 0x01);


	//enable GFx memory space access control for S.L and mmio
	ByteVal = pci_read_config8(d0f0_dev, 0xD4);
	ByteVal |= 0x03;
	//ByteVal |= 0x01;
	pci_write_config8(d0f0_dev, 0xD4, ByteVal);


	//enable Base VGA 16 Bits Decode
	ByteVal = pci_read_config8(d0f0_dev, 0xfe);
	ByteVal |= 0x10;
	pci_write_config8(d0f0_dev, 0xfe, ByteVal);


	//disable CHB L.L
	//set VGA memory selection
	ByteVal = pci_read_config8(vga_dev, 0xb0);
	ByteVal &= 0xF8;
	//ByteVal |= 0x01;
	ByteVal |= 0x03;
	pci_write_config8(vga_dev, 0xb0, ByteVal);

	//set LL size

	//enable memory access to SL,MMIO,LL and IO to 3B0~3BB,3C0 ~3DF
	//ByteVal = 0x03;
	//pci_write_config8(d0f0_dev, 0xc0, ByteVal);

	//Turn on Graphic chip IO port port access
	ByteVal = inb(0x03C3);
	ByteVal |= 0x01;
	outb(ByteVal, 0x03C3);

	//Turn off Graphic chip Register protection
	outb(0x10, 0x03C4);

	ByteVal = inb(0x03C5);
	ByteVal |= 0x01;
	outb(ByteVal, 0x03C5);

	//set VGA memory Frequence
	//direct IO port 0x3DX to vga io space 0x3C2[0]
	ByteVal = inb(0x03CC);
	ByteVal |= 0x03;
	outb(ByteVal, 0x03C2);
	//  ByteVal=inb(0x03C2);
	//   ByteVal |= 0x01;
	//   outb(ByteVal,0x03C2);


#if 1				//bios porting guide has no this two defination:  3d  on 3d4/3d5 and  39 on 3c4/3c5
	//set frequence 0x3D5.3d[7:4]
	outb(0x3d, 0x03d4);

	temp = pci_read_config8(MEMCTRL, 0x90);
	temp = (u8) (temp & 0x07);
	ByteVal = inb(0x03d5);
	switch (temp) {
	case 0:		//DIMMFREQ_200:
		ByteVal = (u8) ((ByteVal & 0x0F) | 0x30);
		break;
	case 1:		//DIMMFREQ_266:
		ByteVal = (u8) ((ByteVal & 0x0F) | 0x40);
		break;
	case 3:		//DIMMFREQ_400:
		ByteVal = (u8) ((ByteVal & 0x0F) | 0x60);
		break;
	case 4:		//DIMMFREQ_533:
		ByteVal = (u8) ((ByteVal & 0x0F) | 0x70);
		break;
	case 5:		//DIMMFREQ_667:
		ByteVal = (u8) ((ByteVal & 0x0F) | 0x80);
		break;
	case 6:		//DIMMFREQ_800:
		ByteVal = (u8) ((ByteVal & 0x0F) | 0x90);
		break;
	default:
		ByteVal = (u8) ((ByteVal & 0x0F) | 0x70);
		break;
	}
	outb(ByteVal, 0x03d5);

	// Set frame buffer size 
	outb(0x39, 0x03c4);
	outb(1 << SLD0F3Val, 0x03c5);

#endif
	// Set S.L. size in GFX's register
	outb(0x68, 0x03c4);
	outb(VgaPortVal, 0x03c5);

	//  ECLK Selection (00:166Mhz, 01:185Mhz, 10:250Mhz, 11:275Mhz)
	// set 3C5.5A[0]=1, address maps to secondary resgiters
	outb(0x5a, 0x03c4);
	ByteVal = inb(0x03c5);
	ByteVal |= 0x01;
	outb(ByteVal, 0x03c5);

	// Set 3D5.4C[7:6] (00:166Mhz, 01:185Mhz, 10:250Mhz, 11:275Mhz)
	outb(0x4c, 0x03d4);
	ByteVal = inb(0x03d5);
	ByteVal = (ByteVal & 0x3F) | 0x80;
	outb(ByteVal, 0x03d5);

	// set 3C5.5A[0]=0, address maps to first resgiters
	outb(0x5a, 0x03c4);
	ByteVal = inb(0x03c5);
	ByteVal &= 0xFE;
	outb(ByteVal, 0x03c5);

	// Set S.L. Address in System Memory
	//calculate dram size
	for (RamSize = 0, i = 0; i < ARRAY_SIZE(ramregs); i++) {
		RamSize = pci_read_config8(MEMCTRL, ramregs[i]);
		if (RamSize != 0)
			break;
	}
	//calculate SL Base Address
	SLBase = (RamSize << 26) - (UmaSize << 20);


	outb(0x6D, 0x03c4);
	//SL Base[28:21] 
	outb((u8) ((SLBase >> 21) & 0xFF), 0x03c5);

	outb(0x6e, 0x03c4);
	//SL Base[36:29]
	outb((u8) ((SLBase >> 29) & 0xFF), 0x03c5);

	outb(0x6f, 0x03c4);
	outb(0x00, 0x03c5);

	// Set SVID high byte
	outb(0x36, 0x03c4);
	outb(0x11, 0x03c5);

	// Set SVID Low byte
	outb(0x35, 0x03c4);
	outb(0x06, 0x03c5);

	// Set SID high byte
	outb(0x38, 0x03c4);
	outb(0x51, 0x03c5);

	// Set SID Low byte
	outb(0x37, 0x03c4);
	outb(0x22, 0x03c5);

	//start : For enable snapshot mode control
	// program 3C5 for SNAPSHOT Mode control, set RxF3h=1Ah
	outb(0xf3, 0x03c4);
	ByteVal = inb(0x03c5);
	ByteVal = (ByteVal & 0xE5) | 0x1A;
	outb(ByteVal, 0x03c5);


	outb(0xf3, 0x03d4);
	ByteVal = inb(0x03d5);
	ByteVal = (ByteVal & 0xE5) | 0x1A;
	outb(ByteVal, 0x03d5);

	u8 table3c43c5[0x70] = {
		0x03, 0x01, 0x0F, 0x00, 0x06, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x78, 0x00, 0x00, 0x00, 0xBE, 0x20, 0x7F,
		0x60, 0x7F, 0x08, 0x31, 0xCC, 0x00, 0x01, 0x00,
		0x00, 0x18, 0x10, 0x00, 0x00, 0x00, 0x3D, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x04, 0xF3, 0xFF, 0xFC,
		0xF8, 0x0C, 0x00, 0x00, 0x40, 0x06, 0x11, 0x22,
		0x51, 0x10, 0x00, 0x01, 0x19, 0x0C, 0x00, 0xFF,
		0x38, 0x40, 0x30, 0xFF, 0x70, 0x8C, 0x85, 0x9D,
		0x80, 0x05, 0x54, 0x90, 0x03, 0x30, 0x00, 0x5F,
		0x1F, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00,
		0x06, 0xDF, 0x00, 0x20, 0x20, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x40, 0x20, 0x00, 0x20, 0x20,
		0xE0, 0x20, 0xD0, 0x3F, 0x00, 0xE0, 0x00, 0x00
	};
	u8 table3d43d5[0x88] = {
		0x7F, 0x63, 0x63, 0x83, 0x69, 0x19, 0x72, 0xE0,
		0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x58, 0x9C, 0x57, 0x90, 0x00, 0x57, 0x73, 0xE3,
		0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x0C, 0x00, 0x11, 0x06, 0x00, 0x20, 0x01, 0x34,
		0xEE, 0x74, 0x01, 0x01, 0x08, 0x84, 0x00, 0x00,
		0x00, 0xF3, 0x40, 0x90, 0x00, 0x00, 0x00, 0x01,
		0x00, 0x12, 0x00, 0x02, 0x00, 0x00, 0x10, 0x00,
		0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D,
		0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x9D, 0x9D, 0x10,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x9D, 0x9D, 0x9D,
		0x9D, 0x9D, 0x9D, 0x9D, 0x00, 0x9D, 0x1D, 0x00,
		0x00, 0x00, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D,
		0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D,
	};


	u8 table3c0space[0xc0] = {
		0x11, 0x00, 0x10, 0x01, 0x26, 0x3D, 0xFF, 0x00,
		0x10, 0x3F, 0x00, 0x00, 0x2F, 0x00, 0x22, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	};
#if 0
//for(i=0;i<0xc0;i++)
	for (i = 0; i < 0x40; i++)	//
	{
		outb(table3c0space[i], 0x03c0 + i);
	}


	for (i = 0; i < 0x70; i++) {
		outb(i, 0x03c4);
		outb(table3c43c5[i], 0x03c5);
	}
	for (i = 0; i < 0x88; i++) {
		outb(i, 0x03d4);
		outb(table3d43d5[i], 0x03d5);
	}
	outb(0x92, 0x03d4);
	outb(0x80, 0x03d5);

	outb(0xa3, 0x03d4);
	outb(0x00, 0x03d5);

	outb(0xe8, 0x03d4);
	outb(0x40, 0x03d5);
#endif
//3d4 3d freq
//IO Port / Index: 3X5.3D
//Scratch Pad Register 4

//    outb(0x39,0x03c4);//
	//outb(1 << SLD0F3Val ,0x03c5);
//
#endif

}