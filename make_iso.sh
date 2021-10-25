#!/bin/sh

export SYSTEM_ROOT=sysroot

./geninitrd default-initrd.sh --compression-method none
./scripts/make_efi_iso_ext4.sh
#./scripts/make_efi_iso.sh
