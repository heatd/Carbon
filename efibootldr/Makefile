PROGRAM:=bootx64.efi
SHARED_PROGRAM:=bootx64.so
# We use clang for the bootloader
CC:=clang
LD:=ld
CFLAGS:= -O2 -fno-stack-protector -fpic -DPIC -fshort-wchar -mno-red-zone -DHAVE_USE_MS_ABI
CPPFLAGS:=-I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol -DHAVE_USE_MS_ABI -I../sysroot/usr/include
LDFLAGS:=-nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds -shared -Bsymbolic -L /usr/lib64 /usr/lib/crt0-efi-x86_64.o
LDLIBS:= -lefi -lgnuefi
OBJS:=\
$(patsubst %.c,%.o,$(wildcard src/*.c)) \
$(patsubst %.c,%.o,$(wildcard src/*/*.c))

all: $(PROGRAM)
install: $(PROGRAM)
%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
$(SHARED_PROGRAM): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
$(PROGRAM): $(SHARED_PROGRAM)
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $< $@
clean:
	rm -f $(PROGRAM)
	rm -f $(SHARED_PROGRAM)
	rm -f $(OBJS)
