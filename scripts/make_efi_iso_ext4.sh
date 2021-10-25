#!/bin/sh
min_size=$(($(stat --printf="%s" kernel/carbon)+$(stat --printf="%s" initrd.tar)+$(stat --printf="%s" efibootldr/bootx64.efi)+10000000))
min_size=$(echo "(($min_size / 1000000) + 1)" | bc)
echo $min_size
fallocate -l $min_size'M' esp.part
mkdir tmp/
mkdir tmp/efi
mkdir tmp/efi/boot
cp kernel/carbon tmp
cp initrd.tar tmp
cp efibootldr/bootx64.efi tmp/efi/boot/bootx64.EFI
mkfs.ext4 -L "CARBONUEFI_ISO" esp.part -d tmp
mkdir iso
cp esp.part iso
xorriso -as mkisofs -volid "CARBON_UEFI_ISO" -e esp.part -efi-boot-part \
--efi-boot-image --protective-msdos-label -no-emul-boot \
-isohybrid-gpt-basdat -o Carbon.iso iso
rm -rf iso
rm -rf tmp
