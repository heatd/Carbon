HOST?=$(shell ../default-host.sh)
HOSTARCH:=$(shell ../target-triplet-to-arch.sh $(HOST))

CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/usr/include
LIBDIR?=$(EXEC_PREFIX)/usr/lib

KERN_IN_TREE_BUILD:=yes

# -fstack-protector-strong

CFLAGS:=$(CFLAGS) -ffreestanding -fbuiltin -Wall -Wextra \
-Wno-unused-parameter -Wno-unused-function -Werror -fpie -DPIC -static

CPPFLAGS:=$(CPPFLAGS) -D__is_carbon_kernel -D_POSIX_C_SOURCE -D_BSD_SOURCE -Iinclude \
-Iinclude/acpica -isystem ../libc/include -DKERNEL_VERSION='"$(KERNEL_VERSION)"' \
-DKERNEL_BRANCH='"$(KERNEL_BRANCH)"' -DKERNEL_ARCH='"$(KERNEL_ARCH)"' -imacros \
include/carbon/config.h

LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib
ARCHDIR:=arch/$(HOSTARCH)

include $(ARCHDIR)/make.config
include kernel.config
include drivers.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS) -D ACPI_LIBRARY -D ACPI_DEBUG_OUTPUT=1 \
-I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol # Needed for EFI
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS) -lk -lgcc
ACPICA_CFLAGS:=$(filter-out -fsanitize=undefined -fsanitize=address -Werror, $(CFLAGS))

OBJS:=\
$(patsubst %.c,%.o,$(wildcard kernel/*.c)) \
$(patsubst %.c,%.o,$(wildcard kernel/*/*.c)) \
$(patsubst %.c,%.o,$(wildcard mm/*.c)) \
$(patsubst %.c,%.o,$(wildcard mm/*/*.c)) \
$(patsubst %.c,%.o,$(wildcard fs/*.c)) \
$(patsubst %.c,%.o,$(wildcard fs/*/*.c)) \
$(patsubst %.c,%.o,$(wildcard $(ARCHDIR)/*.c)) \
$(patsubst %.c,%.o,$(wildcard $(ARCHDIR)/*/*.c)) \
$(filter-out $(ARCHDIR)/crtn.o, $(filter-out $(ARCHDIR)/crti.o, \
$(patsubst %.S,%.o,$(wildcard $(ARCHDIR)/*.S))))

CRTI_OBJ:=$(ARCHDIR)/crti.o
CRTBEGIN_OBJ:=$(shell $(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=crtend.o)
LIBGCC_OBJ:=$(shell $(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=libgcc.a)
CRTN_OBJ:=$(ARCHDIR)/crtn.o

ALL_OUR_OBJS:=\
$(CRTI_OBJ) \
$(OBJS) \
$(CRTN_OBJ) \

OBJ_LINK_LIST:=\
$(CRTI_OBJ) \
$(CRTBEGIN_OBJ) \
$(OBJS) \
$(CRTEND_OBJ) \
$(CRTN_OBJ) \
$(objs-y)

all: onyx-vdso.so.0 carbon

.PHONY: all clean install install-headers install-kernel kernel-modules drivers

carbon: $(OBJ_LINK_LIST) $(ARCHDIR)/linker.ld
	@echo [LD] $@
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(OBJ_LINK_LIST) $(LDFLAGS) $(LIBS)
	cp carbon carbon-unstripped
	strip -g carbon

acpica/%.o: acpica/%.c
	@echo [CC] $<
	@$(CC) -c $< -o $@ -std=c11 $(ACPICA_CFLAGS) $(CPPFLAGS)

%.o: %.c
	@echo [CC] $<
	@$(CC) -c $< -o $@ -std=c11 $(CFLAGS) $(CPPFLAGS) -Wno-error=deprecated-declarations

%.o: %.S
	@echo [AS] $<
	@$(CC) -c $(CPPFLAGS) $< -o $@
%.o: %.cpp
	@echo [CXX] $<
	@$(CXX) -c $< -o $@ -std=c++14 $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS)
$(ARCHDIR)/__vdso.o: $(ARCHDIR)/__vdso.c
	@echo [CC] $<
	@$(CC) -c $< -o $@ -shared -fpic
carbon-vdso.so.0: $(ARCHDIR)/__vdso.o
	@echo [VDSO]
	@$(CC) $< -o $@ -shared -fpic -ffreestanding -nostdlib $(CFLAGS) -z max-page-size=0x1000
clean:
	rm -f carbon $(OBJS) $(ALL_OUR_OBJS) *.o */*.o */*/*.o
	rm -f include/carbon/config.h
	rm -f drivers.config

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -RTv include $(DESTDIR)$(INCLUDEDIR)

install-kernel: carbon
	mkdir -p $(DESTDIR)$(BOOTDIR)
	# Create livefs directories
	mkdir -p $(DESTDIR)/dev/
	mkdir -p $(DESTDIR)/proc/
	mkdir -p $(DESTDIR)/var/
	mkdir -p $(DESTDIR)/tmp/
	mkdir -p $(DESTDIR)/sys/
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $<  $(DESTDIR)$(BOOTDIR)
	#cp onyx-vdso.so.0 $(DESTDIR)$(LIBDIR)
	mkdir -p $(DESTDIR)/usr/share
	mkdir -p $(DESTDIR)/usr/share/man
	mkdir -p $(DESTDIR)/usr/share/man/man9
	#cp .9 $(DESTDIR)/usr/share/man/man9
