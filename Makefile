PROJECTS:=efibootldr libc kernel
SOURCE_PACKAGES:=

ALL_MODULES:=$(PROJECTS) $(SOURCE_PACKAGES)

.PHONY: all iso clean build-prep geninitrd $(SYSTEM_HEADER_PROJECTS) $(PROJECTS) \
$(SOURCE_PACKAGES) build-cleanup

export DESTDIR:=$(PWD)/sysroot
export HOST?=$(shell ./default-host.sh)

export AR:=$(HOST)-ar
export AS:=$(HOST)-as
export CC:=$(HOST)-gcc
export CXX:=$(HOST)-g++
export NM:=$(HOST)-nm
export HOST_CC:=gcc
export PREFIX:=/usr
export EXEC_PREFIX:=$(PREFIX)
export BOOTDIR:=/boot
export LIBDIR:=$(EXEC_PREFIX)/lib
export INCLUDEDIR:=$(PREFIX)/include
export BINDIR:=$(PREFIX)/bin
export MANDIR:=/usr/share/man
export PKGDIR:=/pkg
export CFLAGS?=-Os -g
export CPPFLAGS:=

# Configure the cross-compiler to use the desired system root.
export CXX:=$(CXX) --sysroot=$(PWD)/sysroot
export CC:=$(CC) --sysroot=$(PWD)/sysroot

all: iso

clean:
	for module in $(ALL_MODULES); do $(MAKE) -C $$module clean; done

build-prep:
	mkdir -p sysroot
	cd kernel && ../scripts/config_to_header.py include/carbon/config.h && \
	../scripts/driver-build.py build

install-packages: $(PROJECTS)

efibootldr: install-headers
	$(MAKE) -C $@ install

kernel: install-headers
	$(MAKE) -C $@ install

libc: install-headers
	$(MAKE) -C $@ install

$(SOURCE_PACKAGES): install-packages
	$(MAKE) -C $@ install

install-headers: build-prep
	$(MAKE) -C kernel install-headers
	$(MAKE) -C libc install-headers

build-cleanup: install-packages
	cp kernel/kernel.config sysroot/boot
	rm kernel/include/carbon/config.h

	# TODO: Do this in kernel/Makefile
	$(NM) kernel/carbon > Kernel.map

fullbuild: build-cleanup

iso: fullbuild
	cd sysroot && tar cvf ../initrd.tar * && cd ..
	./scripts/make_efi_iso.sh
	rm initrd.tar

qemu: iso
	qemu-system-$(shell ./target-triplet-to-arch.sh $(HOST)) \
	-s -cdrom Carbon.iso -m 512M \
	-monitor stdio -boot d -netdev user,id=u1 -device e1000,netdev=u1 \
	-object filter-dump,id=f1,netdev=u1,file=net.pcap \
	--enable-kvm -smp cores=2,sockets=2 -cpu host,migratable=no -d int -vga std \
	-bios OVMF.fd -usb -no-reboot -no-shutdown
