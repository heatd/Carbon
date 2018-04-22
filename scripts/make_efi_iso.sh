#!/bin/sh
min_size=$(($(stat --printf="%s" kernel/carbon)+$(stat --printf="%s" initrd.tar)+$(stat --printf="%s" efibootldr/bootx64.efi)+10000000))
min_size=$(echo "(($min_size / 1000000) + 1)" | bc)
echo $min_size
fallocate -l $min_size'M' esp.part
mkfs.fat -n "CARBONUEFI_ISO" esp.part
mmd -i esp.part EFI
mmd -i esp.part EFI/BOOT
mcopy -i esp.part efibootldr/bootx64.efi ::/EFI/BOOT
mcopy -i esp.part kernel/carbon ::
mcopy -i esp.part initrd.tar ::
mkdir iso
cp esp.part iso
xorriso -as mkisofs -volid "CARBON_UEFI_ISO" -e esp.part -efi-boot-part \
--efi-boot-image --protective-msdos-label -no-emul-boot \
-isohybrid-gpt-basdat -o Carbon.iso iso
rm -rf iso
rm esp.part
