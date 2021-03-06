##
## This file is part of the coreboot project.
##
## Copyright (C) 2011 secunet Security Networks AG
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; version 2 of the License.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
##

#######################################################################
# misleadingly named, this is the coreboot version
REV=-r$(shell if [ -d $(top)/.svn -a -f "`which svnversion`" ]; then svnversion $(top); else if [ -d $(top)/.git -a -f "`which git`" ]; then git --git-dir=/$(top)/.git log|grep git-svn-id|cut -f 2 -d@|cut -f 1 -d' '|sort -g|tail -1; fi; fi)
export KERNELVERSION      := 4.0$(REV)

#######################################################################
# Basic component discovery
ARCHDIR-$(CONFIG_ARCH_X86) := x86
MAINBOARDDIR=$(call strip_quotes,$(CONFIG_MAINBOARD_DIR))
export MAINBOARDDIR

#######################################################################
# root rule to resolve if in build mode (ie. configuration exists)
real-target: $(obj)/config.h coreboot
coreboot: $(obj)/coreboot.rom

#######################################################################
# our phony targets
PHONY+= clean-abuild coreboot

#######################################################################
# root source directories of coreboot
subdirs-y := src/lib src/boot src/console src/devices src/ec src/southbridge src/northbridge src/superio src/drivers src/cpu
subdirs-y += util/cbfstool util/sconfig
subdirs-y += src/arch/$(ARCHDIR-y)
subdirs-y += src/mainboard/$(MAINBOARDDIR)

subdirs-$(CONFIG_ARCH_X86) += src/pc80

#######################################################################
# Add source classes and their build options
classes-y := ramstage romstage driver smm

romstage-c-ccopts:=-D__PRE_RAM__
romstage-S-ccopts:=-D__PRE_RAM__
smm-c-ccopts:=-D__SMM__
smm-S-ccopts:=-D__SMM__

ramstage-c-deps:=$$(OPTION_TABLE_H)
romstage-c-deps:=$$(OPTION_TABLE_H)

#######################################################################
# Add handler to compile ACPI's ASL
define ramstage-objs_asl_template
$(obj)/$(1).ramstage.o: src/$(1).asl $(obj)/config.h
	@printf "    IASL       $$(subst $(top)/,,$$(@))\n"
	$(CC) -x assembler-with-cpp -E -MMD -MT $$(@) -D__ACPI__ -P -include $(abspath $(obj)/config.h) -I$(src) -I$(src)/mainboard/$(MAINBOARDDIR) $$< -o $$(basename $$@).asl
	cd $$(dir $$@); iasl -p $$(notdir $$@) -tc $$(notdir $$(basename $$@)).asl
	mv $$(basename $$@).hex $$(basename $$@).c
	$(CC) $$(CFLAGS) $$(if $$(subst dsdt,,$$(basename $$(notdir $(1)))), -DAmlCode=AmlCode_$$(basename $$(notdir $(1)))) -c -o $$@ $$(basename $$@).c
	# keep %.o: %.c rule from catching the temporary .c file after a make clean
	mv $$(basename $$@).c $$(basename $$@).hex
endef

#######################################################################
# Add handler for arbitrary files in CBFS
$(call add-special-class,cbfs-files)
cbfs-files-handler= \
		$(if $(wildcard $(1)$($(2)-file)), \
			$(eval tmp-cbfs-file:= $(wildcard $(1)$($(2)-file))), \
			$(eval tmp-cbfs-file:= $($(2)-file))) \
		$(eval cbfs-files += $(tmp-cbfs-file)|$(2)|$($(2)-type)|$($(2)-position)) \
		$(eval $(2)-name:=) \
		$(eval $(2)-type:=) \
		$(eval $(2)-position:=)

#######################################################################
# a variety of flags for our build
CBFS_COMPRESS_FLAG:=
ifeq ($(CONFIG_COMPRESS_RAMSTAGE),y)
CBFS_COMPRESS_FLAG:=l
endif

CBFS_PAYLOAD_COMPRESS_FLAG:=
CBFS_PAYLOAD_COMPRESS_NAME:=none
ifeq ($(CONFIG_COMPRESSED_PAYLOAD_LZMA),y)
CBFS_PAYLOAD_COMPRESS_FLAG:=l
CBFS_PAYLOAD_COMPRESS_NAME:=LZMA
endif

ifneq ($(CONFIG_LOCALVERSION),"")
COREBOOT_EXTRA_VERSION := -$(call strip_quotes,$(CONFIG_LOCALVERSION))
endif

INCLUDES := -Isrc -Isrc/include -I$(obj) -Isrc/arch/$(ARCHDIR-y)/include
INCLUDES += -Isrc/devices/oprom/include
# abspath is a workaround for romcc
INCLUDES += -include $(abspath $(obj)/config.h)

CFLAGS = $(INCLUDES) -Os -pipe -g -nostdinc
CFLAGS += -nostdlib -Wall -Wundef -Wstrict-prototypes -Wmissing-prototypes
CFLAGS += -Wwrite-strings -Wredundant-decls -Wno-trigraphs
CFLAGS += -Wstrict-aliasing -Wshadow
ifeq ($(CONFIG_WARNINGS_ARE_ERRORS),y)
CFLAGS += -Werror
endif
CFLAGS += -fno-common -ffreestanding -fno-builtin -fomit-frame-pointer

additional-dirs := $(objutil)/cbfstool $(objutil)/romcc $(objutil)/options

#######################################################################
# generate build support files
$(obj)/build.h: .xcompile
	@printf "    GEN        build.h\n"
	rm -f $(obj)/build.h
	printf "/* build system definitions (autogenerated) */\n" > $(obj)/build.ht
	printf "#ifndef __BUILD_H\n" >> $(obj)/build.ht
	printf "#define __BUILD_H\n\n" >> $(obj)/build.ht
	printf "#define COREBOOT_VERSION \"$(KERNELVERSION)\"\n" >> $(obj)/build.ht
	printf "#define COREBOOT_EXTRA_VERSION \"$(COREBOOT_EXTRA_VERSION)\"\n" >> $(obj)/build.ht
	printf "#define COREBOOT_BUILD \"`LANG= date`\"\n" >> $(obj)/build.ht
	printf "\n" >> $(obj)/build.ht
	printf "#define COREBOOT_COMPILER \"$(shell LANG= $(CC) --version | head -n1)\"\n" >> $(obj)/build.ht
	printf "#define COREBOOT_ASSEMBLER \"$(shell LANG= $(AS) --version | head -n1)\"\n" >> $(obj)/build.ht
	printf "#define COREBOOT_LINKER \"$(shell LANG= $(LD) --version | head -n1)\"\n" >> $(obj)/build.ht
	printf "#define COREBOOT_COMPILE_TIME \"`LANG= date +%T`\"\n" >> $(obj)/build.ht
	printf "#define COREBOOT_COMPILE_BY \"$(subst \,@,$(shell PATH=$$PATH:/usr/ucb whoami))\"\n" >> $(obj)/build.ht
	printf "#define COREBOOT_COMPILE_HOST \"$(shell hostname -s 2>/dev/null)\"\n" >> $(obj)/build.ht
	printf "#define COREBOOT_COMPILE_DOMAIN \"$(shell test `uname -s` = "Linux" && dnsdomainname || domainname 2>/dev/null)\"\n" >> $(obj)/build.ht
	printf "#endif\n" >> $(obj)/build.ht
	mv $(obj)/build.ht $(obj)/build.h

$(obj)/ldoptions: $(obj)/config.h
	awk '/^#define ([^"])* ([^"])*$$/ {gsub("\\r","",$$3); print $$2 " = " $$3 ";";}' $< > $@

#######################################################################
# Build the tools
CBFSTOOL:=$(obj)/cbfstool

$(CBFSTOOL): $(objutil)/cbfstool/cbfstool
	cp $< $@

_WINCHECK=$(shell uname -o 2> /dev/null)
STACK=
ifeq ($(_WINCHECK),Msys)
	STACK=-Wl,--stack,16384000
endif
ifeq ($(_WINCHECK),Cygwin)
	STACK=-Wl,--stack,16384000
endif

ROMCC:= $(objutil)/romcc/romcc
$(ROMCC): $(top)/util/romcc/romcc.c
	@printf "    HOSTCC     $(subst $(obj)/,,$(@)) (this may take a while)\n"
	@# Note: Adding -O2 here might cause problems. For details see:
	@# http://www.coreboot.org/pipermail/coreboot/2010-February/055825.html
	$(HOSTCC) -g $(STACK) -Wall -o $@ $<

#######################################################################
# needed objects that every mainboard uses
# Creation of these is architecture and mainboard independent
$(obj)/mainboard/$(MAINBOARDDIR)/static.c: $(src)/mainboard/$(MAINBOARDDIR)/devicetree.cb  $(objutil)/sconfig/sconfig
	@printf "    SCONFIG    $(subst $(src)/,,$(<))\n"
	mkdir -p $(obj)/mainboard/$(MAINBOARDDIR)
	$(objutil)/sconfig/sconfig $(MAINBOARDDIR) $(obj)/mainboard/$(MAINBOARDDIR)

ramstage-y+=$(obj)/mainboard/$(MAINBOARDDIR)/static.c

$(objutil)/%.o: $(objutil)/%.c
	@printf "    HOSTCC     $(subst $(objutil)/,,$(@))\n"
	$(HOSTCC) -MMD -I$(subst $(objutil)/,util/,$(dir $<)) -I$(dir $<) $(HOSTCFLAGS) -c -o $@ $<

$(obj)/%.ramstage.o $(abspath $(obj))/%.ramstage.o: $(obj)/%.c $(obj)/config.h $(OPTION_TABLE_H)
	@printf "    CC         $(subst $(obj)/,,$(@))\n"
	$(CC) -MMD $(CFLAGS) -c -o $@ $<

#######################################################################
# Clean up rules
clean-abuild:
	rm -rf coreboot-builds

clean-for-update-target:
	rm -f $(obj)/coreboot_ram* $(obj)/coreboot.romstage $(obj)/coreboot.pre* $(obj)/coreboot.bootblock $(obj)/coreboot.a
	rm -rf $(obj)/bootblock* $(obj)/romstage* $(obj)/location.*
	rm -f $(obj)/option_table.* $(obj)/crt0.S $(obj)/ldscript
	rm -f $(obj)/mainboard/$(MAINBOARDDIR)/static.c $(obj)/mainboard/$(MAINBOARDDIR)/config.py $(obj)/mainboard/$(MAINBOARDDIR)/static.dot
	rm -f $(obj)/mainboard/$(MAINBOARDDIR)/crt0.s $(obj)/mainboard/$(MAINBOARDDIR)/crt0.disasm
	rm -f $(obj)/mainboard/$(MAINBOARDDIR)/romstage.inc
	rm -f $(obj)/mainboard/$(MAINBOARDDIR)/bootblock.* $(obj)/mainboard/$(MAINBOARDDIR)/dsdt.*
	rm -f $(obj)/cpu/x86/smm/smm_bin.c $(obj)/cpu/x86/smm/smm.* $(obj)/cpu/x86/smm/smm
	$(MAKE) -C payloads/external/SeaBIOS -f Makefile.inc clean

clean-target:
	rm -f $(obj)/coreboot*

#######################################################################
# Development utilities
printcrt0s:
	@echo crt0s=$(crt0s)
	@echo ldscripts=$(ldscripts)

update:
	dongle.py -c /dev/term/1 $(obj)/coreboot.rom EOF

lint:
	FAILED=0; LINTLOG=`mktemp`; \
	for script in util/lint/lint-*; do \
		echo; echo `basename $$script`; \
		grep "^# DESCR:" $$script | sed "s,.*DESCR: *,," ; \
		echo ========; \
		$$script > $$LINTLOG; \
		if [ `wc -l $$LINTLOG | cut -d' ' -f1` -eq 0 ]; then \
			printf "success\n\n"; \
		else \
			echo test failed: ; \
			cat $$LINTLOG; \
			rm -f $$LINTLOG; \
			FAILED=$$(( $$FAILED + 1 )); \
		fi; \
		echo ========; \
	done; \
	test $$FAILED -eq 0 || { echo "ERROR: $$FAILED test(s) failed." &&  exit 1; }; \
	rm -f $$LINTLOG

crossgcc: clean-for-update
	$(MAKE) -C util/crossgcc build

crossgcc-clean: clean-for-update
	$(MAKE) -C util/crossgcc clean

